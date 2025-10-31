# Тестирование конфигураций feature flags

## ✅ Проверено

### 1. Без NEOPIXEL и BUZZER (только моторы)
```cpp
//#define FEATURE_NEOPIXEL
//#define FEATURE_BUZZER
#define FEATURE_MOTORS
#define FEATURE_CAMERA
#define FEATURE_WIFI
#define FEATURE_OTA_UPDATE
```

**Результат**: Компиляция успешна ✅

---

## 🔄 Требуется проверить

### 2. Без CAMERA
```cpp
#define FEATURE_NEOPIXEL
#define FEATURE_BUZZER
#define FEATURE_MOTORS
//#define FEATURE_CAMERA
#define FEATURE_WIFI
#define FEATURE_OTA_UPDATE
```

### 3. Без MOTORS
```cpp
#define FEATURE_NEOPIXEL
#define FEATURE_BUZZER
//#define FEATURE_MOTORS
#define FEATURE_CAMERA
#define FEATURE_WIFI
#define FEATURE_OTA_UPDATE
```

### 4. Только WiFi (минимальная конфигурация)
```cpp
//#define FEATURE_NEOPIXEL
//#define FEATURE_BUZZER
//#define FEATURE_MOTORS
//#define FEATURE_CAMERA
#define FEATURE_WIFI
//#define FEATURE_OTA_UPDATE
```

### 5. Полная конфигурация
```cpp
#define FEATURE_NEOPIXEL
#define FEATURE_BUZZER
#define FEATURE_MOTORS
#define FEATURE_CAMERA
#define FEATURE_WIFI
#define FEATURE_OTA_UPDATE
```

---

## 📝 Заметки

- Компилятор: PlatformIO ESP32
- Окружения: debug, release
- Файлы изменены: config.h, MicroBoxRobot.h, MicroBoxRobot.cpp
- Дата: 2025-10-31
