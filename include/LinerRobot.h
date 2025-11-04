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
    
    // Алгоритм следования по линии
    void updateLineFollowing();
    float detectLinePosition(); // Возвращает позицию линии от -1.0 (слева) до 1.0 (справа)
    void applyPIDControl(float linePosition);
    
    // Обработка кнопки
    void updateButton();
    void onButtonPressed();
    
    // Обновление управления
    void updateMotors();
    void updateStatusLED();
    
    // Веб-обработчики
    void handleCommand(AsyncWebServerRequest* request);
    void handleStatus(AsyncWebServerRequest* request);
    
#ifdef FEATURE_NEOPIXEL
    // Управление LED и эффектами
    void updateEffects();
    void setLEDColor(int ledIndex, uint32_t color);
    void setAllLEDs(uint32_t color);
    void clearLEDs();
    void updateLEDs();
    
    // Эффекты
    void playPoliceEffect();
    void playFireEffect();
    void playAmbulanceEffect();
    void playTerminatorEffect();
    
    Adafruit_NeoPixel* pixels_;
#endif
    
    // Состояние робота
    Mode currentMode_;
    bool buttonPressed_;
    unsigned long lastButtonCheck_;
    
#if defined(FEATURE_NEOPIXEL)
    // Эффекты
    EffectMode currentEffectMode_;
    unsigned long lastEffectUpdate_;
    bool effectState_;
#endif
    
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
