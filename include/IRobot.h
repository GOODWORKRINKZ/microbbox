#ifndef IROBOT_H
#define IROBOT_H

#include <Arduino.h>
#include "IComponent.h"
#include "RobotType.h"

// ═══════════════════════════════════════════════════════════════
// БАЗОВЫЙ ИНТЕРФЕЙС ДЛЯ ВСЕХ ТИПОВ РОБОТОВ
// ═══════════════════════════════════════════════════════════════
// Определяет общий контракт для всех роботов (Classic, Liner, Brain)

class IRobot : public IComponent {
public:
    virtual ~IRobot() = default;
    
    // Основной цикл работы робота
    virtual void loop() = 0;
    
    // Получение IP адреса (если есть WiFi)
    virtual IPAddress getIP() const = 0;
    
    // Получение имени устройства
    virtual String getDeviceName() const = 0;
    
    // Получение типа робота (enum)
    virtual RobotType getRobotType() const = 0;
    
    // Получение типа робота (строка) - для обратной совместимости
    virtual const char* getRobotTypeString() const {
        return robotTypeToString(getRobotType());
    }
};

#endif // IROBOT_H
