#ifndef MX1508_MOTOR_CONTROLLER_H
#define MX1508_MOTOR_CONTROLLER_H

#include "IMotorController.h"
#include "hardware_config.h"

class WiFiSettings; // Forward declaration

// ═══════════════════════════════════════════════════════════════
// КОНТРОЛЛЕР МОТОРОВ MX1508
// ═══════════════════════════════════════════════════════════════
// Реализация управления моторами через драйвер MX1508

class MX1508MotorController : public IMotorController {
public:
    MX1508MotorController();
    virtual ~MX1508MotorController();
    
    // IComponent interface
    bool init() override;
    void update() override;
    void shutdown() override;
    bool isInitialized() const override { return initialized_; }
    
    // IMotorController interface
    void setSpeed(int leftSpeed, int rightSpeed) override;
    void setMotorPWM(int throttlePWM, int steeringPWM) override;
    void stop() override;
    void getCurrentSpeed(int& leftSpeed, int& rightSpeed) const override;
    bool wasWatchdogTriggered() const override;
    
    // Установка WiFi настроек для применения инвертирования моторов
    void setWiFiSettings(WiFiSettings* settings) { wifiSettings_ = settings; }

private:
    bool initialized_;
    int currentLeftSpeed_;
    int currentRightSpeed_;
    unsigned long lastCommandTime_;
    bool watchdogTriggered_;  // Флаг для отслеживания срабатывания watchdog
    WiFiSettings* wifiSettings_;  // Указатель на настройки для инвертирования моторов
    
    // Внутренние методы
    void applyMotorSpeed(int leftSpeed, int rightSpeed);
    int constrainSpeed(int speed) const;
};

#endif // MX1508_MOTOR_CONTROLLER_H
