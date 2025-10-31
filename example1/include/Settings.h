#include <cstdint>
#ifndef SETTINGS_H
#define SETTINGS_H

struct BandSettings
{
    bool mute = false;       // Флаг мьюта для диапазона
    uint8_t sensitivity = 0; // Чувствительность для диапазона
    bool selected = false;
};

struct Settings
{
    BandSettings band_1_2; // Настройки для диапазона 1.2 ГГц
    BandSettings band_2_4; // Настройки для диапазона 2.4 ГГц
    BandSettings band_5_8; // Настройки для диапазона 5.8 ГГц
};

#endif // SETTINGS_H