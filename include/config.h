#ifndef MICROBOX_CONFIG_H
#define MICROBOX_CONFIG_H

// Версия проекта
#ifndef GIT_VERSION
#define GIT_VERSION "v1.0.0-dev"
#endif

#ifndef PROJECT_NAME
#define PROJECT_NAME "МикроББокс"
#endif

// Настройки пинов ESP32CAM
#ifndef LED_BUILTIN
#define LED_BUILTIN 33          // Встроенный светодиод ESP32CAM
#endif

// Пины для нашего проекта
// Вспышка камеры (пин 4) не используется - вместо неё используем передний неопиксель
#define BUZZER_PIN 16           // Бузер на пин 16
#define NEOPIXEL_PIN 2          // Адресные светодиоды на пин 2
#define NEOPIXEL_COUNT 3        // Всего 3 светодиода (2 сзади, 1 спереди)

// Пины моторов (как в Scout32 - проверенная конфигурация!)
#define MOTOR_LEFT_FWD_PIN 12   // IN1 - Левый мотор вперед
#define MOTOR_LEFT_REV_PIN 13   // IN2 - Левый мотор назад  
#define MOTOR_RIGHT_FWD_PIN 15  // IN3 - Правый мотор вперед
#define MOTOR_RIGHT_REV_PIN 14  // IN4 - Правый мотор назад

// PWM настройки для моторов
#define MOTOR_PWM_FREQ 8000     // 8 кГц
#define MOTOR_PWM_RESOLUTION 16 // 16-битное разрешение
#define MOTOR_PWM_CHANNEL_LF 2  // Канал PWM для левого мотора вперед
#define MOTOR_PWM_CHANNEL_LR 3  // Канал PWM для левого мотора назад
#define MOTOR_PWM_CHANNEL_RF 4  // Канал PWM для правого мотора вперед
#define MOTOR_PWM_CHANNEL_RR 5  // Канал PWM для правого мотора назад

// WiFi настройки
#define WIFI_SSID "МикроББокс"
#define WIFI_PASSWORD "12345678"
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

// Режимы управления
enum class ControlMode {
    TANK = 0,        // Танковый режим (левый/правый стик = левая/правая гусеница)
    DIFFERENTIAL = 1  // Дифференциальный (правый = скорость, левый = поворот)
};

// Эффекты и режимы
enum class EffectMode {
    NORMAL = 0,      // Обычный режим
    POLICE = 1,      // Полицейский режим
    FIRE = 2,        // Пожарный режим
    AMBULANCE = 3    // Скорая помощь
};

// Настройки эффектов
#define LED_BRIGHTNESS_DEFAULT 128   // Яркость по умолчанию (50%)
#define LED_BRIGHTNESS_MAX 255       // Максимальная яркость

// Настройки бузера
#define BUZZER_CHANNEL 6            // PWM канал для бузера
#define BUZZER_RESOLUTION 8         // 8-битное разрешение

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