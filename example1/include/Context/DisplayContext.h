#ifndef DISPLAY_CONTEXT_H
#define DISPLAY_CONTEXT_H
#include "hardware_config.h"
#include <cstdint>
#include "struct.h"
#include <globals.h>
#include <Settings.h>

// Состояния устройства.
enum class DeviceState
{
    IDLE = 0,        // Поиск
    WARN = 1,        // Внимание
    LOW_VOLTAGE = 2, // Батарея разряжена
};

// Структура для хранения значений, связанных с диапазоном частот
typedef struct
{
    uint8_t rssi[DISPLAY_CHANNELS()] = {0}; // RSSI для диапазона частот
    uint8_t threshold = 0;                  // Пороговое значение для диапазона частот
    RSSIReading topFreq[TOP_FREQ_COUNT];    // RSSI для лучших частот в диапазоне
    int maxRssi = 0;                        // Максимальный RSSI для диапазона
    unsigned long timestamp = 0;            // Временная метка для диапазона частот
    bool alert = 0;                         // Превышение порога в этом диапазоне
    const BandSettings *settings;           // Ссылка на настройки диапазона

    // Метод для проверки, активно ли предупреждение и не находится ли диапазон в состоянии мьюта
    bool isActiveAlert() const
    {
        return alert && !settings->mute;
    }
} FrequencyRange;

typedef struct
{
    FrequencyRange range_1_2; // Данные для диапазона 1.2 GHz
    FrequencyRange range_2_4; // Данные для диапазона 2.4 GHz
    FrequencyRange range_5_8; // Данные для диапазона 5.8 GHz

    int maxRssi = 0;                              // Максимальный RSSI
    float voltage = 0;                            // Напряжение питания
    const char *notification = IDLE_NOTIFICATION; // Уведомление
    bool speaker = false;                         // Состояние динамика
    uint8_t state = 0;                            // Состояние
    uint8_t calibMode = 0;                        ///< Режим калибровки RSSI

} DisplayContext;

#endif // DISPLAY_CONTEXT_H