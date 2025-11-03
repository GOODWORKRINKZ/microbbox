#ifndef IMOTOR_CONTROLLER_H
#define IMOTOR_CONTROLLER_H

#include "IComponent.h"

// ═══════════════════════════════════════════════════════════════
// ИНТЕРФЕЙС КОНТРОЛЛЕРА МОТОРОВ
// ═══════════════════════════════════════════════════════════════
// Абстракция для управления моторами робота
// Позволяет легко менять реализацию (MX1508, L298N, etc.)

class IMotorController : public IComponent {
public:
    virtual ~IMotorController() = default;
    
    // Установка скорости моторов в процентах (-100 до +100)
    // Отрицательные значения = реверс
    virtual void setSpeed(int leftSpeed, int rightSpeed) = 0;
    
    // Установка скорости в формате PWM (1000-2000 мкс, центр 1500)
    virtual void setMotorPWM(int throttlePWM, int steeringPWM) = 0;
    
    // Остановка моторов
    virtual void stop() = 0;
    
    // Получение текущей скорости
    virtual void getCurrentSpeed(int& leftSpeed, int& rightSpeed) const = 0;
    
    // Проверка, был ли произведен watchdog-stop (нужно для повторной отправки команды)
    virtual bool wasWatchdogTriggered() const = 0;
};

#endif // IMOTOR_CONTROLLER_H
