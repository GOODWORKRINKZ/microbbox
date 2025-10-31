#include "MicroBoxRobot.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#ifdef EMBEDDED_RESOURCES_H
#include "embedded_resources.h"
#endif

MicroBoxRobot::MicroBoxRobot() : 
    initialized(false),
    cameraInitialized(false),
    wifiConnected(false),
    wifiAPMode(true),
    server(nullptr),
    pixels(nullptr),
    firmwareUpdate(nullptr),
    currentControlMode(ControlMode::TANK),
    currentEffectMode(EffectMode::NORMAL),
    currentLeftSpeed(0),
    currentRightSpeed(0),
    lastEffectUpdate(0),
    effectState(false),
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
    
    // Инициализация компонентов
    if (!initCamera()) {
        DEBUG_PRINTLN("ОШИБКА: Не удалось инициализировать камеру");
        return false;
    }
    
    initMotors();
    initLEDs();
    initBuzzer();
    
    // Запуск WiFi в режиме AP
    startWiFiAP();
    
    // Инициализация веб-сервера
    initWebServer();
    
    // Запуск камеры сервера
    startCameraServer();
    
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
    
    lastLoop = currentTime;
}

void MicroBoxRobot::shutdown() {
    DEBUG_PRINTLN("Выключение МикроББокс...");
    
    stopMotors();
    clearLEDs();
    stopBuzzer();
    
    if (server) {
        delete server;
        server = nullptr;
    }
    
    if (pixels) {
        delete pixels;
        pixels = nullptr;
    }
    
    if (firmwareUpdate) {
        delete firmwareUpdate;
        firmwareUpdate = nullptr;
    }
    
    initialized = false;
}

void MicroBoxRobot::startWiFiAP() {
    DEBUG_PRINTLN("Запуск WiFi точки доступа...");
    
    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL, WIFI_HIDDEN, WIFI_MAX_CONNECTIONS);
    
    IPAddress ip(AP_IP_ADDR);
    IPAddress gateway(AP_GATEWAY);
    IPAddress subnet(AP_SUBNET);
    
    WiFi.softAPConfig(ip, gateway, subnet);
    
    wifiAPMode = true;
    wifiConnected = true;
    
    DEBUG_PRINT("WiFi AP запущена. IP: ");
    DEBUG_PRINTLN(WiFi.softAPIP());
}

void MicroBoxRobot::connectWiFiDHCP(const char* ssid, const char* password) {
    DEBUG_PRINTLN("Подключение к WiFi сети...");
    
    WiFi.mode(WIFI_STA);
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
    } else {
        DEBUG_PRINTLN();
        DEBUG_PRINTLN("Не удалось подключиться к WiFi, запуск AP режима");
        startWiFiAP();
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

void MicroBoxRobot::startCameraServer() {
    DEBUG_PRINTLN("Запуск камера-сервера...");
    
    // Обработчик для стрима камеры
    server->on("/stream", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response = request->beginChunkedResponse("multipart/x-mixed-replace; boundary=frame",
            [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
                camera_fb_t *fb = esp_camera_fb_get();
                if (!fb) {
                    DEBUG_PRINTLN("Ошибка захвата кадра");
                    return 0;
                }
                
                static bool header_sent = false;
                size_t len = 0;
                
                if (!header_sent) {
                    len = snprintf((char*)buffer, maxLen,
                        "\\r\\n--frame\\r\\n"
                        "Content-Type: image/jpeg\\r\\n"
                        "Content-Length: %u\\r\\n\\r\\n", fb->len);
                    header_sent = true;
                } else {
                    if (index < fb->len) {
                        len = min(maxLen, fb->len - index);
                        memcpy(buffer, fb->buf + index, len);
                    }
                    
                    if (index + len >= fb->len) {
                        header_sent = false;
                        esp_camera_fb_return(fb);
                    }
                }
                
                return len;
            });
        
        request->send(response);
    });
}

void MicroBoxRobot::initMotors() {
    DEBUG_PRINTLN("Инициализация моторов...");
    
    // Настройка PWM каналов для моторов
    ledcSetup(MOTOR_PWM_CHANNEL_LF, MOTOR_PWM_FREQ, MOTOR_PWM_RESOLUTION);
    ledcSetup(MOTOR_PWM_CHANNEL_LR, MOTOR_PWM_FREQ, MOTOR_PWM_RESOLUTION);
    ledcSetup(MOTOR_PWM_CHANNEL_RF, MOTOR_PWM_FREQ, MOTOR_PWM_RESOLUTION);
    ledcSetup(MOTOR_PWM_CHANNEL_RR, MOTOR_PWM_FREQ, MOTOR_PWM_RESOLUTION);
    
    // Привязка каналов к пинам
    ledcAttachPin(MOTOR_LEFT_FWD_PIN, MOTOR_PWM_CHANNEL_LF);
    ledcAttachPin(MOTOR_LEFT_REV_PIN, MOTOR_PWM_CHANNEL_LR);
    ledcAttachPin(MOTOR_RIGHT_FWD_PIN, MOTOR_PWM_CHANNEL_RF);
    ledcAttachPin(MOTOR_RIGHT_REV_PIN, MOTOR_PWM_CHANNEL_RR);
    
    // Остановка моторов
    stopMotors();
    
    DEBUG_PRINTLN("Моторы инициализированы");
}

void MicroBoxRobot::initLEDs() {
    DEBUG_PRINTLN("Инициализация светодиодов...");
    
    pixels = new Adafruit_NeoPixel(NEOPIXEL_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
    pixels->begin();
    pixels->setBrightness(LED_BRIGHTNESS_DEFAULT);
    clearLEDs();
    
    DEBUG_PRINTLN("Светодиоды инициализированы");
}

void MicroBoxRobot::initBuzzer() {
    DEBUG_PRINTLN("Инициализация бузера...");
    
    // Настройка PWM для бузера
    ledcSetup(BUZZER_CHANNEL, 1000, BUZZER_RESOLUTION); // Базовая частота 1кГц
    ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL);
    
    stopBuzzer();
    
    DEBUG_PRINTLN("Бузер инициализирован");
}

void MicroBoxRobot::initWebServer() {
    DEBUG_PRINTLN("Инициализация веб-сервера...");
    
    server = new AsyncWebServer(WIFI_PORT);
    
    // Главная страница
    server->on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleRoot(request);
    });
    
    // API команды
    server->on("/command", HTTP_POST, [this](AsyncWebServerRequest *request) {
        this->handleCommand(request);
    });
    
    // Статические ресурсы
    #ifdef EMBEDDED_RESOURCES_H
    server->on("/styles.css", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response = request->beginResponse_P(200, "text/css", stylesCss, stylesCss_len);
        response->addHeader("Cache-Control", "max-age=86400");
        request->send(response);
    });
    
    server->on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response = request->beginResponse_P(200, "application/javascript", scriptJs, scriptJs_len);
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
    #ifdef EMBEDDED_RESOURCES_H
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", indexHtml, indexHtml_len);
    #else
    // Fallback HTML если ресурсы не встроены
    String html = "<!DOCTYPE html><html><head><title>МикроББокс</title></head><body>";
    html += "<h1>МикроББокс</h1>";
    html += "<p>Добро пожаловать в систему управления МикроББокс!</p>";
    html += "<p>Видео стрим: <img src='/stream' style='max-width:100%'></p>";
    html += "<p>Статические ресурсы не загружены. Пожалуйста, пересоберите проект.</p>";
    html += "</body></html>";
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", html);
    #endif
    
    response->addHeader("Cache-Control", "no-cache");
    request->send(response);
}

void MicroBoxRobot::handleCommand(AsyncWebServerRequest *request) {
    // Обработка JSON команд от веб-интерфейса
    if (request->hasParam("body", true)) {
        String body = request->getParam("body", true)->value();
        
        // Здесь должен быть парсинг JSON и выполнение команд
        // Для простоты используем простые строковые команды
        
        if (body.indexOf("move") >= 0) {
            // Парсинг команды движения
            // Например: {"command":"move","left":50,"right":50}
            // Простой парсинг для демонстрации
            int leftSpeed = 0, rightSpeed = 0;
            
            int leftPos = body.indexOf("\"left\":");
            int rightPos = body.indexOf("\"right\":");
            
            if (leftPos >= 0) {
                leftSpeed = body.substring(leftPos + 7).toInt();
            }
            if (rightPos >= 0) {
                rightSpeed = body.substring(rightPos + 8).toInt();
            }
            
            setMotorSpeed(leftSpeed, rightSpeed);
        }
        else if (body.indexOf("flashlight") >= 0) {
            // Переключение фонарика - используем передний неопиксель (индекс 0)
            static bool flashlightState = false;
            flashlightState = !flashlightState;
            
            if (flashlightState) {
                // Включаем передний светодиод белым цветом на полную яркость
                pixels->setPixelColor(0, pixels->Color(255, 255, 255));
            } else {
                // Выключаем передний светодиод
                pixels->setPixelColor(0, pixels->Color(0, 0, 0));
            }
            pixels->show();
        }
        else if (body.indexOf("horn") >= 0) {
            // Сигнал
            if (body.indexOf("true") >= 0) {
                playTone(800, 0); // Непрерывный сигнал
            } else {
                stopBuzzer();
            }
        }
        else if (body.indexOf("setEffectMode") >= 0) {
            // Переключение режима эффектов
            if (body.indexOf("police") >= 0) {
                setEffectMode(EffectMode::POLICE);
            } else if (body.indexOf("fire") >= 0) {
                setEffectMode(EffectMode::FIRE);
            } else if (body.indexOf("ambulance") >= 0) {
                setEffectMode(EffectMode::AMBULANCE);
            } else {
                setEffectMode(EffectMode::NORMAL);
            }
        }
        else if (body.indexOf("enterUpdateMode") >= 0) {
            // Вход в режим обновления прошивки
            enterUpdateMode();
        }
    }
    
    request->send(200, "application/json", "{\"status\":\"ok\"}");
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
    
    // Преобразование в PWM значения (0-65535 для 16-битного разрешения)
    int leftPWM = map(abs(leftSpeed), 0, 100, 0, 65535);
    int rightPWM = map(abs(rightSpeed), 0, 100, 0, 65535);
    
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
        ledcWrite(MOTOR_PWM_CHANNEL_RF, rightPWM);
        ledcWrite(MOTOR_PWM_CHANNEL_RR, 0);
    } else if (rightSpeed < 0) {
        ledcWrite(MOTOR_PWM_CHANNEL_RF, 0);
        ledcWrite(MOTOR_PWM_CHANNEL_RR, rightPWM);
    } else {
        ledcWrite(MOTOR_PWM_CHANNEL_RF, 0);
        ledcWrite(MOTOR_PWM_CHANNEL_RR, 0);
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
            // Синие светодиоды сзади, красные спереди
            setLEDColor(0, pixels->Color(0, 0, 255));   // Задний левый - синий
            setLEDColor(1, pixels->Color(0, 0, 255));   // Задний правый - синий
            setLEDColor(2, pixels->Color(255, 0, 0));   // Передний - красный
            
            // Звук сирены
            playTone(800, 0);
        } else {
            // Красные светодиоды сзади, синие спереди
            setLEDColor(0, pixels->Color(255, 0, 0));   // Задний левый - красный
            setLEDColor(1, pixels->Color(255, 0, 0));   // Задний правый - красный
            setLEDColor(2, pixels->Color(0, 0, 255));   // Передний - синий
            
            // Другой тон сирены
            playTone(1000, 0);
        }
        
        updateLEDs();
        lastToggle = currentTime;
    }
}

void MicroBoxRobot::playFireEffect() {
    static unsigned long lastToggle = 0;
    unsigned long currentTime = millis();
    
    if (currentTime - lastToggle > 200) { // Быстрое мигание
        effectState = !effectState;
        
        if (effectState) {
            setAllLEDs(pixels->Color(255, 0, 0)); // Красный
            playTone(900, 0);
        } else {
            setAllLEDs(pixels->Color(255, 165, 0)); // Оранжевый
            playTone(1100, 0);
        }
        
        updateLEDs();
        lastToggle = currentTime;
    }
}

void MicroBoxRobot::playAmbulanceEffect() {
    static unsigned long lastToggle = 0;
    unsigned long currentTime = millis();
    
    if (currentTime - lastToggle > 300) { // Медленнее полиции
        effectState = !effectState;
        
        if (effectState) {
            setAllLEDs(pixels->Color(255, 255, 255)); // Белый
            playTone(750, 0);
        } else {
            setAllLEDs(pixels->Color(255, 0, 0)); // Красный
            playTone(1050, 0);
        }
        
        updateLEDs();
        lastToggle = currentTime;
    }
}

void MicroBoxRobot::playMovementAnimation() {
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
}

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
        clearLEDs();
        stopBuzzer();
        
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