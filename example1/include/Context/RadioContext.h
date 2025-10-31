#ifndef RADIO_CONTEXT_H
#define RADIO_CONTEXT_H
#include "hardware_config.h"
#include <cstdint>
#include "struct.h"

// Структура для представления данных одного диапазона частот
template <size_t N, uint16_t MIN_FREQ, uint16_t MAX_FREQ>
struct FrequencyBand
{
    uint8_t rssi[N] = {0};
    uint16_t currentChannel;
    ulong timestamp = 0;
    const size_t numChannels = N;

    uint16_t getChannelFrequency(uint16_t channel) const
    {
        return (MIN_FREQ + channel * (MAX_FREQ - MIN_FREQ) / N);
    }
};

// Используем предопределенные константы для количества каналов в каждом диапазоне и частот
typedef struct
{
    FrequencyBand<MAX_CHANNELS_1_2G, MIN_1200_FREQ, MAX_1200_FREQ> range_1_2; // Данные для диапазона 1.2 GHz
    FrequencyBand<MAX_CHANNELS_2_4G, MIN_2400_FREQ, MAX_2400_FREQ> range_2_4; // Данные для диапазона 2.4 GHz
    FrequencyBand<MAX_CHANNELS_5_8G, MIN_5800_FREQ, MAX_5800_FREQ> range_5_8; // Данные для диапазона 5.8 GHz
} RadioContext;

#endif // RADIO_CONTEXT_H