#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

#include "target_config.h"

// ═══════════════════════════════════════════════════════════════
// КОНФИГУРАЦИЯ ПИНОВ ESP32CAM
// ═══════════════════════════════════════════════════════════════

// Встроенные компоненты ESP32CAM
#ifndef LED_BUILTIN
#define LED_BUILTIN 33          // Встроенный светодиод ESP32CAM
#endif

// Настройки камеры (стандартные для ESP32CAM AI-Thinker)
#define CAMERA_MODEL_AI_THINKER

#if defined(CAMERA_MODEL_AI_THINKER)
  #define PWDN_GPIO_NUM     32
  #define RESET_GPIO_NUM    -1
  #define XCLK_GPIO_NUM      0
  #define SIOD_GPIO_NUM     26
  #define SIOC_GPIO_NUM     27
  #define Y9_GPIO_NUM       35
  #define Y8_GPIO_NUM       34
  #define Y7_GPIO_NUM       39
  #define Y6_GPIO_NUM       36
  #define Y5_GPIO_NUM       21
  #define Y4_GPIO_NUM       19
  #define Y3_GPIO_NUM       18
  #define Y2_GPIO_NUM        5
  #define VSYNC_GPIO_NUM    25
  #define HREF_GPIO_NUM     23
  #define PCLK_GPIO_NUM     22
#endif

// ═══════════════════════════════════════════════════════════════
// КОНФИГУРАЦИЯ ПЕРИФЕРИИ (зависит от типа робота)
// ═══════════════════════════════════════════════════════════════

#ifdef FEATURE_NEOPIXEL
    #define NEOPIXEL_PIN 2          // Адресные светодиоды на пин 2
    #define NEOPIXEL_LED_CHANNEL 7  // PWM канал 7
    #define NEOPIXEL_COUNT 3        // Всего 3 светодиода (2 сзади, 1 спереди)
    #define LED_BRIGHTNESS_DEFAULT 128   // Яркость по умолчанию (50%)
    #define LED_BRIGHTNESS_MAX 255       // Максимальная яркость
#endif

#ifdef FEATURE_BUZZER
    // ВРЕМЕННО ОТКЛЮЧЕНО через комментирование FEATURE_BUZZER в target_config.h
    // Причина: конфликт пина 16 с камерой ESP32CAM
    // TODO: Переназначить бузер на другой безопасный пин когда потребуется
    #define BUZZER_PIN 16           // Бузер на пин 16 (ВНИМАНИЕ: конфликтует с камерой!)
    #define BUZZER_CHANNEL 6        // PWM канал для бузера
    #define BUZZER_RESOLUTION 8     // 8-битное разрешение
#endif

#ifdef FEATURE_BUTTON
    #define BUTTON_PIN 4            // Кнопка на пин 4 (GPIO4, безопасный пин)
    #define BUTTON_DEBOUNCE_MS 50   // Время антидребезга
#endif

#ifdef FEATURE_MOTORS
    // Пины моторов (как в Scout32 - проверенная конфигурация!)
    #define MOTOR_LEFT_FWD_PIN 12   // IN1 - Левый мотор вперед
    #define MOTOR_LEFT_REV_PIN 13   // IN2 - Левый мотор назад  
    #define MOTOR_RIGHT_FWD_PIN 14  // IN3 - Правый мотор вперед
    #define MOTOR_RIGHT_REV_PIN 15  // IN4 - Правый мотор назад

    // PWM настройки для моторов
    #define MOTOR_PWM_FREQ 8000     // 8 кГц
    #define MOTOR_PWM_RESOLUTION 13 // 13-битное разрешение (8192 макс)
    
    // Каналы PWM (как в Scout32!)
    #define MOTOR_PWM_CHANNEL_LF 3  // Канал PWM для левого мотора вперед (GPIO12)
    #define MOTOR_PWM_CHANNEL_LR 4  // Канал PWM для левого мотора назад (GPIO13)
    #define MOTOR_PWM_CHANNEL_RF 2  // Канал PWM для правого мотора вперед (GPIO14)
    #define MOTOR_PWM_CHANNEL_RR 5  // Канал PWM для правого мотора назад (GPIO15)

    // Ограничение мощности моторов (защита от перегрева регулятора)
    #define MOTOR_MAX_POWER_PERCENT 100  // Максимальная мощность в процентах (0-100)
    
    // Watchdog для автоостановки моторов
    #define MOTOR_COMMAND_TIMEOUT_MS 500  // Если команды не приходят N мс - останавливаем моторы
#endif

// ═══════════════════════════════════════════════════════════════
// КОНФИГУРАЦИЯ ПРОТОКОЛОВ (для TARGET_BRAIN)
// ═══════════════════════════════════════════════════════════════

#ifdef FEATURE_PWM_OUTPUT
    #define PWM_OUT_PIN_1 12
    #define PWM_OUT_PIN_2 13
    #define PWM_OUT_PIN_3 14
    #define PWM_OUT_PIN_4 15
    #define PWM_OUT_FREQ 50         // 50 Гц для стандартных servo
    #define PWM_OUT_RESOLUTION 16   // 16-битное разрешение
#endif

#ifdef FEATURE_PPM_OUTPUT
    #define PPM_OUT_PIN 12          // PPM выход
    #define PPM_CHANNELS 8          // Количество каналов PPM
#endif

#ifdef FEATURE_SBUS_OUTPUT
    #define SBUS_TX_PIN 12          // SBUS передача
    #define SBUS_BAUD 100000        // 100k baud для SBUS
#endif

#ifdef FEATURE_TBS_OUTPUT
    #define TBS_TX_PIN 12           // TBS Crossfire передача
    #define TBS_BAUD 420000         // 420k baud для Crossfire
#endif

// ═══════════════════════════════════════════════════════════════
// НАСТРОЙКИ WIFI
// ═══════════════════════════════════════════════════════════════

// WiFi настройки - режим подключения к существующей сети
#define WIFI_MODE_CLIENT true          // true = подключение к роутеру, false = точка доступа
#define WIFI_SSID_CLIENT "ROS2"        // Имя сети для подключения
#define WIFI_PASSWORD_CLIENT "1234567890"  // Пароль сети для подключения

// WiFi настройки для режима точки доступа
#define WIFI_PASSWORD_AP "12345678"

// Общие настройки WiFi
#define WIFI_PORT 80
#define WIFI_CHANNEL 1
#define WIFI_HIDDEN false
#define WIFI_MAX_CONNECTIONS 4

// IP адреса для AP режима
#define AP_IP_ADDR 192, 168, 4, 1
#define AP_GATEWAY 192, 168, 4, 1
#define AP_SUBNET 255, 255, 255, 0

// ═══════════════════════════════════════════════════════════════
// СПЕЦИФИЧНЫЕ НАСТРОЙКИ ДЛЯ ТИПОВ РОБОТОВ
// ═══════════════════════════════════════════════════════════════

#ifdef TARGET_LINER
    // Настройки камеры для следования по линии
    #define LINE_CAMERA_WIDTH 96        // Ширина изображения
    #define LINE_CAMERA_HEIGHT 96       // Высота изображения
    #define LINE_THRESHOLD 128          // Порог для ЧБ изображения
    #define LINE_PID_KP 1.0            // Пропорциональный коэффициент PID
    #define LINE_PID_KI 0.0            // Интегральный коэффициент PID
    #define LINE_PID_KD 0.1            // Дифференциальный коэффициент PID
    #define LINE_BASE_SPEED 50          // Базовая скорость движения (%)
#endif

// Режим управления моторами
enum class ControlMode {
    DIFFERENTIAL = 0  // Дифференциальный (правый стик = скорость, левый стик = поворот)
};

#if defined(FEATURE_NEOPIXEL) || defined(FEATURE_BUZZER)
// Эффекты и режимы
enum class EffectMode {
    NORMAL = 0,      // Обычный режим
    POLICE = 1,      // Полицейский режим
    FIRE = 2,        // Пожарный режим
    AMBULANCE = 3,   // Скорая помощь
    TERMINATOR = 4   // Режим T-800 (красный HUD)
};
#endif

#endif // HARDWARE_CONFIG_H
