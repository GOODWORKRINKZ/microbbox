#include "LinerRobot.h"

#ifdef TARGET_LINER

#include "MX1508MotorController.h"
#include "hardware_config.h"
#include <esp_camera.h>

LinerRobot::LinerRobot() :
    BaseRobot(),
#ifdef FEATURE_NEOPIXEL
    pixels_(nullptr),
    currentEffectMode_(EffectMode::NORMAL),
#endif
    currentMode_(Mode::MANUAL),
    buttonPressed_(false),
    lastButtonCheck_(0),
    pidError_(0.0f),
    pidLastError_(0.0f),
    pidIntegral_(0.0f),
    targetThrottlePWM_(1500),
    targetSteeringPWM_(1500)
{
    DEBUG_PRINTLN("Создание LinerRobot");
}

LinerRobot::~LinerRobot() {
    shutdown();
}

bool LinerRobot::initSpecificComponents() {
    DEBUG_PRINTLN("=== Инициализация компонентов Liner робота ===");
    
    // Инициализация моторов
    if (!initMotors()) {
        DEBUG_PRINTLN("ОШИБКА: Не удалось инициализировать моторы");
        return false;
    }
    
#ifdef FEATURE_NEOPIXEL
    // Инициализация LED для индикации
    if (!initLEDs()) {
        DEBUG_PRINTLN("ПРЕДУПРЕЖДЕНИЕ: Не удалось инициализировать LED");
    }
#endif
    
#ifdef FEATURE_BUTTON
    // Инициализация кнопки
    DEBUG_PRINTLN("FEATURE_BUTTON определен, инициализируем кнопку...");
    if (!initButton()) {
        DEBUG_PRINTLN("ПРЕДУПРЕЖДЕНИЕ: Не удалось инициализировать кнопку");
    } else {
        DEBUG_PRINTLN("✓ Кнопка успешно инициализирована!");
    }
#else
    DEBUG_PRINTLN("ВНИМАНИЕ: FEATURE_BUTTON НЕ определен! Кнопка не будет работать!");
#endif
    
#ifdef FEATURE_NEOPIXEL
    // Применяем сохраненный эффект
    if (wifiSettings_) {
        currentEffectMode_ = static_cast<EffectMode>(wifiSettings_->getEffectMode());
        DEBUG_PRINT("Применен сохраненный эффект: ");
        DEBUG_PRINTLN(wifiSettings_->getEffectMode());
    }
#endif
    
    DEBUG_PRINTLN("=== Liner робот готов ===");
    return true;
}

void LinerRobot::updateSpecificComponents() {
    // ДИАГНОСТИКА: Выводим текущий режим каждые 5 секунд
    static unsigned long lastModePrint = 0;
    unsigned long now = millis();
    if (now - lastModePrint > 5000) {
        DEBUG_PRINT("[MODE_DIAG] Текущий режим: ");
        DEBUG_PRINTLN(currentMode_ == Mode::AUTONOMOUS ? "АВТОНОМНЫЙ (следование по линии)" : "РУЧНОЙ");
        lastModePrint = now;
    }
    
    // Обновление кнопки
#ifdef FEATURE_BUTTON
    updateButton();
#endif
    
    // Обновление в зависимости от режима
    if (currentMode_ == Mode::AUTONOMOUS) {
        updateLineFollowing();
    } else {
        updateMotors();
    }
    
    // Обновление индикации
#ifdef FEATURE_NEOPIXEL
    updateStatusLED();
#endif
    
    // Обновление контроллера моторов
    if (motorController_) {
        motorController_->update();
    }
}

void LinerRobot::shutdownSpecificComponents() {
#ifdef FEATURE_NEOPIXEL
    if (pixels_) {
        pixels_->clear();
        pixels_->show();
        delete pixels_;
        pixels_ = nullptr;
    }
#endif
}

void LinerRobot::setupWebHandlers(AsyncWebServer* server) {
    DEBUG_PRINTLN("Настройка веб-обработчиков для Liner робота");
    
    // Команды управления
    server->on("/cmd", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleCommand(request);
    });
    
    // Статус
    server->on("/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleStatus(request);
    });
    
    // API: Тип робота
    server->on("/api/robot-type", HTTP_GET, [this](AsyncWebServerRequest* request) {
        String json = "{\"type\":\"liner\",\"name\":\"MicroBox Liner\"}";
        request->send(200, "application/json", json);
    });
    
    // Специфичные для Liner endpoints
    // (общие /api/settings/*, /api/restart уже в BaseRobot)
}
bool LinerRobot::initMotors() {
    DEBUG_PRINTLN("Инициализация моторов...");
    
#ifdef FEATURE_MOTORS
    motorController_ = new MX1508MotorController();
    if (!motorController_->init()) {
        DEBUG_PRINTLN("ОШИБКА: Не удалось инициализировать контроллер моторов");
        return false;
    }
    
    // Передаем WiFi настройки для применения инвертирования моторов
    if (wifiSettings_) {
        static_cast<MX1508MotorController*>(motorController_)->setWiFiSettings(wifiSettings_);
    }
    
    DEBUG_PRINTLN("Моторы инициализированы");
    return true;
#else
    return true;
#endif
}

bool LinerRobot::initLEDs() {
#ifdef FEATURE_NEOPIXEL
    DEBUG_PRINTLN("Инициализация NeoPixel LED...");
    
    pixels_ = new Adafruit_NeoPixel(NEOPIXEL_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
    pixels_->begin();
    pixels_->setBrightness(LED_BRIGHTNESS_DEFAULT);
    pixels_->clear();
    pixels_->show();
    
    DEBUG_PRINTLN("NeoPixel LED инициализированы");
    return true;
#else
    return true;
#endif
}

bool LinerRobot::initButton() {
#ifdef FEATURE_BUTTON
    DEBUG_PRINTLN("Инициализация кнопки...");
    
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    
    // Читаем начальное состояние
    bool initialState = digitalRead(BUTTON_PIN);
    DEBUG_PRINT("Кнопка на пине ");
    DEBUG_PRINT(BUTTON_PIN);
    DEBUG_PRINT(", начальное состояние: ");
    DEBUG_PRINTLN(initialState == HIGH ? "HIGH (не нажата)" : "LOW (нажата)");
    DEBUG_PRINTLN("Кнопка настроена с INPUT_PULLUP, нажатие = LOW (замыкание на GND)");
    
    DEBUG_PRINTLN("Кнопка инициализирована");
    return true;
#else
    return true;
#endif
}

void LinerRobot::updateButton() {
#ifdef FEATURE_BUTTON
    unsigned long now = millis();
    if (now - lastButtonCheck_ < BUTTON_DEBOUNCE_MS) {
        return; // Антидребезг
    }
    lastButtonCheck_ = now;
    
    // Читаем состояние кнопки
    // HIGH = не нажата (подтянута к VCC через pull-up)
    // LOW = нажата (замкнута на GND)
    int rawPinValue = digitalRead(BUTTON_PIN);
    bool currentButtonState = (rawPinValue == LOW);
    
    // ДИАГНОСТИКА: Выводим состояние каждые 2 секунды
    static unsigned long lastDiagPrint = 0;
    if (now - lastDiagPrint > 2000) {
        DEBUG_PRINT("[BUTTON_DIAG] Pin ");
        DEBUG_PRINT(BUTTON_PIN);
        DEBUG_PRINT(" = ");
        DEBUG_PRINT(rawPinValue);
        DEBUG_PRINT(" (");
        DEBUG_PRINT(rawPinValue == HIGH ? "HIGH/не_нажата" : "LOW/нажата");
        DEBUG_PRINT("), buttonPressed_ = ");
        DEBUG_PRINTLN(buttonPressed_ ? "true" : "false");
        lastDiagPrint = now;
    }
    
    // Детектируем переход из не нажатого состояния в нажатое (фронт нажатия)
    if (currentButtonState && !buttonPressed_) {
        // Кнопка только что нажата (переход с HIGH на LOW)
        buttonPressed_ = true;
        onButtonPressed();
        DEBUG_PRINTLN("Кнопка: переход в НАЖАТО, вызов onButtonPressed()");
    } else if (!currentButtonState && buttonPressed_) {
        // Кнопка отпущена (переход с LOW на HIGH)
        buttonPressed_ = false;
        DEBUG_PRINTLN("Кнопка: переход в ОТПУЩЕНО");
    }
#endif
}

void LinerRobot::onButtonPressed() {
    DEBUG_PRINTLN("==================================================");
    DEBUG_PRINTLN("КНОПКА НАЖАТА!");
    DEBUG_PRINT("Текущий режим: ");
    DEBUG_PRINTLN(currentMode_ == Mode::MANUAL ? "РУЧНОЙ" : "АВТОНОМНЫЙ");
    
    // Переключение режима
    if (currentMode_ == Mode::MANUAL) {
        currentMode_ = Mode::AUTONOMOUS;
        DEBUG_PRINTLN(">>> ПЕРЕХОД В АВТОНОМНЫЙ РЕЖИМ <<<");
        DEBUG_PRINTLN(">>> НАЧАТО АВТОСЛЕДОВАНИЕ ПО ЛИНИИ <<<");
        
        // Сброс PID контроллера
        pidError_ = 0.0f;
        pidLastError_ = 0.0f;
        pidIntegral_ = 0.0f;
        DEBUG_PRINTLN("PID контроллер сброшен");
    } else {
        currentMode_ = Mode::MANUAL;
        DEBUG_PRINTLN(">>> ПЕРЕХОД В РУЧНОЙ РЕЖИМ <<<");
        DEBUG_PRINTLN(">>> АВТОСЛЕДОВАНИЕ ОСТАНОВЛЕНО <<<");
        
        // Остановка моторов
        if (motorController_) {
            motorController_->stop();
            DEBUG_PRINTLN("Моторы остановлены");
        }
    }
    DEBUG_PRINTLN("==================================================");
}

void LinerRobot::updateLineFollowing() {
#ifdef FEATURE_LINE_FOLLOWING
    // Определение позиции линии
    float linePosition = detectLinePosition();
    
    // Применение PID управления
    applyPIDControl(linePosition);
#endif
}

float LinerRobot::detectLinePosition() {
    // Захват кадра с камеры
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
        DEBUG_PRINTLN("ОШИБКА: Не удалось получить кадр с камеры");
        return 0.0f;
    }
    
    // Проверка формата кадра
    if (fb->format != PIXFORMAT_GRAYSCALE) {
        DEBUG_PRINTLN("ПРЕДУПРЕЖДЕНИЕ: Камера не в режиме GRAYSCALE!");
        esp_camera_fb_return(fb);
        return 0.0f;
    }
    
    // Проверка размера кадра
    if (fb->width != LINE_CAMERA_WIDTH || fb->height != LINE_CAMERA_HEIGHT) {
        DEBUG_PRINTF("ПРЕДУПРЕЖДЕНИЕ: Размер кадра %dx%d, ожидалось %dx%d\n", 
                    fb->width, fb->height, LINE_CAMERA_WIDTH, LINE_CAMERA_HEIGHT);
        esp_camera_fb_return(fb);
        return 0.0f;
    }
    
    // Анализ изображения 96x96 grayscale
    // Ищем линию в нижней части изображения
    int width = fb->width;
    int height = fb->height;
    int scanLine = height * 3 / 4; // Сканируем на 75% высоты
    
    uint8_t* img = fb->buf;
    
    // Подсчет суммы позиций белых пикселей
    float sumPosition = 0.0f;
    int count = 0;
    
    for (int x = 0; x < width; x++) {
        int idx = scanLine * width + x;
        uint8_t pixel = img[idx];
        
        if (pixel > LINE_THRESHOLD) {
            // Белый пиксель (линия)
            sumPosition += (float)x;
            count++;
        }
    }
    
    esp_camera_fb_return(fb);
    
    if (count == 0) {
        // Линия не найдена
        DEBUG_PRINTLN("ПРЕДУПРЕЖДЕНИЕ: Линия не обнаружена");
        return 0.0f;
    }
    
    // Средняя позиция линии
    float avgPosition = sumPosition / (float)count;
    
    // Нормализация от -1.0 (левый край) до 1.0 (правый край)
    float normalized = (avgPosition / (float)width) * 2.0f - 1.0f;
    
    return normalized;
}

void LinerRobot::applyPIDControl(float linePosition) {
    // PID расчет
    pidError_ = linePosition;
    pidIntegral_ += pidError_;
    float derivative = pidError_ - pidLastError_;
    pidLastError_ = pidError_;
    
    // PID формула
    float control = LINE_PID_KP * pidError_ + 
                   LINE_PID_KI * pidIntegral_ + 
                   LINE_PID_KD * derivative;
    
    // Ограничение интегральной составляющей (anti-windup)
    pidIntegral_ = constrain(pidIntegral_, -100.0f, 100.0f);
    
    // Применение к моторам
    int baseSpeed = LINE_BASE_SPEED;
    int steering = (int)(control * 100.0f);
    
    int leftSpeed = baseSpeed - steering;
    int rightSpeed = baseSpeed + steering;
    
    if (motorController_) {
        motorController_->setSpeed(leftSpeed, rightSpeed);
    }
    
    DEBUG_PRINTF("Line: %.2f, Control: %.2f, L: %d, R: %d\n", 
                 linePosition, control, leftSpeed, rightSpeed);
}

void LinerRobot::updateMotors() {
    if (!motorController_ || !motorController_->isInitialized()) {
        return;
    }
    
    // Применяем значения PWM только если они изменились или если сработал watchdog
    static int lastAppliedThrottle = 1500;
    static int lastAppliedSteering = 1500;
    
    // Если watchdog остановил моторы, сбрасываем целевые значения в нейтральное положение
    // чтобы предотвратить повторное применение старых команд движения
    if (motorController_->wasWatchdogTriggered()) {
        targetThrottlePWM_ = 1500;
        targetSteeringPWM_ = 1500;
    }
    
    if (targetThrottlePWM_ != lastAppliedThrottle || targetSteeringPWM_ != lastAppliedSteering) {
        motorController_->setMotorPWM(targetThrottlePWM_, targetSteeringPWM_);
        lastAppliedThrottle = targetThrottlePWM_;
        lastAppliedSteering = targetSteeringPWM_;
    }
}

void LinerRobot::handleMotorCommand(int throttlePWM, int steeringPWM) {
    // В ручном режиме обновляем целевые значения PWM
    if (currentMode_ == Mode::MANUAL) {
        targetThrottlePWM_ = constrain(throttlePWM, 1000, 2000);
        targetSteeringPWM_ = constrain(steeringPWM, 1000, 2000);
        
        // ВАЖНО: Обновляем timestamp СРАЗУ при получении команды
        // Это предотвращает срабатывание watchdog когда команды приходят с одинаковыми значениями
        if (motorController_) {
            motorController_->updateCommandTime();
        }
    }
    // В автономном режиме игнорируем команды управления
}

void LinerRobot::updateStatusLED() {
#ifdef FEATURE_NEOPIXEL
    if (!pixels_) return;
    
    // Индикация режима
    if (currentMode_ == Mode::AUTONOMOUS) {
        // Автономный режим - зеленый
        for (int i = 0; i < NEOPIXEL_COUNT; i++) {
            pixels_->setPixelColor(i, pixels_->Color(0, 255, 0));
        }
    } else {
        // Ручной режим - синий
        for (int i = 0; i < NEOPIXEL_COUNT; i++) {
            pixels_->setPixelColor(i, pixels_->Color(0, 0, 255));
        }
    }
    pixels_->show();
#endif
}

void LinerRobot::handleCommand(AsyncWebServerRequest* request) {
    if (request->hasParam("mode")) {
        String mode = request->getParam("mode")->value();
        if (mode == "auto") {
            currentMode_ = Mode::AUTONOMOUS;
            pidError_ = 0.0f;
            pidLastError_ = 0.0f;
            pidIntegral_ = 0.0f;
        } else if (mode == "manual") {
            currentMode_ = Mode::MANUAL;
            if (motorController_) {
                motorController_->stop();
            }
        }
        request->send(200, "text/plain", "OK");
    } else if (request->hasParam("throttle") && request->hasParam("steering")) {
        int throttle = request->getParam("throttle")->value().toInt();
        int steering = request->getParam("steering")->value().toInt();
        
        targetThrottlePWM_ = constrain(throttle, 1000, 2000);
        targetSteeringPWM_ = constrain(steering, 1000, 2000);
        
        request->send(200, "text/plain", "OK");
    } else if (request->hasParam("effect")) {
        int effect = request->getParam("effect")->value().toInt();
#ifdef FEATURE_NEOPIXEL
        currentEffectMode_ = static_cast<EffectMode>(constrain(effect, 0, 4));
#endif
        request->send(200, "text/plain", "OK");
    } else {
        request->send(400, "text/plain", "Bad Request");
    }
}

void LinerRobot::handleStatus(AsyncWebServerRequest* request) {
    String json = "{";
    json += "\"mode\":\"" + String(currentMode_ == Mode::AUTONOMOUS ? "autonomous" : "manual") + "\",";
    json += "\"pid_error\":" + String(pidError_, 2);
    json += "}";
    
    request->send(200, "application/json", json);
}

#endif // TARGET_LINER
