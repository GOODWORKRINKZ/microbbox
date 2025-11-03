#include "BaseRobot.h"
#include "hardware_config.h"
#include "CameraServer.h"
#include <ESPmDNS.h>
#include <esp_camera.h>

#ifdef USE_EMBEDDED_RESOURCES
#include "embedded_resources.h"
#endif

BaseRobot::BaseRobot() :
    initialized_(false),
    cameraInitialized_(false),
    wifiConnected_(false),
    wifiAPMode_(true),
    server_(nullptr),
    wifiSettings_(nullptr),
    firmwareUpdate_(nullptr),
    motorController_(nullptr)
{
    // Генерация имени устройства на основе MAC адреса
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char macStr[7];
    snprintf(macStr, sizeof(macStr), "%02X%02X%02X", mac[3], mac[4], mac[5]);
    deviceName_ = String("MICROBBOX-") + String(macStr);
}

BaseRobot::~BaseRobot() {
    shutdown();
}

bool BaseRobot::init() {
    if (initialized_) {
        return true;
    }
    
    DEBUG_PRINTLN("=== Инициализация BaseRobot ===");
    
    // Инициализация WiFi настроек
    wifiSettings_ = new WiFiSettings();
    if (!wifiSettings_->init()) {
        DEBUG_PRINTLN("ОШИБКА: Не удалось инициализировать WiFi настройки");
        return false;
    }
    
    // Инициализация системы обновления
    // Тип робота определяется автоматически из конфигурации компиляции
    firmwareUpdate_ = new FirmwareUpdate();
    
    // Инициализация WiFi
    if (!initWiFi()) {
        DEBUG_PRINTLN("ОШИБКА: Не удалось инициализировать WiFi");
        return false;
    }
    
    // Инициализация mDNS
    if (!initMDNS()) {
        DEBUG_PRINTLN("ПРЕДУПРЕЖДЕНИЕ: Не удалось инициализировать mDNS");
    }
    
    // Инициализация камеры (если включена)
#ifdef FEATURE_CAMERA
    if (!initCamera()) {
        DEBUG_PRINTLN("ОШИБКА: Не удалось инициализировать камеру");
        return false;
    }
#endif
    
    // Инициализация веб-сервера
    if (!initWebServer()) {
        DEBUG_PRINTLN("ОШИБКА: Не удалось инициализировать веб-сервер");
        return false;
    }
    
    // Инициализация специфичных компонентов наследника
    if (!initSpecificComponents()) {
        DEBUG_PRINTLN("ОШИБКА: Не удалось инициализировать специфичные компоненты");
        return false;
    }
    
    initialized_ = true;
    DEBUG_PRINTLN("=== BaseRobot успешно инициализирован ===");
    return true;
}

void BaseRobot::update() {
    if (!initialized_) {
        return;
    }
    
    // Обновление специфичных компонентов
    updateSpecificComponents();
}

void BaseRobot::shutdown() {
    if (initialized_) {
        shutdownSpecificComponents();
        
        if (motorController_) {
            motorController_->shutdown();
        }
        
        if (server_) {
            server_->end();
            delete server_;
            server_ = nullptr;
        }
        
        if (firmwareUpdate_) {
            delete firmwareUpdate_;
            firmwareUpdate_ = nullptr;
        }
        
        if (wifiSettings_) {
            delete wifiSettings_;
            wifiSettings_ = nullptr;
        }
        
        WiFi.disconnect();
        
        initialized_ = false;
    }
}

void BaseRobot::loop() {
    update();
    delay(10); // Небольшая задержка для стабильности
}

IPAddress BaseRobot::getIP() const {
    if (wifiAPMode_) {
        return WiFi.softAPIP();
    } else {
        return WiFi.localIP();
    }
}

String BaseRobot::getDeviceName() const {
    return deviceName_;
}

bool BaseRobot::initWiFi() {
    DEBUG_PRINTLN("Инициализация WiFi...");
    
    if (wifiSettings_->getMode() == WiFiMode::CLIENT) {
        // Пытаемся подключиться к сохраненной сети
        if (!connectToSavedWiFi()) {
            DEBUG_PRINTLN("Не удалось подключиться к сохраненной WiFi, запускаем AP режим");
            startWiFiAP();
        }
    } else {
        // Режим точки доступа
        startWiFiAP();
    }
    
    return true;
}

bool BaseRobot::initWebServer() {
    DEBUG_PRINTLN("Инициализация веб-сервера...");
    
    server_ = new AsyncWebServer(WIFI_PORT);
    
    // Регистрация общего обработчика главной страницы
    server_->on("/", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleRoot(request);
    });
    
    // Регистрация обработчиков статических ресурсов
#ifdef USE_EMBEDDED_RESOURCES
    server_->on("/styles.css", HTTP_GET, [](AsyncWebServerRequest* request) {
        AsyncWebServerResponse *response = request->beginResponse_P(200, "text/css; charset=utf-8", stylesCss, stylesCss_len);
        response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        response->addHeader("Pragma", "no-cache");
        response->addHeader("Expires", "0");
        request->send(response);
    });
    
    server_->on("/script.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        AsyncWebServerResponse *response = request->beginResponse_P(200, "application/javascript; charset=utf-8", scriptJs, scriptJs_len);
        response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        response->addHeader("Pragma", "no-cache");
        response->addHeader("Expires", "0");
        request->send(response);
    });
#else
    // Fallback: загрузка из SPIFFS
    server_->on("/styles.css", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (SPIFFS.exists("/styles.css")) {
            request->send(SPIFFS, "/styles.css", "text/css; charset=utf-8");
        } else {
            request->send(404, "text/plain", "styles.css not found");
        }
    });
    
    server_->on("/script.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (SPIFFS.exists("/script.js")) {
            request->send(SPIFFS, "/script.js", "application/javascript; charset=utf-8");
        } else {
            request->send(404, "text/plain", "script.js not found");
        }
    });
#endif
    
    // API endpoint: конфигурация
    server_->on("/api/config", HTTP_GET, [this](AsyncWebServerRequest* request) {
        String json = "{";
        json += "\"version\":\"" + String(GIT_VERSION) + "\",";
        json += "\"robotType\":\"" + String(robotTypeToLowerString(getRobotType())) + "\"";
        json += "}";
        request->send(200, "application/json", json);
    });
    
    // Move command - motor control
    server_->on("/move", HTTP_GET, [this](AsyncWebServerRequest* request) {
        if (request->hasParam("t") && request->hasParam("s")) {
            int throttle = request->getParam("t")->value().toInt();
            int steering = request->getParam("s")->value().toInt();
            motorController_->move(throttle, steering);
            request->send(200, "text/plain", "OK");
        } else {
            request->send(400, "text/plain", "Missing parameters");
        }
    });
    
    // Favicon
    server_->on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(404);
    });
    
    // Регистрация обработчиков прошивки
    firmwareUpdate_->init(server_);
    
    // Настройка специфичных обработчиков наследником
    setupWebHandlers(server_);
    
    // Обработчик 404
    server_->onNotFound([](AsyncWebServerRequest* request) {
        request->send(404, "text/plain", "Not Found");
    });
    
    // Запуск сервера
    server_->begin();
    
    DEBUG_PRINTLN("Веб-сервер запущен");
    return true;
}

bool BaseRobot::initMDNS() {
    DEBUG_PRINTLN("Инициализация mDNS...");
    
    // Получаем последние 6 символов MAC
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char macStr[7];
    snprintf(macStr, sizeof(macStr), "%02x%02x%02x", mac[3], mac[4], mac[5]);
    
    String mdnsName = String(macStr) + ".microbbox";
    
    if (!MDNS.begin(mdnsName.c_str())) {
        DEBUG_PRINTLN("ОШИБКА: Не удалось запустить mDNS");
        return false;
    }
    
    MDNS.addService("http", "tcp", WIFI_PORT);
    
    DEBUG_PRINTF("mDNS запущен: http://%s.local\n", mdnsName.c_str());
    return true;
}

bool BaseRobot::initCamera() {
#ifdef FEATURE_CAMERA
    DEBUG_PRINTLN("Инициализация камеры...");
    
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;  // Исправлено: sccb (две 'c')
    config.pin_sccb_scl = SIOC_GPIO_NUM;  // Исправлено: sccb (две 'c')
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    config.grab_mode = CAMERA_GRAB_LATEST;  // Всегда брать последний кадр
    
    // Параметры для разных типов роботов
#ifdef TARGET_LINER
    // Для линейного робота нужно низкое разрешение ЧБ
    config.frame_size = FRAMESIZE_96X96;
    config.pixel_format = PIXFORMAT_GRAYSCALE;
    config.jpeg_quality = 12;
    config.fb_count = 1;  // Один буфер достаточно для ЧБ
    config.fb_location = CAMERA_FB_IN_PSRAM;
    DEBUG_PRINTLN("Настройка камеры для Liner: 96x96 ЧБ");
#else
    // Для остальных - стандартное разрешение с учетом PSRAM
    if (psramFound()) {
        config.frame_size = FRAMESIZE_QVGA;  // Стабильное разрешение 320x240
        config.jpeg_quality = 10;
        config.fb_count = 2;
        config.fb_location = CAMERA_FB_IN_PSRAM;
        DEBUG_PRINTLN("PSRAM найдена, использую двойную буферизацию");
    } else {
        config.frame_size = FRAMESIZE_QVGA;
        config.jpeg_quality = 12;
        config.fb_count = 1;
        config.fb_location = CAMERA_FB_IN_DRAM;
        DEBUG_PRINTLN("PSRAM не найдена, использую одиночный буфер");
    }
#endif
    
    // Инициализация камеры
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        DEBUG_PRINTF("ОШИБКА: Камера не инициализирована: 0x%x\n", err);
        return false;
    }
    
    // Настройка параметров камеры
    sensor_t* s = esp_camera_sensor_get();
    if (s != nullptr) {
        s->set_brightness(s, 0);     // -2 to 2
        s->set_contrast(s, 0);       // -2 to 2
        s->set_saturation(s, 0);     // -2 to 2
        s->set_special_effect(s, 0); // 0 to 6 (0 - No Effect)
        s->set_whitebal(s, 1);       // 0 = disable , 1 = enable
        s->set_awb_gain(s, 1);       // 0 = disable , 1 = enable
        s->set_wb_mode(s, 0);        // 0 to 4
        s->set_exposure_ctrl(s, 1);  // 0 = disable , 1 = enable
        s->set_aec2(s, 0);           // 0 = disable , 1 = enable
        s->set_ae_level(s, 0);       // -2 to 2
        s->set_aec_value(s, 300);    // 0 to 1200
        s->set_gain_ctrl(s, 1);      // 0 = disable , 1 = enable
        s->set_agc_gain(s, 0);       // 0 to 30
        s->set_gainceiling(s, (gainceiling_t)0);  // 0 to 6
        s->set_bpc(s, 0);            // 0 = disable , 1 = enable
        s->set_wpc(s, 1);            // 0 = disable , 1 = enable
        s->set_raw_gma(s, 1);        // 0 = disable , 1 = enable
        s->set_lenc(s, 1);           // 0 = disable , 1 = enable
        s->set_hmirror(s, 0);        // 0 = disable , 1 = enable
        s->set_vflip(s, 0);          // 0 = disable , 1 = enable
        s->set_dcw(s, 1);            // 0 = disable , 1 = enable
        s->set_colorbar(s, 0);       // 0 = disable , 1 = enable
    }
    
    // Запуск камера-сервера
    startCameraStreamServer();
    
    cameraInitialized_ = true;
    DEBUG_PRINTLN("Камера инициализирована");
    return true;
#else
    return true; // Камера не включена
#endif
}

void BaseRobot::startWiFiAP() {
    DEBUG_PRINTLN("Запуск WiFi в режиме точки доступа...");
    
    WiFi.mode(WIFI_AP);
    
    IPAddress local_IP(AP_IP_ADDR);
    IPAddress gateway(AP_GATEWAY);
    IPAddress subnet(AP_SUBNET);
    
    WiFi.softAPConfig(local_IP, gateway, subnet);
    
    bool result = WiFi.softAP(deviceName_.c_str(), WIFI_PASSWORD_AP, WIFI_CHANNEL, WIFI_HIDDEN, WIFI_MAX_CONNECTIONS);
    
    if (result) {
        wifiAPMode_ = true;
        wifiConnected_ = true;
        DEBUG_PRINTLN("WiFi AP запущен");
        DEBUG_PRINT("SSID: ");
        DEBUG_PRINTLN(deviceName_);
        DEBUG_PRINT("IP: ");
        DEBUG_PRINTLN(WiFi.softAPIP());
    } else {
        DEBUG_PRINTLN("ОШИБКА: Не удалось запустить WiFi AP");
    }
}

bool BaseRobot::connectToSavedWiFi() {
    String ssid = wifiSettings_->getSSID();
    String password = wifiSettings_->getPassword();
    
    if (ssid.isEmpty()) {
        DEBUG_PRINTLN("Нет сохраненных настроек WiFi");
        return false;
    }
    
    return connectWiFiDHCP(ssid.c_str(), password.c_str());
}

bool BaseRobot::connectWiFiDHCP(const char* ssid, const char* password) {
    DEBUG_PRINT("Подключение к WiFi: ");
    DEBUG_PRINTLN(ssid);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        DEBUG_PRINT(".");
        attempts++;
    }
    DEBUG_PRINTLN("");
    
    if (WiFi.status() == WL_CONNECTED) {
        wifiAPMode_ = false;
        wifiConnected_ = true;
        DEBUG_PRINTLN("WiFi подключен");
        DEBUG_PRINT("IP: ");
        DEBUG_PRINTLN(WiFi.localIP());
        return true;
    } else {
        DEBUG_PRINTLN("Не удалось подключиться к WiFi");
        return false;
    }
}

void BaseRobot::handleRoot(AsyncWebServerRequest* request) {
#ifdef USE_EMBEDDED_RESOURCES
    // Отправка встроенных ресурсов (из embedded_resources.h)
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html; charset=UTF-8", indexHtml, indexHtml_len);
    response->addHeader("Cache-Control", "no-cache");
    request->send(response);
#else
    // Fallback: отправка из SPIFFS или заглушка
    if (SPIFFS.exists("/index.html")) {
        request->send(SPIFFS, "/index.html", "text/html; charset=UTF-8");
    } else {
        // Заглушка если ресурсы не встроены и файл не найден
        String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>МикРоББокс</title></head><body>";
        html += "<h1>МикРоББокс " + getRobotTypeString() + "</h1>";
        html += "<p>Веб-интерфейс недоступен. Пересоберите проект с USE_EMBEDDED_RESOURCES или загрузите файлы в SPIFFS.</p>";
        html += "<p>IP: " + WiFi.localIP().toString() + "</p>";
        html += "<p><a href='/update'>Обновление прошивки</a></p>";
        html += "</body></html>";
        request->send(200, "text/html; charset=UTF-8", html);
    }
#endif
}
