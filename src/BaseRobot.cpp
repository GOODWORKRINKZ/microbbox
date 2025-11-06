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
    
    // API endpoint: Получение ВСЕХ настроек (WiFi + моторы + стики)
    server_->on("/api/settings/get", HTTP_GET, [this](AsyncWebServerRequest* request) {
        if (!wifiSettings_) {
            request->send(500, "application/json", "{\"error\":\"WiFiSettings not initialized\"}");
            return;
        }
        
        String json = "{";
        
        // WiFi настройки
        json += "\"wifi\":{";
        json += "\"ssid\":\"" + wifiSettings_->getSSID() + "\",";
        json += "\"mode\":\"" + String(wifiSettings_->getMode() == WiFiMode::CLIENT ? "CLIENT" : "AP") + "\",";
        json += "\"deviceName\":\"" + wifiSettings_->getDeviceName() + "\"";
        json += "},";
        
        // Настройки моторов
        json += "\"motors\":{";
        json += "\"swapLeftRight\":" + String(wifiSettings_->getMotorSwapLeftRight() ? "true" : "false") + ",";
        json += "\"invertLeft\":" + String(wifiSettings_->getMotorInvertLeft() ? "true" : "false") + ",";
        json += "\"invertRight\":" + String(wifiSettings_->getMotorInvertRight() ? "true" : "false");
        json += "},";
        
        // Настройки стиков
        json += "\"sticks\":{";
        json += "\"invertThrottle\":" + String(wifiSettings_->getInvertThrottleStick() ? "true" : "false") + ",";
        json += "\"invertSteering\":" + String(wifiSettings_->getInvertSteeringStick() ? "true" : "false");
        json += "},";
        
        // Настройки камеры
        json += "\"camera\":{";
        json += "\"hMirror\":" + String(wifiSettings_->getCameraHMirror() ? "true" : "false") + ",";
        json += "\"vFlip\":" + String(wifiSettings_->getCameraVFlip() ? "true" : "false");
        json += "},";
        
        // Настройки эффектов
        json += "\"effects\":{";
        json += "\"effectMode\":" + String(wifiSettings_->getEffectMode());
        json += "}";
        
        json += "}";
        
        request->send(200, "application/json", json);
    });
    
    // API endpoint: Сохранение настроек (поддержка частичных обновлений)
    server_->on("/api/settings/save", HTTP_POST,
        [this](AsyncWebServerRequest* request) {
            // Ответ будет отправлен в onBody
        },
        NULL,
        [this](AsyncWebServerRequest* request, uint8_t *data, size_t len, size_t index, size_t total) {
            static String settingsBody = "";
            
            // Добавляем полученный фрагмент
            for (size_t i = 0; i < len; i++) {
                settingsBody += (char)data[i];
            }
            
            // Если это последний фрагмент
            if (index + len == total) {
                DEBUG_PRINT("Получена конфигурация настроек: ");
                DEBUG_PRINTLN(settingsBody);
                
                if (!wifiSettings_) {
                    request->send(500, "application/json", "{\"status\":\"error\",\"message\":\"WiFiSettings not initialized\"}");
                    settingsBody = "";
                    return;
                }
                
                bool needRestart = false;
                
                // WiFi настройки (требуют перезагрузки)
                if (settingsBody.indexOf("\"ssid\"") >= 0) {
                    int ssidStart = settingsBody.indexOf("\"ssid\":\"") + 8;
                    int ssidEnd = settingsBody.indexOf("\"", ssidStart);
                    if (ssidEnd > ssidStart) {
                        String ssid = settingsBody.substring(ssidStart, ssidEnd);
                        wifiSettings_->setSSID(ssid);
                        needRestart = true;
                    }
                }
                
                if (settingsBody.indexOf("\"password\"") >= 0) {
                    int passStart = settingsBody.indexOf("\"password\":\"") + 12;
                    int passEnd = settingsBody.indexOf("\"", passStart);
                    if (passEnd > passStart) {
                        String password = settingsBody.substring(passStart, passEnd);
                        wifiSettings_->setPassword(password);
                        needRestart = true;
                    }
                }
                
                if (settingsBody.indexOf("\"mode\":\"AP\"") >= 0) {
                    wifiSettings_->setMode(WiFiMode::AP);
                    needRestart = true;
                } else if (settingsBody.indexOf("\"mode\":\"CLIENT\"") >= 0) {
                    wifiSettings_->setMode(WiFiMode::CLIENT);
                    needRestart = true;
                }
                
                // Настройки моторов (применяются сразу)
                if (settingsBody.indexOf("\"swapLeftRight\":true") >= 0) {
                    wifiSettings_->setMotorSwapLeftRight(true);
                } else if (settingsBody.indexOf("\"swapLeftRight\":false") >= 0) {
                    wifiSettings_->setMotorSwapLeftRight(false);
                }
                
                if (settingsBody.indexOf("\"invertLeft\":true") >= 0) {
                    wifiSettings_->setMotorInvertLeft(true);
                } else if (settingsBody.indexOf("\"invertLeft\":false") >= 0) {
                    wifiSettings_->setMotorInvertLeft(false);
                }
                
                if (settingsBody.indexOf("\"invertRight\":true") >= 0) {
                    wifiSettings_->setMotorInvertRight(true);
                } else if (settingsBody.indexOf("\"invertRight\":false") >= 0) {
                    wifiSettings_->setMotorInvertRight(false);
                }
                
                // Настройки стиков (применяются сразу)
                if (settingsBody.indexOf("\"invertThrottle\":true") >= 0) {
                    wifiSettings_->setInvertThrottleStick(true);
                } else if (settingsBody.indexOf("\"invertThrottle\":false") >= 0) {
                    wifiSettings_->setInvertThrottleStick(false);
                }
                
                if (settingsBody.indexOf("\"invertSteering\":true") >= 0) {
                    wifiSettings_->setInvertSteeringStick(true);
                } else if (settingsBody.indexOf("\"invertSteering\":false") >= 0) {
                    wifiSettings_->setInvertSteeringStick(false);
                }
                
                // Настройки камеры (применяются сразу)
                if (settingsBody.indexOf("\"hMirror\":true") >= 0) {
                    wifiSettings_->setCameraHMirror(true);
                } else if (settingsBody.indexOf("\"hMirror\":false") >= 0) {
                    wifiSettings_->setCameraHMirror(false);
                }
                
                if (settingsBody.indexOf("\"vFlip\":true") >= 0) {
                    wifiSettings_->setCameraVFlip(true);
                } else if (settingsBody.indexOf("\"vFlip\":false") >= 0) {
                    wifiSettings_->setCameraVFlip(false);
                }
                
                // Настройки эффектов (сохраняются, применение через loadSettings)
                if (settingsBody.indexOf("\"effectMode\":") >= 0) {
                    int modeStart = settingsBody.indexOf("\"effectMode\":") + 13;
                    int modeEnd = settingsBody.indexOf(",", modeStart);
                    if (modeEnd == -1) modeEnd = settingsBody.indexOf("}", modeStart);
                    if (modeEnd > modeStart) {
                        String modeStr = settingsBody.substring(modeStart, modeEnd);
                        int effectMode = modeStr.toInt();
                        wifiSettings_->setEffectMode(effectMode);
                    }
                }
                
                // Сохраняем в NVS
                if (wifiSettings_->save()) {
                    String response = "{\"status\":\"ok\",\"message\":\"Настройки сохранены\"";
                    if (needRestart) {
                        response += ",\"needRestart\":true";
                    }
                    response += "}";
                    request->send(200, "application/json", response);
                } else {
                    request->send(500, "application/json", "{\"status\":\"error\",\"message\":\"Ошибка сохранения настроек\"}");
                }
                
                settingsBody = "";
            }
        }
    );
    
    // API endpoint: Перезагрузка устройства
    server_->on("/api/restart", HTTP_POST, [](AsyncWebServerRequest* request) {
        request->send(200, "text/plain", "Rebooting...");
        request->onDisconnect([]() {
            delay(100);
            ESP.restart();
        });
    });
    
    // API endpoint: Применение настроек камеры (без перезагрузки)
    server_->on("/api/camera/apply", HTTP_POST, [this](AsyncWebServerRequest* request) {
#ifdef FEATURE_CAMERA
        sensor_t* s = esp_camera_sensor_get();
        if (s != nullptr && wifiSettings_ != nullptr) {
            bool hMirror = wifiSettings_->getCameraHMirror();
            bool vFlip = wifiSettings_->getCameraVFlip();
            DEBUG_PRINTF("API: Применение настроек камеры: hMirror=%s, vFlip=%s\n",
                         hMirror ? "true" : "false", vFlip ? "true" : "false");
            s->set_hmirror(s, hMirror ? 1 : 0);
            s->set_vflip(s, vFlip ? 1 : 0);
            request->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Настройки камеры применены\"}");
        } else {
            request->send(500, "application/json", "{\"status\":\"error\",\"message\":\"Камера не инициализирована\"}");
        }
#else
        request->send(500, "application/json", "{\"status\":\"error\",\"message\":\"Камера отключена\"}");
#endif
    });
    
    // Move command - motor control
    server_->on("/move", HTTP_GET, [this](AsyncWebServerRequest* request) {
        if (request->hasParam("t") && request->hasParam("s")) {
            int throttle = request->getParam("t")->value().toInt();
            int steering = request->getParam("s")->value().toInt();
            
            // Debug logging
            Serial.print("CMD: t=");
            Serial.print(throttle);
            Serial.print(" s=");
            Serial.println(steering);
            
            // Вызываем метод наследника для обработки команды
            handleMotorCommand(throttle, steering);
            request->send(200, "text/plain", "OK");
        } else {
            request->send(400, "text/plain", "Missing parameters");
        }
    });
    
    // Favicon
#ifdef USE_EMBEDDED_RESOURCES
    server_->on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest* request) {
        AsyncWebServerResponse *response = request->beginResponse_P(200, "image/x-icon", faviconIco, faviconIco_len);
        response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        response->addHeader("Pragma", "no-cache");
        response->addHeader("Expires", "0");
        request->send(response);
    });
#else
    server_->on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (SPIFFS.exists("/favicon.ico")) {
            request->send(SPIFFS, "/favicon.ico", "image/x-icon");
        } else {
            request->send(404);
        }
    });
#endif
    
    // Регистрация обработчиков прошивки
    firmwareUpdate_->init(server_);
    
    DEBUG_PRINTLN("Настройка специфичных веб-обработчиков...");
    // Настройка специфичных обработчиков наследником
    setupWebHandlers(server_);
    
    DEBUG_PRINTLN("Регистрация обработчика 404...");
    // Обработчик 404
    server_->onNotFound([](AsyncWebServerRequest* request) {
        request->send(404, "text/plain", "Not Found");
    });
    
    DEBUG_PRINTLN("Запуск веб-сервера...");
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
    // FRAMESIZE_QQVGA = 160x120 для захвата большего пространства по бокам
    config.frame_size = FRAMESIZE_QQVGA;
    config.pixel_format = PIXFORMAT_GRAYSCALE;
    config.jpeg_quality = 12;
    config.fb_count = 1;  // Один буфер достаточно для ЧБ
    config.fb_location = CAMERA_FB_IN_PSRAM;
    DEBUG_PRINTLN("Настройка камеры для Liner: 160x120 ЧБ (QQVGA)");
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
        
        // Применение настроек ориентации камеры из WiFiSettings
        bool hMirror = wifiSettings_->getCameraHMirror();
        bool vFlip = wifiSettings_->getCameraVFlip();
        DEBUG_PRINTF("Применение настроек камеры: hMirror=%s, vFlip=%s\n", 
                     hMirror ? "true" : "false", vFlip ? "true" : "false");
        s->set_hmirror(s, hMirror ? 1 : 0);  // Горизонтальное зеркало
        s->set_vflip(s, vFlip ? 1 : 0);      // Вертикальный переворот
        
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
