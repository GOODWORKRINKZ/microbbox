#ifndef BRAIN_ROBOT_H
#define BRAIN_ROBOT_H

#include "BaseRobot.h"
#include "target_config.h"

#ifdef TARGET_BRAIN

// ═══════════════════════════════════════════════════════════════
// МИКРОБОКС БРЕЙН - Модуль управления для других роботов
// ═══════════════════════════════════════════════════════════════
// Транслирует команды управления в различные протоколы:
// PWM, PPM, SBUS, TBS Crossfire

class BrainRobot : public BaseRobot {
public:
    BrainRobot();
    virtual ~BrainRobot();
    
    RobotType getRobotType() const override { return RobotType::BRAIN; }
    
    // Обработка команд управления (для Brain транслируются в протоколы)
    void handleMotorCommand(int throttlePWM, int steeringPWM) override;
    
protected:
    bool initSpecificComponents() override;
    void updateSpecificComponents() override;
    void shutdownSpecificComponents() override;
    void setupWebHandlers(AsyncWebServer* server) override;
    
private:
    // Протоколы вывода
    enum class OutputProtocol {
        PWM,      // PWM 1000-2000 мкс
        PPM,      // PPM сумма
        SBUS,     // SBUS цифровой
        TBS       // TBS Crossfire
    };
    
    // Инициализация протоколов
    bool initPWMOutput();
    bool initPPMOutput();
    bool initSBUSOutput();
    bool initTBSOutput();
    
    // Обновление выходов
    void updateOutputs();
    void sendPWMOutput(int ch1, int ch2, int ch3, int ch4);
    void sendPPMOutput(int channels[], int count);
    void sendSBUSOutput(int channels[], int count);
    void sendTBSOutput(int channels[], int count);
    
    // Веб-обработчики
    void handleCommand(AsyncWebServerRequest* request);
    void handleProtocol(AsyncWebServerRequest* request);
    
    // Состояние
    OutputProtocol currentProtocol_;
    
    // Канал управления (от веб-интерфейса)
    volatile int channelValues_[8]; // 8 каналов
};

#endif // TARGET_BRAIN

#endif // BRAIN_ROBOT_H
