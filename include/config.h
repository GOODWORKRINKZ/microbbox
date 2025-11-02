#ifndef MICROBOX_CONFIG_H
#define MICROBOX_CONFIG_H

// ═══════════════════════════════════════════════════════════════
// КОНФИГУРАЦИЯ ФИЧ - Включение/выключение опциональных компонентов
// ═══════════════════════════════════════════════════════════════

// Закомментируйте ненужные ОПЦИОНАЛЬНЫЕ компоненты для экономии памяти и пинов
#define FEATURE_NEOPIXEL        // Адресные светодиоды WS2812B (пин 4)
//#define FEATURE_BUZZER          // Звуковой бузер

// Обязательные компоненты (всегда включены):
// - MOTORS: Моторы с MX1508
// - CAMERA: Камера ESP32CAM
// - WIFI: WiFi и веб-сервер
// - OTA_UPDATE: OTA обновления прошивки

// ═══════════════════════════════════════════════════════════════

// Версия проекта
#ifndef GIT_VERSION
#define GIT_VERSION "v1.0.0-dev"
#endif

#ifndef PROJECT_NAME
#define PROJECT_NAME "MicRoBBox"
#endif

// GitHub repository для проверки обновлений
#ifndef GITHUB_REPO_URL
#define GITHUB_REPO_URL "GOODWORKRINKZ/microbbox"
#endif

// Настройки пинов ESP32CAM
#ifndef LED_BUILTIN
#define LED_BUILTIN 33          // Встроенный светодиод ESP32CAM
#endif

// Пины для нашего проекта
// Вспышка камеры (пин 4) не используется - вместо неё используем передний неопиксель

#ifdef FEATURE_NEOPIXEL
#define NEOPIXEL_PIN 2          // Адресные светодиоды на пин 2
#define NEOPIXEL_LED_CHANNEL 7  // PWM канал 7 для встроенного светодиода на пине 4 (Scout32 стиль)
#define NEOPIXEL_COUNT 3        // Всего 3 светодиода (2 сзади, 1 спереди)
#endif

#ifdef FEATURE_BUZZER
#define BUZZER_PIN 16           // Бузер на пин 16
#define BUZZER_CHANNEL 6        // PWM канал для бузера
#define BUZZER_RESOLUTION 8     // 8-битное разрешение
#endif

// Пины моторов (как в Scout32 - проверенная конфигурация!)
#define MOTOR_LEFT_FWD_PIN 12   // IN1 - Левый мотор вперед
#define MOTOR_LEFT_REV_PIN 13   // IN2 - Левый мотор назад  
#define MOTOR_RIGHT_FWD_PIN 14  // IN3 - Правый мотор вперед (Scout32: RF=GPIO14)
#define MOTOR_RIGHT_REV_PIN 15  // IN4 - Правый мотор назад (Scout32: RR=GPIO15)

// PWM настройки для моторов (как в Scout32!)
#define MOTOR_PWM_FREQ 8000     // 8 кГц
#define MOTOR_PWM_RESOLUTION 13 // 13-битное разрешение (8192 макс, было 16 - невозможно!)
// Каналы PWM (как в Scout32 - проверенная конфигурация!)
#define MOTOR_PWM_CHANNEL_LF 3  // Канал PWM для левого мотора вперед (GPIO12)
#define MOTOR_PWM_CHANNEL_LR 4  // Канал PWM для левого мотора назад (GPIO13)
#define MOTOR_PWM_CHANNEL_RF 2  // Канал PWM для правого мотора вперед (GPIO14)
#define MOTOR_PWM_CHANNEL_RR 5  // Канал PWM для правого мотора назад (GPIO15)

// Ограничение мощности моторов (защита от перегрева регулятора)
// ВАЖНО: Один канал регулятора сгорел, поэтому ограничиваем мощность!
#define MOTOR_MAX_POWER_PERCENT 80  // Максимальная мощность в процентах (0-100)
                                     // 80% = безопасный режим для защиты регулятора

// WiFi настройки - режим подключения к существующей сети
#define WIFI_MODE_CLIENT true          // true = подключение к роутеру, false = точка доступа
#define WIFI_SSID_CLIENT "ROS2"        // Имя сети для подключения
#define WIFI_PASSWORD_CLIENT "1234567890"  // Пароль сети для подключения

// WiFi настройки для режима точки доступа (если WIFI_MODE_CLIENT = false)
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

// Настройки камеры (стандартные для ESP32CAM)
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

// Режим управления моторами (только дифференциальный)
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

#ifdef FEATURE_NEOPIXEL
// Настройки эффектов
#define LED_BRIGHTNESS_DEFAULT 128   // Яркость по умолчанию (50%)
#define LED_BRIGHTNESS_MAX 255       // Максимальная яркость
#endif

// Debug настройки
#ifdef DEBUG
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
  #define DEBUG_PRINTF(x, ...) Serial.printf(x, __VA_ARGS__)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)  
  #define DEBUG_PRINTF(x, ...)
#endif

#endif // MICROBOX_CONFIG_H