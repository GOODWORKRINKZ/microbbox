#ifndef CLASSIC_ROBOT_H
#define CLASSIC_ROBOT_H

#include "BaseRobot.h"
#include "target_config.h"

#ifdef TARGET_CLASSIC

#ifdef FEATURE_NEOPIXEL
#include <Adafruit_NeoPixel.h>
#endif

// ═══════════════════════════════════════════════════════════════
// МИКРОБОКС КЛАССИК - Полнофункциональный управляемый робот
// ═══════════════════════════════════════════════════════════════

class ClassicRobot : public BaseRobot {
public:
    ClassicRobot();
    virtual ~ClassicRobot();
    
    RobotType getRobotType() const override { return RobotType::CLASSIC; }
    
protected:
    bool initSpecificComponents() override;
    void updateSpecificComponents() override;
    void shutdownSpecificComponents() override;
    void setupWebHandlers(AsyncWebServer* server) override;
    
private:
    // Инициализация компонентов
    bool initMotors();
    bool initLEDs();
    bool initBuzzer();
    
    // Обработка управления
    void updateMotors();
    void updateEffects();
    
    // Веб-обработчики
    void handleCommand(AsyncWebServerRequest* request);
    
#ifdef FEATURE_NEOPIXEL
    // Управление LED
    void setLEDColor(int ledIndex, uint32_t color);
    void setAllLEDs(uint32_t color);
    void clearLEDs();
    void updateLEDs();
    
    // Эффекты
    void playPoliceEffect();
    void playFireEffect();
    void playAmbulanceEffect();
    void playTerminatorEffect();
    void playMovementAnimation();
#endif
    
#ifdef FEATURE_BUZZER
    // Управление бузером
    void playTone(int frequency, int duration);
    void stopBuzzer();
#endif
    
    // Специфичные поля Classic робота
#ifdef FEATURE_NEOPIXEL
    Adafruit_NeoPixel* pixels_;
#endif
    
#if defined(FEATURE_NEOPIXEL) || defined(FEATURE_BUZZER)
    EffectMode currentEffectMode_;
    unsigned long lastEffectUpdate_;
    bool effectState_;
#endif
    
    ControlMode currentControlMode_;
    
    // Целевые значения PWM (из веб-команд)
    volatile int targetThrottlePWM_;
    volatile int targetSteeringPWM_;
};

#endif // TARGET_CLASSIC

#endif // CLASSIC_ROBOT_H
