#include "MicroBoxRobot.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "driver/gpio.h"
#include "embedded_resources.h"
#include "CameraServer.h"

MicroBoxRobot::MicroBoxRobot() : 
    initialized(false),
    cameraInitialized(false),
    wifiConnected(false),
    wifiAPMode(true),
    server(nullptr),
#ifdef FEATURE_NEOPIXEL
    pixels(nullptr),
#endif
    firmwareUpdate(nullptr),
    currentControlMode(ControlMode::TANK),
#if defined(FEATURE_NEOPIXEL) || defined(FEATURE_BUZZER)
    currentEffectMode(EffectMode::NORMAL),
#endif
    currentLeftSpeed(0),
    currentRightSpeed(0),
#if defined(FEATURE_NEOPIXEL) || defined(FEATURE_BUZZER)
    lastEffectUpdate(0),
    effectState(false),
#endif
    lastLoop(0)
{
    DEBUG_PRINTLN("МикроББокс конструктор");
}

MicroBoxRobot::~MicroBoxRobot() {
    shutdown();
}

bool MicroBoxRobot::init() {
    DEBUG_PRINTLN("Инициализация МикроББокс...");
    
    // Инициализация системы обновления
    firmwareUpdate = new FirmwareUpdate();
    if (!firmwareUpdate->init()) {
        DEBUG_PRINTLN("ОШИБКА: Не удалось инициализировать систему обновления");
        return false;
    }
    
    // ВАЖНО: Сначала камера, потом моторы! (как в Scout32)
    // Камера использует LEDC каналы 0-1, моторы 2-5
    if (!initCamera()) {
        DEBUG_PRINTLN("ОШИБКА: Не удалось инициализировать камеру");
        return false;
    }
    
    // Теперь безопасно инициализировать моторы и остальное
    initMotors();
#ifdef FEATURE_NEOPIXEL
    initLEDs();
#endif
#ifdef FEATURE_BUZZER
    initBuzzer();
#endif
    
    // Запуск WiFi
#if WIFI_MODE_CLIENT
    if (!connectWiFiDHCP(WIFI_SSID_CLIENT, WIFI_PASSWORD_CLIENT)) {
        DEBUG_PRINTLN("Не удалось подключиться к WiFi, запускаем AP режим");
        startWiFiAP();
    }
#else
    startWiFiAP();
#endif
    
    // Инициализация веб-сервера
    initWebServer();
    
    // Запуск камеры сервера на порту 81
    startCameraStreamServer();
    
    initialized = true;
    DEBUG_PRINTLN("МикроББокс успешно инициализирован");
    return true;
}

void MicroBoxRobot::loop() {
    unsigned long currentTime = millis();
    
    // Обработка системы обновления
    if (firmwareUpdate && firmwareUpdate->isInUpdateMode()) {
        firmwareUpdate->loop();
        return; // В режиме обновления не выполняем другие операции
    }
    
#if defined(FEATURE_NEOPIXEL) || defined(FEATURE_BUZZER)
    // Обработка эффектов
    if (currentTime - lastEffectUpdate > 100) { // Обновляем эффекты каждые 100мс
        switch (currentEffectMode) {
            case EffectMode::POLICE:
                playPoliceEffect();
                break;
            case EffectMode::FIRE:
                playFireEffect();
                break;
            case EffectMode::AMBULANCE:
                playAmbulanceEffect();
                break;
            case EffectMode::NORMAL:
                if (currentLeftSpeed != 0 || currentRightSpeed != 0) {
                    playMovementAnimation();
                }
                break;
        }
        lastEffectUpdate = currentTime;
    }
#endif
    
    lastLoop = currentTime;
}

void MicroBoxRobot::shutdown() {
    DEBUG_PRINTLN("Выключение МикроББокс...");
    
    stopMotors();
#ifdef FEATURE_NEOPIXEL
    clearLEDs();
#endif
#ifdef FEATURE_BUZZER
    stopBuzzer();
#endif
    
    if (server) {
        delete server;
        server = nullptr;
    }
    
#ifdef FEATURE_NEOPIXEL
    if (pixels) {
        delete pixels;
        pixels = nullptr;
    }
#endif
    
    if (firmwareUpdate) {
        delete firmwareUpdate;
        firmwareUpdate = nullptr;
    }
    
    initialized = false;
}

void MicroBoxRobot::startWiFiAP() {
    DEBUG_PRINTLN("Запуск WiFi точки доступа...");
    
    WiFi.mode(WIFI_AP);
    
    // Установка hostname для AP режима
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    String hostname = "MICROBBOX-" + mac.substring(6);
    WiFi.setHostname(hostname.c_str());
    
    WiFi.softAP(WIFI_SSID_AP, WIFI_PASSWORD_AP, WIFI_CHANNEL, WIFI_HIDDEN, WIFI_MAX_CONNECTIONS);
    
    IPAddress ip(AP_IP_ADDR);
    IPAddress gateway(AP_GATEWAY);
    IPAddress subnet(AP_SUBNET);
    
    WiFi.softAPConfig(ip, gateway, subnet);
    
    wifiAPMode = true;
    wifiConnected = true;
    
    DEBUG_PRINT("WiFi AP запущена. IP: ");
    DEBUG_PRINTLN(WiFi.softAPIP());
    DEBUG_PRINT("Hostname: ");
    DEBUG_PRINTLN(hostname);
}

bool MicroBoxRobot::connectWiFiDHCP(const char* ssid, const char* password) {
    DEBUG_PRINTLN("Подключение к WiFi сети...");
    
    WiFi.mode(WIFI_STA);
    
    // Установка hostname перед подключением
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    String hostname = "MICROBBOX-" + mac.substring(6);
    WiFi.setHostname(hostname.c_str());
    
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        DEBUG_PRINT(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        wifiAPMode = false;
        wifiConnected = true;
        DEBUG_PRINTLN();
        DEBUG_PRINT("Подключено к WiFi. IP: ");
        DEBUG_PRINTLN(WiFi.localIP());
        DEBUG_PRINT("Hostname: ");
        DEBUG_PRINTLN(hostname);
        return true;
    } else {
        DEBUG_PRINTLN();
        DEBUG_PRINTLN("Не удалось подключиться к WiFi");
        return false;
    }
}

bool MicroBoxRobot::isWiFiConnected() {
    return wifiConnected;
}

IPAddress MicroBoxRobot::getIP() {
    if (wifiAPMode) {
        return WiFi.softAPIP();
    } else {
        return WiFi.localIP();
    }
}

bool MicroBoxRobot::initCamera() {
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
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    
    // Настройки качества в зависимости от доступной памяти
    if (psramFound()) {
        config.frame_size = FRAMESIZE_UXGA;
        config.jpeg_quality = 10;
        config.fb_count = 2;
        DEBUG_PRINTLN("PSRAM найдена, использую высокое качество");
    } else {
        config.frame_size = FRAMESIZE_SVGA;
        config.jpeg_quality = 12;
        config.fb_count = 1;
        DEBUG_PRINTLN("PSRAM не найдена, использую стандартное качество");
    }
    
    // Инициализация камеры
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        DEBUG_PRINTF("Ошибка инициализации камеры: 0x%x\n", err);
        return false;
    }
    
    // Настройка параметров камеры для лучшего качества изображения
    sensor_t* s = esp_camera_sensor_get();
    if (s) {
        s->set_brightness(s, 0);    // Яркость -2 до 2
        s->set_contrast(s, 0);      // Контраст -2 до 2
        s->set_saturation(s, 0);    // Насыщенность -2 до 2
        s->set_special_effect(s, 0); // Эффекты 0-6
        s->set_whitebal(s, 1);      // Баланс белого
        s->set_awb_gain(s, 1);      // Автоматический баланс белого
        s->set_wb_mode(s, 0);       // Режим баланса белого
        s->set_exposure_ctrl(s, 1); // Автоэкспозиция
        s->set_aec2(s, 0);          // AEC алгоритм
        s->set_ae_level(s, 0);      // Уровень экспозиции -2 до 2
        s->set_aec_value(s, 300);   // Значение экспозиции
        s->set_gain_ctrl(s, 1);     // Контроль усиления
        s->set_agc_gain(s, 0);      // Усиление
        s->set_gainceiling(s, (gainceiling_t)0); // Потолок усиления
        s->set_bpc(s, 0);           // Коррекция плохих пикселей
        s->set_wpc(s, 1);           // Коррекция белых пикселей
        s->set_raw_gma(s, 1);       // Гамма коррекция
        s->set_lenc(s, 1);          // Коррекция линзы
        s->set_hmirror(s, 0);       // Горизонтальное зеркало
        s->set_vflip(s, 0);         // Вертикальный переворот
        s->set_dcw(s, 1);           // DCW (Downsize EN)
        s->set_colorbar(s, 0);      // Цветная полоса для тестов
    }
    
    cameraInitialized = true;
    DEBUG_PRINTLN("Камера успешно инициализирована");
    return true;
}

void MicroBoxRobot::initMotors() {
    DEBUG_PRINTLN("Инициализация моторов...");
    
    // Настройка PWM для моторов
    // ВАЖНО: Камера занимает канал 0 (таймер 0)
    // Моторы используют каналы 2,3,4,5 (таймеры 1,1,2,2) - безопасно!
    ledcSetup(MOTOR_PWM_CHANNEL_LF, MOTOR_PWM_FREQ, MOTOR_PWM_RESOLUTION); // канал 3
    ledcSetup(MOTOR_PWM_CHANNEL_LR, MOTOR_PWM_FREQ, MOTOR_PWM_RESOLUTION); // канал 4
    ledcSetup(MOTOR_PWM_CHANNEL_RF, MOTOR_PWM_FREQ, MOTOR_PWM_RESOLUTION); // канал 2
    ledcSetup(MOTOR_PWM_CHANNEL_RR, MOTOR_PWM_FREQ, MOTOR_PWM_RESOLUTION); // канал 5
    
    ledcAttachPin(MOTOR_LEFT_FWD_PIN, MOTOR_PWM_CHANNEL_LF);   // GPIO12 → канал 3
    ledcAttachPin(MOTOR_LEFT_REV_PIN, MOTOR_PWM_CHANNEL_LR);   // GPIO13 → канал 4
    ledcAttachPin(MOTOR_RIGHT_FWD_PIN, MOTOR_PWM_CHANNEL_RF);  // GPIO14 → канал 2
    ledcAttachPin(MOTOR_RIGHT_REV_PIN, MOTOR_PWM_CHANNEL_RR);  // GPIO15 → канал 5
    
    // Остановка моторов (устанавливаем PWM в 0)
    ledcWrite(MOTOR_PWM_CHANNEL_LF, 0);
    ledcWrite(MOTOR_PWM_CHANNEL_LR, 0);
    ledcWrite(MOTOR_PWM_CHANNEL_RF, 0);
    ledcWrite(MOTOR_PWM_CHANNEL_RR, 0);
    
    DEBUG_PRINTLN("Моторы инициализированы");
}

#ifdef FEATURE_NEOPIXEL
void MicroBoxRobot::initLEDs() {
    DEBUG_PRINTLN("Инициализация светодиодов...");
    
    // Настройка белого LED на пине 4 (как в Scout32)
    ledcSetup(NEOPIXEL_LED_CHANNEL, 5000, 8); // 5kHz, 8-bit
    ledcAttachPin(NEOPIXEL_PIN, NEOPIXEL_LED_CHANNEL);
    ledcWrite(NEOPIXEL_LED_CHANNEL, 0); // Выключен по умолчанию
    
    // Инициализация NeoPixel на пине 2
    pixels = new Adafruit_NeoPixel(NEOPIXEL_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
    pixels->begin();
    pixels->setBrightness(LED_BRIGHTNESS_DEFAULT);
    clearLEDs();
    
    DEBUG_PRINTLN("Светодиоды инициализированы");
}
#endif

#ifdef FEATURE_BUZZER
void MicroBoxRobot::initBuzzer() {
    DEBUG_PRINTLN("Инициализация бузера...");
    
    // Настройка PWM для бузера
    ledcSetup(BUZZER_CHANNEL, 1000, BUZZER_RESOLUTION); // Базовая частота 1кГц
    ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL);
    
    stopBuzzer();
    
    DEBUG_PRINTLN("Бузер инициализирован");
}
#endif

void MicroBoxRobot::initWebServer() {
    DEBUG_PRINTLN("Инициализация веб-сервера...");
    
    server = new AsyncWebServer(WIFI_PORT);
    
    // Главная страница
    server->on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleRoot(request);
    });
    
    // API команды - тестовый GET для отладки
    server->on("/command", HTTP_GET, [this](AsyncWebServerRequest *request) {
        DEBUG_PRINTLN("GET /command вызван!");
        
        if (request->hasParam("test")) {
            String testValue = request->getParam("test")->value();
            DEBUG_PRINT("Test параметр: ");
            DEBUG_PRINTLN(testValue);
            
            if (testValue == "motor") {
                DEBUG_PRINTLN("Тест моторов: вперед на 50%");
                setMotorSpeed(50, 50);
                delay(2000);
                setMotorSpeed(0, 0);
                request->send(200, "text/plain", "Motors test OK: forward 50% for 2 sec");
                return;
            }
        }
        
        request->send(200, "text/plain", "Command GET endpoint. Try: /command?test=motor");
    });
    
    // API команды - POST для JSON
    server->on("/command", HTTP_POST, 
        [this](AsyncWebServerRequest *request) {
            // Ответ будет отправлен в onBody
        },
        NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            // Накапливаем данные пока не получим всё
            static String commandBody = "";
            
            // Добавляем полученный фрагмент
            for (size_t i = 0; i < len; i++) {
                commandBody += (char)data[i];
            }
            
            // Если это последний фрагмент (index + len == total)
            if (index + len == total) {
                DEBUG_PRINT("Получена команда: ");
                DEBUG_PRINTLN(commandBody);
                
                // Парсинг JSON команды
                if (commandBody.indexOf("move") >= 0) {
                    int leftSpeed = 0, rightSpeed = 0;
                    
                    int leftPos = commandBody.indexOf("\"left\":");
                    int rightPos = commandBody.indexOf("\"right\":");
                    
                    if (leftPos >= 0) {
                        leftSpeed = commandBody.substring(leftPos + 7).toInt();
                    }
                    if (rightPos >= 0) {
                        rightSpeed = commandBody.substring(rightPos + 8).toInt();
                    }
                    
                    DEBUG_PRINT("Движение: left=");
                    DEBUG_PRINT(leftSpeed);
                    DEBUG_PRINT(" right=");
                    DEBUG_PRINTLN(rightSpeed);
                    
                    setMotorSpeed(leftSpeed, rightSpeed);
                    request->send(200, "application/json", "{\"status\":\"ok\",\"action\":\"Поехал: left=" + String(leftSpeed) + ", right=" + String(rightSpeed) + "\"}");
                }
                else if (commandBody.indexOf("flashlight") >= 0) {
#ifdef FEATURE_NEOPIXEL
                    static bool flashlightState = false;
                    flashlightState = !flashlightState;
                    
                    if (flashlightState) {
                        pixels->setPixelColor(0, pixels->Color(255, 255, 255));
                        request->send(200, "application/json", "{\"status\":\"ok\",\"action\":\"Фонарик включен\"}");
                    } else {
                        pixels->setPixelColor(0, pixels->Color(0, 0, 0));
                        request->send(200, "application/json", "{\"status\":\"ok\",\"action\":\"Фонарик выключен\"}");
                    }
                    pixels->show();
#else
                    request->send(200, "application/json", "{\"status\":\"ok\",\"action\":\"Фонарик (нет LED)\"}");
#endif
                }
                else if (commandBody.indexOf("horn") >= 0) {
#ifdef FEATURE_BUZZER
                    if (commandBody.indexOf("true") >= 0) {
                        playTone(800, 0);
                        request->send(200, "application/json", "{\"status\":\"ok\",\"action\":\"Сигналю!\"}");
                    } else {
                        stopBuzzer();
                        request->send(200, "application/json", "{\"status\":\"ok\",\"action\":\"Сигнал выкл\"}");
                    }
#else
                    request->send(200, "application/json", "{\"status\":\"ok\",\"action\":\"Сигнал (нет бузера)\"}");
#endif
                }
                else if (commandBody.indexOf("setEffectMode") >= 0) {
#if defined(FEATURE_NEOPIXEL) || defined(FEATURE_BUZZER)
                    if (commandBody.indexOf("police") >= 0) {
                        setEffectMode(EffectMode::POLICE);
                        request->send(200, "application/json", "{\"status\":\"ok\",\"action\":\"Режим: Полиция\"}");
                    } else if (commandBody.indexOf("fire") >= 0) {
                        setEffectMode(EffectMode::FIRE);
                        request->send(200, "application/json", "{\"status\":\"ok\",\"action\":\"Режим: Пожарная\"}");
                    } else if (commandBody.indexOf("ambulance") >= 0) {
                        setEffectMode(EffectMode::AMBULANCE);
                        request->send(200, "application/json", "{\"status\":\"ok\",\"action\":\"Режим: Скорая\"}");
                    } else {
                        setEffectMode(EffectMode::NORMAL);
                        request->send(200, "application/json", "{\"status\":\"ok\",\"action\":\"Режим: Обычный\"}");
                    }
#else
                    request->send(200, "application/json", "{\"status\":\"ok\",\"action\":\"Эффекты недоступны\"}");
#endif
                }
                else if (commandBody.indexOf("enterUpdateMode") >= 0) {
                    enterUpdateMode();
                    request->send(200, "application/json", "{\"status\":\"ok\",\"action\":\"Режим обновления\"}");
                }
                
                // Очищаем буфер для следующей команды
                commandBody = "";
            }
        }
    );
    
    // Статические ресурсы
    #ifdef EMBEDDED_RESOURCES_H
    server->on("/styles.css", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response = request->beginResponse_P(200, "text/css; charset=utf-8", stylesCss, stylesCss_len);
        response->addHeader("Cache-Control", "max-age=86400");
        request->send(response);
    });
    
    server->on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response = request->beginResponse_P(200, "application/javascript; charset=utf-8", scriptJs, scriptJs_len);
        response->addHeader("Cache-Control", "max-age=86400");
        request->send(response);
    });
    
    server->on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response = request->beginResponse_P(200, "image/x-icon", faviconIco, faviconIco_len);
        response->addHeader("Cache-Control", "max-age=86400");
        request->send(response);
    });
    #endif
    
    // 404 обработчик
    server->onNotFound([this](AsyncWebServerRequest *request) {
        this->handleNotFound(request);
    });
    
    server->begin();
    DEBUG_PRINTLN("Веб-сервер запущен на порту 80");
}

void MicroBoxRobot::handleRoot(AsyncWebServerRequest *request) {
    #ifdef USE_EMBEDDED_RESOURCES
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html; charset=utf-8", indexHtml, indexHtml_len);
    #else
    // Fallback HTML если ресурсы не встроены
    String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>МикроББокс</title></head><body>";
    html += "<h1>МикроББокс</h1>";
    html += "<p>Добро пожаловать в систему управления МикроББокс!</p>";
    html += "<p>Видео стрим: <img src='/stream' style='max-width:100%'></p>";
    html += "<p>Статические ресурсы не загружены. Пожалуйста, пересоберите проект.</p>";
    html += "</body></html>";
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html; charset=utf-8", html);
    #endif
    
    response->addHeader("Cache-Control", "no-cache");
    request->send(response);
}

void MicroBoxRobot::handleNotFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Страница не найдена");
}

void MicroBoxRobot::setMotorSpeed(int leftSpeed, int rightSpeed) {
    // Ограничение значений
    leftSpeed = constrain(leftSpeed, -100, 100);
    rightSpeed = constrain(rightSpeed, -100, 100);
    
    currentLeftSpeed = leftSpeed;
    currentRightSpeed = rightSpeed;
    
    // Преобразование в PWM значения (0-8191 для 13-битного разрешения)
    int leftPWM = map(abs(leftSpeed), 0, 100, 0, 8191);
    int rightPWM = map(abs(rightSpeed), 0, 100, 0, 8191);
    
    // Левый мотор
    if (leftSpeed > 0) {
        ledcWrite(MOTOR_PWM_CHANNEL_LF, leftPWM);
        ledcWrite(MOTOR_PWM_CHANNEL_LR, 0);
    } else if (leftSpeed < 0) {
        ledcWrite(MOTOR_PWM_CHANNEL_LF, 0);
        ledcWrite(MOTOR_PWM_CHANNEL_LR, leftPWM);
    } else {
        ledcWrite(MOTOR_PWM_CHANNEL_LF, 0);
        ledcWrite(MOTOR_PWM_CHANNEL_LR, 0);
    }
    
    // Правый мотор
    if (rightSpeed > 0) {
        ledcWrite(MOTOR_PWM_CHANNEL_RR, rightPWM);
        ledcWrite(MOTOR_PWM_CHANNEL_RF, 0);
    } else if (rightSpeed < 0) {
        ledcWrite(MOTOR_PWM_CHANNEL_RR, 0);
        ledcWrite(MOTOR_PWM_CHANNEL_RF, rightPWM);
    } else {
        ledcWrite(MOTOR_PWM_CHANNEL_RR, 0);
        ledcWrite(MOTOR_PWM_CHANNEL_RF, 0);
    }
    
    DEBUG_PRINTF("Моторы: левый=%d, правый=%d\n", leftSpeed, rightSpeed);
}

void MicroBoxRobot::moveForward(int speed) {
    setMotorSpeed(speed, speed);
}

void MicroBoxRobot::moveBackward(int speed) {
    setMotorSpeed(-speed, -speed);
}

void MicroBoxRobot::turnLeft(int speed) {
    setMotorSpeed(-speed, speed);
}

void MicroBoxRobot::turnRight(int speed) {
    setMotorSpeed(speed, -speed);
}

void MicroBoxRobot::stopMotors() {
    setMotorSpeed(0, 0);
}

#ifdef FEATURE_NEOPIXEL
void MicroBoxRobot::setLEDColor(int ledIndex, uint32_t color) {
    if (pixels && ledIndex >= 0 && ledIndex < NEOPIXEL_COUNT) {
        pixels->setPixelColor(ledIndex, color);
    }
}

void MicroBoxRobot::setAllLEDs(uint32_t color) {
    if (pixels) {
        for (int i = 0; i < NEOPIXEL_COUNT; i++) {
            pixels->setPixelColor(i, color);
        }
    }
}

void MicroBoxRobot::clearLEDs() {
    setAllLEDs(0);
    updateLEDs();
}

void MicroBoxRobot::updateLEDs() {
    if (pixels) {
        pixels->show();
    }
}
#endif

#if defined(FEATURE_NEOPIXEL) || defined(FEATURE_BUZZER)
void MicroBoxRobot::setEffectMode(EffectMode mode) {
    currentEffectMode = mode;
    DEBUG_PRINTF("Режим эффектов изменен на: %d\n", (int)mode);
}

void MicroBoxRobot::playPoliceEffect() {
    static unsigned long lastToggle = 0;
    unsigned long currentTime = millis();
    
    if (currentTime - lastToggle > 250) { // Переключение каждые 250мс
        effectState = !effectState;
        
        if (effectState) {
#ifdef FEATURE_NEOPIXEL
            // Синие светодиоды сзади, красные спереди
            setLEDColor(0, pixels->Color(0, 0, 255));   // Задний левый - синий
            setLEDColor(1, pixels->Color(0, 0, 255));   // Задний правый - синий
            setLEDColor(2, pixels->Color(255, 0, 0));   // Передний - красный
#endif
#ifdef FEATURE_BUZZER            
            // Звук сирены
            playTone(800, 0);
#endif
        } else {
#ifdef FEATURE_NEOPIXEL
            // Красные светодиоды сзади, синие спереди
            setLEDColor(0, pixels->Color(255, 0, 0));   // Задний левый - красный
            setLEDColor(1, pixels->Color(255, 0, 0));   // Задний правый - красный
            setLEDColor(2, pixels->Color(0, 0, 255));   // Передний - синий
#endif
#ifdef FEATURE_BUZZER            
            // Другой тон сирены
            playTone(1000, 0);
#endif
        }
        
#ifdef FEATURE_NEOPIXEL
        updateLEDs();
#endif
        lastToggle = currentTime;
    }
}

void MicroBoxRobot::playFireEffect() {
    static unsigned long lastToggle = 0;
    unsigned long currentTime = millis();
    
    if (currentTime - lastToggle > 200) { // Быстрое мигание
        effectState = !effectState;
        
        if (effectState) {
#ifdef FEATURE_NEOPIXEL
            setAllLEDs(pixels->Color(255, 0, 0)); // Красный
#endif
#ifdef FEATURE_BUZZER
            playTone(900, 0);
#endif
        } else {
#ifdef FEATURE_NEOPIXEL
            setAllLEDs(pixels->Color(255, 165, 0)); // Оранжевый
#endif
#ifdef FEATURE_BUZZER
            playTone(1100, 0);
#endif
        }
        
#ifdef FEATURE_NEOPIXEL
        updateLEDs();
#endif
        lastToggle = currentTime;
    }
}

void MicroBoxRobot::playAmbulanceEffect() {
    static unsigned long lastToggle = 0;
    unsigned long currentTime = millis();
    
    if (currentTime - lastToggle > 300) { // Медленнее полиции
        effectState = !effectState;
        
        if (effectState) {
#ifdef FEATURE_NEOPIXEL
            setAllLEDs(pixels->Color(255, 255, 255)); // Белый
#endif
#ifdef FEATURE_BUZZER
            playTone(750, 0);
#endif
        } else {
#ifdef FEATURE_NEOPIXEL
            setAllLEDs(pixels->Color(255, 0, 0)); // Красный
#endif
#ifdef FEATURE_BUZZER
            playTone(1050, 0);
#endif
        }
        
#ifdef FEATURE_NEOPIXEL
        updateLEDs();
#endif
        lastToggle = currentTime;
    }
}

void MicroBoxRobot::playMovementAnimation() {
#ifdef FEATURE_NEOPIXEL
    static unsigned long lastUpdate = 0;
    static int animationStep = 0;
    unsigned long currentTime = millis();
    
    if (currentTime - lastUpdate > 100) { // Обновление каждые 100мс
        // Анимация "бегущий огонь"
        clearLEDs();
        
        int activeLED = animationStep % NEOPIXEL_COUNT;
        setLEDColor(activeLED, pixels->Color(0, 255, 0)); // Зеленый
        
        updateLEDs();
        
        animationStep++;
        lastUpdate = currentTime;
    }
#endif
}
#endif

#ifdef FEATURE_BUZZER
void MicroBoxRobot::playTone(int frequency, int duration) {
    if (frequency > 0) {
        ledcWriteTone(BUZZER_CHANNEL, frequency);
        if (duration > 0) {
            // Для временных тонов можно добавить таймер
        }
    } else {
        stopBuzzer();
    }
}

void MicroBoxRobot::playMelody(const int* melody, const int* noteDurations, int noteCount) {
    // TODO: Реализовать проигрывание мелодий
    DEBUG_PRINTLN("Проигрывание мелодий пока не реализовано");
}

void MicroBoxRobot::stopBuzzer() {
    ledcWrite(BUZZER_CHANNEL, 0);
}
#endif

void MicroBoxRobot::setControlMode(ControlMode mode) {
    currentControlMode = mode;
    DEBUG_PRINTF("Режим управления изменен на: %d\n", (int)mode);
}

void MicroBoxRobot::processControlInput(int leftX, int leftY, int rightX, int rightY) {
    // Обработка ввода в зависимости от режима управления
    int leftSpeed = 0;
    int rightSpeed = 0;
    
    if (currentControlMode == ControlMode::TANK) {
        // Танковый режим
        leftSpeed = leftY;
        rightSpeed = rightY;
    } else if (currentControlMode == ControlMode::DIFFERENTIAL) {
        // Дифференциальный режим
        int speed = rightY;
        int turn = leftX;
        
        leftSpeed = speed - turn;
        rightSpeed = speed + turn;
    }
    
    setMotorSpeed(leftSpeed, rightSpeed);
}

void MicroBoxRobot::enterUpdateMode() {
    DEBUG_PRINTLN("Вход в режим обновления прошивки");
    
    if (firmwareUpdate) {
        // Остановка всех операций
        stopMotors();
#ifdef FEATURE_NEOPIXEL
        clearLEDs();
#endif
#ifdef FEATURE_BUZZER
        stopBuzzer();
#endif
        
        // Остановка основного сервера
        if (server) {
            server->end();
        }
        
        // Запуск режима обновления
        firmwareUpdate->startUpdateMode();
    }
}

bool MicroBoxRobot::isInUpdateMode() const {
    return firmwareUpdate && firmwareUpdate->isInUpdateMode();
}