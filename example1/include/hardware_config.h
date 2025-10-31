/*
В этом файле определяется аппаратная конфигурация – какое оборудование присутствует и включено в системе.
Это может включать не только информацию о том, какие модули установлены, но также конфигурацию функций,
доступных в фирмваре, наборы возможностей и другие характеристики конкретных устройств.

Здесь определения относятся к наличию и активации устройств или функций в системе.
*/
#include "globals.h"

#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

// === Включение или отключение радиомодулей ===
#ifdef RADIO_1_2G_ENABLED
#define RADIO_1_2G_ENABLED 1   // Модуль Rx1200 для 1.2 ГГц включен
#else
#define RADIO_1_2G_ENABLED 0
#endif

#ifdef RADIO_2_4G_ENABLED
#define RADIO_2_4G_ENABLED 1   // Модуль Rx2400 для 2.4 ГГц включен
#else
#define RADIO_2_4G_ENABLED 0
#endif

#ifdef RADIO_5_8G_ENABLED
#define RADIO_5_8G_ENABLED 1   // Модуль Rx5808 для 5.8 ГГц включен
#else
#define RADIO_5_8G_ENABLED 0
#endif

#define TARGET_VERSION_STRING TOSTRING(RADIO_1_2G_ENABLED) \
                              TOSTRING(RADIO_2_4G_ENABLED) \
                              TOSTRING(RADIO_5_8G_ENABLED)

inline const char* getTargetVersion() {
    return TARGET_VERSION_STRING;
}


// === Определения для HSPI шины ===
#define HSPI_MOSI_PIN 13  // MOSI пин для HSPI
#define HSPI_MISO_PIN 12  // MISO пин для HSPI
#define HSPI_SCLK_PIN 14  // SCLK пин для HSPI

// === Определения для приемника 5.8 ГГц (RX5808) ===
#define RX5808_CS_PIN 21            // Chip Select (CS) для 5.8 ГГц приемника
#define RX5808_RSSI_PIN 34          // RSSI пин для 5.8 ГГц приемника
#define MAX_CHANNELS_5_8G 256       // Максимальное количество каналов для модуля 5.8 ГГц
#define RSSI_5800_MIN_VAL_DEFAULT 500       // Минимальное значение RSSI для 5.8 ГГц
#define RSSI_5800_MAX_VAL_DEFAULT 750       // Максимальное значение RSSI для 5.8 ГГц
#define MIN_5800_FREQ 5000          // Минимальная частота для 5.8 ГГц
#define MAX_5800_FREQ 6000          // Максимальная частота для 5.8 ГГц

// === Определения для приемника 2.4 ГГц (CC2500) ===
#define CC2500_CS_PIN 25            // Chip Select (CS) для 2.4 ГГц приемника
#define MAX_CHANNELS_2_4G 255       // Максимальное количество каналов для модуля 2.4 ГГц
#define RSSI_2400_MIN_VAL_DEFAULT -100       // Минимальное значение RSSI для 2.4 ГГц
#define RSSI_2400_MAX_VAL_DEFAULT -40       // Максимальное значение RSSI для 2.4 ГГц
#define MIN_2400_FREQ 2400          // Минимальная частота для 2.4 ГГц
#define MAX_2400_FREQ 2484          // Максимальная частота для 2.4 ГГц

// === Определения для приемника 1.2 ГГц (SM1370R) ===
#define SM1370R_RSSI_PIN 35         // RSSI пин для 1.2 ГГц приемника (аналоговый вход)
#define SM1370R_S1_PIN 15           // S1 пин для 1.2 ГГц приемника (цифровой выход)
#define SM1370R_CS1_PIN 27          // CS1 пин для 1.2 ГГц приемника (цифровой выход)
#define SM1370R_CS2_PIN 19          // CS2 пин для 1.2 ГГц приемника (цифровой выход)
#define SM1370R_CS3_PIN 22          // CS3 пин для 1.2 ГГц приемника (цифровой выход)
#define MAX_CHANNELS_1_2G 9         // Максимальное количество каналов для модуля 1.2 ГГц
#define RSSI_1200_MIN_VAL_DEFAULT 1200       // Минимальное значение RSSI для 1.2 ГГц
#define RSSI_1200_MAX_VAL_DEFAULT 1650       // Максимальное значение RSSI для 1.2 ГГц
#define MIN_1200_FREQ 1080          // Минимальная частота для 1.2 ГГц
#define MAX_1200_FREQ 1360          // Максимальная частота для 1.2 ГГц

// === Другие определения ===
#define BATTERY_PIN 26              // Пин для измерения напряжения
// Определите пины кнопок
#define BUTTON_UP_PIN 16
#define BUTTON_DOWN_PIN 17
// Tone Settings
#define BUZZER_PIN 33
#define MOTOR_PIN 32

#endif // HARDWARE_CONFIG_H