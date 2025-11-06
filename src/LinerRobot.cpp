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
    lineDetected_(false),
    lineNotDetectedCount_(0),
    lineEndAnimationPlayed_(false),
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
    // ДИАГНОСТИКА: Выводим текущий режим периодически
    static unsigned long lastModePrint = 0;
    if (millis() - lastModePrint > MODE_DIAG_INTERVAL_MS) {
        unsigned long now = millis();
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
    
    // Для Liner используем пониженную яркость для экономии батареи
#ifdef TARGET_LINER
    pixels_->setBrightness(LED_BRIGHTNESS_LINER_MAX);
    DEBUG_PRINT("Яркость LED установлена: ");
    DEBUG_PRINT(LED_BRIGHTNESS_LINER_MAX);
    DEBUG_PRINTLN(" (экономия батареи)");
#else
    pixels_->setBrightness(LED_BRIGHTNESS_DEFAULT);
#endif
    
    pixels_->clear();
    pixels_->show();
    
    DEBUG_PRINTLN("NeoPixel LED инициализированы");
    
    // Красивая анимация запуска
    DEBUG_PRINTLN("Запуск анимации LED...");
    playStartupAnimation();
    
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
    
    // Устанавливаем задержку перед первой проверкой кнопки
    // Это предотвращает ложные срабатывания при загрузке из-за нестабильных сигналов
    lastButtonCheck_ = millis() + BUTTON_INIT_DELAY_MS;
    DEBUG_PRINTF("Первая проверка кнопки будет через %d мс\n", BUTTON_INIT_DELAY_MS);
    
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
    
    // ДИАГНОСТИКА: Выводим состояние периодически
    static unsigned long lastDiagPrint = 0;
    if (now - lastDiagPrint > BUTTON_DIAG_INTERVAL_MS) {
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
        
        // Сброс счетчиков линии
        lineDetected_ = false;
        lineNotDetectedCount_ = 0;
        lineEndAnimationPlayed_ = false;
        
        // Анимация начала следования по линии
#ifdef FEATURE_NEOPIXEL
        playLineFollowStartAnimation();
#endif
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
        // Линия не найдена (обрыв)
        lineDetected_ = false;
        lineNotDetectedCount_++;
        
        // Если линия не найдена 10+ кадров подряд - считаем что конец линии
        if (lineNotDetectedCount_ >= 10 && !lineEndAnimationPlayed_) {
            DEBUG_PRINTLN("!!! КОНЕЦ ЛИНИИ: ОБРЫВ !!!");
            lineEndAnimationPlayed_ = true;
#ifdef FEATURE_NEOPIXEL
            playLineEndAnimation();
#endif
            // Остановка моторов
            if (motorController_) {
                motorController_->stop();
            }
        }
        
        DEBUG_PRINTLN("ПРЕДУПРЕЖДЕНИЕ: Линия не обнаружена");
        return 0.0f;
    }
    
    // Проверка на T-образное пересечение или разветвление
    // Если линия занимает больше порогового значения ширины кадра - это пересечение/разветвление
    float lineWidthPercent = (float)count / (float)width;
    if (lineWidthPercent > LINE_T_JUNCTION_THRESHOLD && !lineEndAnimationPlayed_) {
        DEBUG_PRINTF("!!! КОНЕЦ ЛИНИИ: T-ОБРАЗНОЕ ПЕРЕСЕЧЕНИЕ (ширина линии %.0f%%) !!!\n", lineWidthPercent * 100);
        lineEndAnimationPlayed_ = true;
#ifdef FEATURE_NEOPIXEL
        playLineEndAnimation();
#endif
        // Остановка моторов
        if (motorController_) {
            motorController_->stop();
        }
        
        // Возвращаем центр, чтобы не было резких движений перед остановкой
        return 0.0f;
    }
    
    // Линия найдена
    lineDetected_ = true;
    lineNotDetectedCount_ = 0;
    
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
    
    // Преобразование в PWM сигналы (1000-2000)
    // Базовая скорость движения вперед
    int baseSpeed = LINE_BASE_SPEED;  // 0-100%
    int steering = (int)(control * 100.0f);  // -100 до +100
    
    // Преобразуем baseSpeed в throttle PWM (1500 = стоп, 2000 = полный вперед)
    int throttlePWM = map(baseSpeed, 0, 100, 1500, 2000);
    
    // Преобразуем steering в steering PWM (1500 = прямо)
    int steeringPWM = map(steering, -100, 100, 1000, 2000);
    
    DEBUG_PRINTF("Line: %.2f, Control: %.2f, Throttle PWM: %d, Steering PWM: %d\n", 
                 linePosition, control, throttlePWM, steeringPWM);
    
    // Используем setMotorPWM() - это автоматически применит все настройки:
    // - Инверсию левого мотора
    // - Инверсию правого мотора
    // - Своп моторов
    // Гарантируется одинаковое поведение в ручном и автономном режимах!
    if (motorController_) {
        motorController_->setMotorPWM(throttlePWM, steeringPWM);
    }
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

void LinerRobot::playStartupAnimation() {
#ifdef FEATURE_NEOPIXEL
    if (!pixels_) return;
    
    const int leftStart = 0;    // Первые 8 LED - левая сторона
    const int leftEnd = 7;
    const int rightStart = 8;   // Следующие 8 LED - правая сторона  
    const int rightEnd = 15;
    
    // Эффект 1: Радуга слева направо и справа налево
    DEBUG_PRINTLN("Анимация: Радужная волна");
    for (int j = 0; j < 256; j += 8) {
        for (int i = leftStart; i <= leftEnd; i++) {
            uint32_t color = pixels_->ColorHSV((j + i * 32) % 65536, 255, 200);
            pixels_->setPixelColor(i, color);
        }
        for (int i = rightStart; i <= rightEnd; i++) {
            uint32_t color = pixels_->ColorHSV((j + (rightEnd - i) * 32) % 65536, 255, 200);
            pixels_->setPixelColor(i, color);
        }
        pixels_->show();
        delay(15);
    }
    
    // Эффект 2: Заполнение от центра к краям
    DEBUG_PRINTLN("Анимация: Заполнение от центра");
    pixels_->clear();
    pixels_->show();
    delay(100);
    
    // Красный цвет заполняет от центра к краям
    for (int i = 0; i < 8; i++) {
        int leftIdx = 7 - i;      // Левая сторона: от центра (7) к краю (0)
        int rightIdx = 8 + i;     // Правая сторона: от центра (8) к краю (15)
        
        pixels_->setPixelColor(leftIdx, pixels_->Color(255, 0, 0));
        pixels_->setPixelColor(rightIdx, pixels_->Color(255, 0, 0));
        pixels_->show();
        delay(60);
    }
    
    delay(200);
    
    // Эффект 3: Смена цветов синхронно
    DEBUG_PRINTLN("Анимация: Цветовая последовательность");
    uint32_t colors[] = {
        pixels_->Color(255, 0, 0),    // Красный
        pixels_->Color(255, 128, 0),  // Оранжевый
        pixels_->Color(255, 255, 0),  // Желтый
        pixels_->Color(0, 255, 0),    // Зеленый
        pixels_->Color(0, 0, 255),    // Синий
        pixels_->Color(128, 0, 255)   // Фиолетовый
    };
    
    for (int c = 0; c < 6; c++) {
        for (int i = 0; i < NEOPIXEL_COUNT; i++) {
            pixels_->setPixelColor(i, colors[c]);
        }
        pixels_->show();
        delay(150);
    }
    
    // Эффект 4: "Бегущие огни" навстречу друг другу
    DEBUG_PRINTLN("Анимация: Бегущие огни");
    for (int lap = 0; lap < 2; lap++) {
        for (int i = 0; i < 8; i++) {
            pixels_->clear();
            
            // Левая сторона: бежит слева направо (0->7)
            pixels_->setPixelColor(i, pixels_->Color(0, 255, 255));
            if (i > 0) pixels_->setPixelColor(i - 1, pixels_->Color(0, 128, 128));
            
            // Правая сторона: бежит справа налево (15->8)
            pixels_->setPixelColor(rightEnd - i, pixels_->Color(255, 0, 255));
            if (i > 0) pixels_->setPixelColor(rightEnd - i + 1, pixels_->Color(128, 0, 128));
            
            pixels_->show();
            delay(80);
        }
    }
    
    // Эффект 5: Финальная вспышка
    DEBUG_PRINTLN("Анимация: Финальная вспышка");
    for (int brightness = 0; brightness < 255; brightness += 15) {
        for (int i = 0; i < NEOPIXEL_COUNT; i++) {
            pixels_->setPixelColor(i, pixels_->Color(brightness, brightness, brightness));
        }
        pixels_->show();
        delay(10);
    }
    
    delay(100);
    
    for (int brightness = 255; brightness >= 0; brightness -= 15) {
        for (int i = 0; i < NEOPIXEL_COUNT; i++) {
            pixels_->setPixelColor(i, pixels_->Color(brightness, brightness, brightness));
        }
        pixels_->show();
        delay(10);
    }
    
    // Переход к начальному состоянию (синий = ручной режим)
    delay(200);
    for (int i = 0; i < NEOPIXEL_COUNT; i++) {
        pixels_->setPixelColor(i, pixels_->Color(0, 0, 255));
    }
    pixels_->show();
    
    DEBUG_PRINTLN("Анимация завершена!");
#endif
}

void LinerRobot::playLineFollowStartAnimation() {
#ifdef FEATURE_NEOPIXEL
    if (!pixels_) return;
    
    DEBUG_PRINTLN(">>> АНИМАЦИЯ СТАРТА СЛЕДОВАНИЯ ПО ЛИНИИ <<<");
    
    const int leftStart = 0;
    const int leftEnd = 7;
    const int rightStart = 8;
    const int rightEnd = 15;
    
    // Эффект: Зеленая волна от краев к центру (готовность к старту)
    for (int i = 0; i < 8; i++) {
        pixels_->clear();
        
        // Левая сторона: от 0 к 7
        for (int j = 0; j <= i; j++) {
            int brightness = 255 - (i - j) * 30;
            pixels_->setPixelColor(j, pixels_->Color(0, brightness, 0));
        }
        
        // Правая сторона: от 15 к 8
        for (int j = 0; j <= i; j++) {
            int brightness = 255 - (i - j) * 30;
            pixels_->setPixelColor(rightEnd - j, pixels_->Color(0, brightness, 0));
        }
        
        pixels_->show();
        delay(60);
    }
    
    // Финальная вспышка зеленым
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < NEOPIXEL_COUNT; j++) {
            pixels_->setPixelColor(j, pixels_->Color(0, 255, 0));
        }
        pixels_->show();
        delay(100);
        
        pixels_->clear();
        pixels_->show();
        delay(100);
    }
    
    DEBUG_PRINTLN("Анимация старта завершена!");
#endif
}

void LinerRobot::playLineEndAnimation() {
#ifdef FEATURE_NEOPIXEL
    if (!pixels_) return;
    
    DEBUG_PRINTLN(">>> АНИМАЦИЯ КОНЦА ЛИНИИ <<<");
    
    const int leftStart = 0;
    const int leftEnd = 7;
    const int rightStart = 8;
    const int rightEnd = 15;
    
    // Эффект 1: Красная волна - предупреждение о конце
    for (int wave = 0; wave < 3; wave++) {
        for (int i = 0; i < 8; i++) {
            pixels_->clear();
            
            // Левая сторона
            pixels_->setPixelColor(i, pixels_->Color(255, 0, 0));
            if (i > 0) pixels_->setPixelColor(i - 1, pixels_->Color(128, 0, 0));
            
            // Правая сторона
            pixels_->setPixelColor(rightStart + i, pixels_->Color(255, 0, 0));
            if (i > 0) pixels_->setPixelColor(rightStart + i - 1, pixels_->Color(128, 0, 0));
            
            pixels_->show();
            delay(50);
        }
    }
    
    // Эффект 2: Пульсация красным
    for (int pulse = 0; pulse < 5; pulse++) {
        for (int brightness = 0; brightness < 255; brightness += 20) {
            for (int i = 0; i < NEOPIXEL_COUNT; i++) {
                pixels_->setPixelColor(i, pixels_->Color(brightness, 0, 0));
            }
            pixels_->show();
            delay(15);
        }
        
        for (int brightness = 255; brightness >= 0; brightness -= 20) {
            for (int i = 0; i < NEOPIXEL_COUNT; i++) {
                pixels_->setPixelColor(i, pixels_->Color(brightness, 0, 0));
            }
            pixels_->show();
            delay(15);
        }
    }
    
    // Финал: оставить красные LED гореть
    for (int i = 0; i < NEOPIXEL_COUNT; i++) {
        pixels_->setPixelColor(i, pixels_->Color(255, 0, 0));
    }
    pixels_->show();
    
    DEBUG_PRINTLN("Анимация конца завершена!");
#endif
}

void LinerRobot::updateLineFollowingLED(float linePosition) {
#ifdef FEATURE_NEOPIXEL
    if (!pixels_) return;
    
    // linePosition: -1.0 (левый край) до 1.0 (правый край)
    // Показываем отклонение: чем больше отклонение, тем больше LED горят с той стороны
    
    const int leftStart = 0;
    const int leftEnd = 7;
    const int rightStart = 8;
    const int rightEnd = 15;
    
    // Очищаем
    pixels_->clear();
    
    if (linePosition < 0) {
        // Линия слева - зажигаем левые LED
        float leftIntensity = -linePosition; // 0.0 до 1.0
        int numLeftLEDs = (int)(leftIntensity * 8);
        numLeftLEDs = constrain(numLeftLEDs, 0, 8);
        
        // Левая сторона: зеленый (чем больше отклонение, тем больше LED)
        for (int i = 0; i < numLeftLEDs; i++) {
            int brightness = 255 - (i * 20); // Градиент яркости
            pixels_->setPixelColor(leftStart + i, pixels_->Color(0, brightness, 0));
        }
        
        // Правая сторона: синий (минимальная индикация)
        for (int i = rightStart; i <= rightEnd; i++) {
            pixels_->setPixelColor(i, pixels_->Color(0, 0, 50));
        }
        
    } else if (linePosition > 0) {
        // Линия справа - зажигаем правые LED
        float rightIntensity = linePosition; // 0.0 до 1.0
        int numRightLEDs = (int)(rightIntensity * 8);
        numRightLEDs = constrain(numRightLEDs, 0, 8);
        
        // Правая сторона: зеленый (чем больше отклонение, тем больше LED)
        for (int i = 0; i < numRightLEDs; i++) {
            int brightness = 255 - (i * 20); // Градиент яркости
            pixels_->setPixelColor(rightStart + i, pixels_->Color(0, brightness, 0));
        }
        
        // Левая сторона: синий (минимальная индикация)
        for (int i = leftStart; i <= leftEnd; i++) {
            pixels_->setPixelColor(i, pixels_->Color(0, 0, 50));
        }
        
    } else {
        // Линия по центру - все зеленые
        for (int i = 0; i < NEOPIXEL_COUNT; i++) {
            pixels_->setPixelColor(i, pixels_->Color(0, 255, 0));
        }
    }
    
    pixels_->show();
#endif
}

void LinerRobot::updateStatusLED() {
#ifdef FEATURE_NEOPIXEL
    if (!pixels_) return;
    
    // Индикация режима
    if (currentMode_ == Mode::AUTONOMOUS) {
        // В автономном режиме отображаем статус следования
        // Если конец линии - LED уже настроены анимацией
        if (!lineEndAnimationPlayed_) {
            updateLineFollowingLED(pidError_);
        }
    } else {
        // Ручной режим - синий
        for (int i = 0; i < NEOPIXEL_COUNT; i++) {
            pixels_->setPixelColor(i, pixels_->Color(0, 0, 255));
        }
        pixels_->show();
    }
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
