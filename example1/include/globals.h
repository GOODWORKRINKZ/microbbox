/*
Файл globals.h обычно используется для определения глобальных переменных, констант, макросов,
типов данных и общих утилит, которые могут быть использованы во многих местах проекта.
*/
#ifndef GLOBALS_H
#define GLOBALS_H

// === Макросы для преобразования в строку ===
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

// === Глобальные параметры дисплея ===
#ifndef TFT_WIDTH
#define TFT_WIDTH 240
#endif

#define LEFT_SIDE_WIDTH 40
#define DISPLAY_CHANNELS() (TFT_WIDTH - LEFT_SIDE_WIDTH - 2) // TODO

// === Константы и глобальные параметры приложения ===
#define APP_VERSION TOSTRING(GIT_VERSION)
#define APP_TARGET TOSTRING(TARGET)
#define DEVICE_NAME "ФИЛИН"
#define TOP_FREQ_COUNT 9 // Количество частот в топе по RSSI
#define DEFAULT_SENSITIVITY 40
#define DEFAULT_MUTE false

// === Уведомления и сообщения ===
#define IDLE_NOTIFICATION "Поиск..."
#define ATTENTION_NOTIFICATION "Внимание!"
#define VOLTAGE_NOTIFICATION "Разряжен!"

// === Параметры питания и безопасности ===
#define MIN_VOLTAGE_WARN 6.2

// Настройки WiFi точки доступа для режима прошивки
#define WIFI_SSID "FILIN"
#define WIFI_PASSWORD "@DronesFactory"
#define WIFI_IP IPAddress(10, 0, 0, 5)
#define WIFI_PORT 80

#define FORCE_TARGET_ON_UPDATE 1 // не проверять версию при обновлении по Wi-Fi.
#endif // GLOBALS_H