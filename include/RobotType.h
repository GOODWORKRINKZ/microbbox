#ifndef ROBOT_TYPE_H
#define ROBOT_TYPE_H

#include <Arduino.h>

// ═══════════════════════════════════════════════════════════════
// ENUM ТИПОВ РОБОТОВ
// ═══════════════════════════════════════════════════════════════
// Определяет типы роботов как enum для эффективности и типобезопасности

enum class RobotType : uint8_t {
    UNKNOWN = 0,
    CLASSIC = 1,
    LINER = 2,
    BRAIN = 3
};

// ═══════════════════════════════════════════════════════════════
// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ДЛЯ КОНВЕРТАЦИИ
// ═══════════════════════════════════════════════════════════════

// Конвертация enum в строку (заглавные буквы)
inline const char* robotTypeToString(RobotType type) {
    switch (type) {
        case RobotType::CLASSIC: return "Classic";
        case RobotType::LINER:   return "Liner";
        case RobotType::BRAIN:   return "Brain";
        default:                 return "Unknown";
    }
}

// Конвертация enum в строку (lowercase для API)
inline String robotTypeToLowerString(RobotType type) {
    switch (type) {
        case RobotType::CLASSIC: return "classic";
        case RobotType::LINER:   return "liner";
        case RobotType::BRAIN:   return "brain";
        default:                 return "unknown";
    }
}

// Конвертация строки в enum
inline RobotType stringToRobotType(const String& str) {
    String lower = str;
    lower.toLowerCase();
    
    if (lower == "classic") return RobotType::CLASSIC;
    if (lower == "liner")   return RobotType::LINER;
    if (lower == "brain")   return RobotType::BRAIN;
    
    return RobotType::UNKNOWN;
}

// Конвертация int в enum (с проверкой диапазона)
inline RobotType intToRobotType(int value) {
    if (value >= static_cast<int>(RobotType::CLASSIC) && 
        value <= static_cast<int>(RobotType::BRAIN)) {
        return static_cast<RobotType>(value);
    }
    return RobotType::UNKNOWN;
}

// Конвертация enum в int
inline int robotTypeToInt(RobotType type) {
    return static_cast<int>(type);
}

#endif // ROBOT_TYPE_H
