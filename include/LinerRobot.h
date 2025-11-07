#ifndef LINER_ROBOT_H
#define LINER_ROBOT_H

#include "BaseRobot.h"
#include "target_config.h"
#include "hardware_config.h"

#ifdef TARGET_LINER

#ifdef FEATURE_NEOPIXEL
#include <Adafruit_NeoPixel.h>
#endif

// ═══════════════════════════════════════════════════════════════
// МИКРОБОКС ЛАЙНЕР - Автономный робот следующий по линии
// ═══════════════════════════════════════════════════════════════

class LinerRobot : public BaseRobot {
public:
    LinerRobot();
    virtual ~LinerRobot();
    
    RobotType getRobotType() const override { return RobotType::LINER; }
    
    // Обработка команд управления моторами
    void handleMotorCommand(int throttlePWM, int steeringPWM) override;
    
protected:
    bool initSpecificComponents() override;
    void updateSpecificComponents() override;
    void shutdownSpecificComponents() override;
    void setupWebHandlers(AsyncWebServer* server) override;
    
private:
    // Инициализация компонентов
    bool initMotors();
    bool initLEDs();
    bool initButton();
    
    // Режимы работы
    enum class Mode {
        MANUAL,      // Ручное управление через веб
        AUTONOMOUS   // Автономное следование по линии
    };
    
    // Режимы загрузки
    enum class BootMode {
        CONFIGURATION,  // Режим настройки (с веб-сервером и стримом)
        LINE_FOLLOWING  // Режим следования по линии (оптимизирован, без веб/стрима)
    };
    
    // Алгоритм следования по линии
    void updateLineFollowing();
    float detectLinePosition(); // Возвращает позицию линии от -1.0 (слева) до 1.0 (справа)
    void applyPIDControl(float linePosition);
    
    // Оптимизации детектирования (Best Practices)
    uint8_t calculateOtsuThreshold(uint8_t* img, int width, int height);  // Метод Otsu для адаптивного порога
    float applyMedianFilter(float newPosition);  // Медианный фильтр для сглаживания
    float filterPositionJump(float newPosition); // Фильтрация резких скачков
    
    // Обработка кнопки
    void updateButton();
    void onButtonPressed();
    BootMode detectBootMode();       // Определяет режим загрузки по кнопке при старте
    
#ifdef FEATURE_DUAL_CORE
    // Детектирование линии на отдельном ядре (Core 0)
    static void lineDetectionTask(void* parameter);
    TaskHandle_t lineDetectionTaskHandle_;
    volatile float detectedLinePosition_;
    SemaphoreHandle_t linePositionMutex_;
#endif
    
    // Обновление управления
    void updateMotors();
    void updateStatusLED();
    
    // Анимация LED
    void playStartupAnimation();
    void playLineFollowStartAnimation();  // Анимация при старте следования
    void playLineEndAnimation();          // Анимация при обнаружении конца линии
    void updateLineFollowingLED(float linePosition);  // Отображение отклонения на LED
    
    // Веб-обработчики
    void handleCommand(AsyncWebServerRequest* request);
    void handleStatus(AsyncWebServerRequest* request);
    
#ifdef FEATURE_NEOPIXEL
    Adafruit_NeoPixel* pixels_;
    
    // Эффекты (для будущей поддержки)
    EffectMode currentEffectMode_;
#endif
    
    // Состояние робота
    Mode currentMode_;
    BootMode bootMode_;              // Режим загрузки (определяется при старте)
    bool buttonPressed_;
    unsigned long lastButtonCheck_;
    
    // Следование по линии
    bool lineDetected_;              // Обнаружена ли линия
    int lineNotDetectedCount_;       // Счетчик кадров без линии
    bool lineEndAnimationPlayed_;    // Проиграна ли анимация конца линии
    
    // Оптимизации детектирования (Best Practices)
#if LINE_USE_MEDIAN_FILTER
    float positionHistory_[LINE_MEDIAN_FILTER_SIZE];  // История позиций для медианного фильтра
    int positionHistoryIndex_;                         // Индекс в кольцевом буфере
#endif
    float lastValidPosition_;        // Последняя валидная позиция (для фильтрации скачков)
    uint8_t adaptiveThreshold_;      // Адаптивный порог бинаризации (вычисляется динамически)
    
    // PID контроллер
    float pidError_;
    float pidLastError_;
    float pidIntegral_;
    
    // Управление
    volatile int targetThrottlePWM_;
    volatile int targetSteeringPWM_;
};

#endif // TARGET_LINER

#endif // LINER_ROBOT_H
