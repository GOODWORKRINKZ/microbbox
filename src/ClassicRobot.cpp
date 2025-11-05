#include "ClassicRobot.h"

#ifdef TARGET_CLASSIC

#include "MX1508MotorController.h"
#include "hardware_config.h"
#include "embedded_resources.h"

ClassicRobot::ClassicRobot() :
    BaseRobot(),
#ifdef FEATURE_NEOPIXEL
    pixels_(nullptr),
#endif
#if defined(FEATURE_NEOPIXEL) || defined(FEATURE_BUZZER)
    currentEffectMode_(EffectMode::NORMAL),
    lastEffectUpdate_(0),
    effectState_(false),
#endif
    currentControlMode_(ControlMode::DIFFERENTIAL),
    targetThrottlePWM_(1500),
    targetSteeringPWM_(1500)
{
    DEBUG_PRINTLN("Создание ClassicRobot");
}

ClassicRobot::~ClassicRobot() {
    shutdown();
}

bool ClassicRobot::initSpecificComponents() {
    DEBUG_PRINTLN("=== Инициализация компонентов Classic робота ===");
    
    // Инициализация моторов
    if (!initMotors()) {
        DEBUG_PRINTLN("ОШИБКА: Не удалось инициализировать моторы");
        return false;
    }
    
#ifdef FEATURE_NEOPIXEL
    // Инициализация LED
    if (!initLEDs()) {
        DEBUG_PRINTLN("ПРЕДУПРЕЖДЕНИЕ: Не удалось инициализировать LED");
    }
#endif
    
#ifdef FEATURE_BUZZER
    // Инициализация бузера
    if (!initBuzzer()) {
        DEBUG_PRINTLN("ПРЕДУПРЕЖДЕНИЕ: Не удалось инициализировать бузер");
    }
#endif
    
#if defined(FEATURE_NEOPIXEL) || defined(FEATURE_BUZZER)
    // Применяем сохраненный эффект
    if (wifiSettings_) {
        currentEffectMode_ = static_cast<EffectMode>(wifiSettings_->getEffectMode());
        DEBUG_PRINT("Применен сохраненный эффект: ");
        DEBUG_PRINTLN(wifiSettings_->getEffectMode());
    }
#endif
    
    DEBUG_PRINTLN("=== Classic робот готов ===");
    return true;
}

void ClassicRobot::updateSpecificComponents() {
    // Обновление моторов
    updateMotors();
    
    // Обновление эффектов
#if defined(FEATURE_NEOPIXEL) || defined(FEATURE_BUZZER)
    updateEffects();
#endif
    
    // Обновление контроллера моторов
    if (motorController_) {
        motorController_->update();
    }
}

void ClassicRobot::shutdownSpecificComponents() {
#ifdef FEATURE_NEOPIXEL
    if (pixels_) {
        clearLEDs();
        delete pixels_;
        pixels_ = nullptr;
    }
#endif
    
#ifdef FEATURE_BUZZER
    stopBuzzer();
#endif
}

void ClassicRobot::setupWebHandlers(AsyncWebServer* server) {
    DEBUG_PRINTLN("Настройка веб-обработчиков для Classic робота");
    
    // Команды управления
    server->on("/cmd", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleCommand(request);
    });
    
    // API: Тип робота
    server->on("/api/robot-type", HTTP_GET, [this](AsyncWebServerRequest* request) {
        String json = "{\"type\":\"classic\",\"name\":\"MicroBox Classic\"}";
        request->send(200, "application/json", json);
    });
    
    // Фонарик (камера LED)
    server->on("/flashlight", HTTP_GET, [](AsyncWebServerRequest* request) {
        // TODO: Реализовать управление камерой LED если поддерживается
        request->send(501, "text/plain", "Not Implemented");
    });
    
    // Звуковой сигнал
    server->on("/horn", HTTP_GET, [this](AsyncWebServerRequest* request) {
#ifdef FEATURE_BUZZER
        playTone(1000, 200); // 1kHz, 200ms
        request->send(200, "text/plain", "OK");
#else
        request->send(501, "text/plain", "Not Implemented");
#endif
    });
    
    // Специфичные для Classic endpoints
    // (общие /api/settings/*, /api/restart уже в BaseRobot)
}

bool ClassicRobot::initMotors() {
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
    DEBUG_PRINTLN("Моторы не включены в сборку");
    return true;
#endif
}

bool ClassicRobot::initLEDs() {
#ifdef FEATURE_NEOPIXEL
    DEBUG_PRINTLN("Инициализация NeoPixel LED...");
    
    pixels_ = new Adafruit_NeoPixel(NEOPIXEL_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
    pixels_->begin();
    pixels_->setBrightness(LED_BRIGHTNESS_DEFAULT);
    clearLEDs();
    
    DEBUG_PRINTLN("NeoPixel LED инициализированы");
    return true;
#else
    return true;
#endif
}

bool ClassicRobot::initBuzzer() {
#ifdef FEATURE_BUZZER
    DEBUG_PRINTLN("Инициализация бузера...");
    
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    
    ledcSetup(BUZZER_CHANNEL, 2000, BUZZER_RESOLUTION);
    ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL);
    ledcWrite(BUZZER_CHANNEL, 0);
    
    DEBUG_PRINTLN("Бузер инициализирован");
    return true;
#else
    return true;
#endif
}

void ClassicRobot::updateMotors() {
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

void ClassicRobot::handleMotorCommand(int throttlePWM, int steeringPWM) {
    // Обновляем целевые значения PWM (быстро, без блокировки)
    targetThrottlePWM_ = constrain(throttlePWM, 1000, 2000);
    targetSteeringPWM_ = constrain(steeringPWM, 1000, 2000);
    
    // ВАЖНО: Обновляем timestamp СРАЗУ при получении команды
    // Это предотвращает срабатывание watchdog когда команды приходят с одинаковыми значениями
    if (motorController_) {
        motorController_->updateCommandTime();
    }
}

void ClassicRobot::updateEffects() {
#if defined(FEATURE_NEOPIXEL) || defined(FEATURE_BUZZER)
    unsigned long now = millis();
    
    // Обновление эффектов с определенной частотой
    if (now - lastEffectUpdate_ < 100) {
        return; // Пока рано обновлять
    }
    lastEffectUpdate_ = now;
    
    switch (currentEffectMode_) {
        case EffectMode::POLICE:
            playPoliceEffect();
            break;
        case EffectMode::FIRE:
            playFireEffect();
            break;
        case EffectMode::AMBULANCE:
            playAmbulanceEffect();
            break;
        case EffectMode::TERMINATOR:
            playTerminatorEffect();
            break;
        case EffectMode::NORMAL:
        default:
            playMovementAnimation();
            break;
    }
#endif
}

void ClassicRobot::handleCommand(AsyncWebServerRequest* request) {
    // Обработка команд управления
    if (request->hasParam("throttle") && request->hasParam("steering")) {
        int throttle = request->getParam("throttle")->value().toInt();
        int steering = request->getParam("steering")->value().toInt();
        
        targetThrottlePWM_ = constrain(throttle, 1000, 2000);
        targetSteeringPWM_ = constrain(steering, 1000, 2000);
        
        request->send(200, "text/plain", "OK");
    } else if (request->hasParam("effect")) {
        int effect = request->getParam("effect")->value().toInt();
#if defined(FEATURE_NEOPIXEL) || defined(FEATURE_BUZZER)
        currentEffectMode_ = static_cast<EffectMode>(constrain(effect, 0, 4));
#endif
        request->send(200, "text/plain", "OK");
    } else {
        request->send(400, "text/plain", "Bad Request");
    }
}

#ifdef FEATURE_NEOPIXEL
void ClassicRobot::setLEDColor(int ledIndex, uint32_t color) {
    if (pixels_ && ledIndex >= 0 && ledIndex < NEOPIXEL_COUNT) {
        pixels_->setPixelColor(ledIndex, color);
    }
}

void ClassicRobot::setAllLEDs(uint32_t color) {
    if (pixels_) {
        for (int i = 0; i < NEOPIXEL_COUNT; i++) {
            pixels_->setPixelColor(i, color);
        }
    }
}

void ClassicRobot::clearLEDs() {
    if (pixels_) {
        pixels_->clear();
        pixels_->show();
    }
}

void ClassicRobot::updateLEDs() {
    if (pixels_) {
        pixels_->show();
    }
}

void ClassicRobot::playPoliceEffect() {
    if (!pixels_) return;
    
    effectState_ = !effectState_;
    if (effectState_) {
        setLEDColor(0, pixels_->Color(255, 0, 0)); // Красный
        setLEDColor(1, pixels_->Color(0, 0, 0));   // Выкл
        setLEDColor(2, pixels_->Color(0, 0, 0));   // Выкл
    } else {
        setLEDColor(0, pixels_->Color(0, 0, 0));   // Выкл
        setLEDColor(1, pixels_->Color(0, 0, 255)); // Синий
        setLEDColor(2, pixels_->Color(0, 0, 0));   // Выкл
    }
    updateLEDs();
}

void ClassicRobot::playFireEffect() {
    if (!pixels_) return;
    
    effectState_ = !effectState_;
    uint32_t color = effectState_ ? pixels_->Color(255, 0, 0) : pixels_->Color(255, 165, 0);
    setAllLEDs(color);
    updateLEDs();
}

void ClassicRobot::playAmbulanceEffect() {
    if (!pixels_) return;
    
    effectState_ = !effectState_;
    uint32_t color = effectState_ ? pixels_->Color(255, 0, 0) : pixels_->Color(255, 255, 255);
    setAllLEDs(color);
    updateLEDs();
}

void ClassicRobot::playTerminatorEffect() {
    if (!pixels_) return;
    
    setAllLEDs(pixels_->Color(255, 0, 0)); // Красный HUD
    updateLEDs();
}

void ClassicRobot::playMovementAnimation() {
    if (!pixels_) return;
    
    // Простая анимация в зависимости от движения
    int leftSpeed, rightSpeed;
    if (motorController_) {
        motorController_->getCurrentSpeed(leftSpeed, rightSpeed);
        
        if (leftSpeed > 0 || rightSpeed > 0) {
            // Движение вперед - зеленый
            setAllLEDs(pixels_->Color(0, 255, 0));
        } else if (leftSpeed < 0 || rightSpeed < 0) {
            // Движение назад - красный
            setAllLEDs(pixels_->Color(255, 0, 0));
        } else {
            // Стоим - синий
            setAllLEDs(pixels_->Color(0, 0, 255));
        }
        updateLEDs();
    }
}
#endif

#ifdef FEATURE_BUZZER
void ClassicRobot::playTone(int frequency, int duration) {
    ledcWriteTone(BUZZER_CHANNEL, frequency);
    delay(duration);
    ledcWriteTone(BUZZER_CHANNEL, 0);
}

void ClassicRobot::stopBuzzer() {
    ledcWriteTone(BUZZER_CHANNEL, 0);
}
#endif

#endif // TARGET_CLASSIC
