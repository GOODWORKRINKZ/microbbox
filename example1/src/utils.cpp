#include <cstdint>
#include <Arduino.h>
#include <random>
#include <esp_system.h>
#include "Settings.h"
#include <EEPROM.h>
#include <globals.h>
#include <Context/DisplayContext.h>
#include <RSSICalibrationData.h>
const int SETTINGS_ADDRESS = 0;
const int RSSI_ADDRESS = sizeof(Settings); // RSSI данные будут сразу после настроек

// Функция для преобразования значения RSSI в цвет
uint16_t rssiToColor565(uint8_t rssi, bool invert = true)
{
    if (rssi == 0)
    {
        return 0x0000;
    }
    uint8_t red = 0, green = 0, blue = 0;

    // Настройка цветов для неинвертированной градации
    if (rssi < 50)
    { // Переход от белого к синему
        // При низком RSSI уменьшаем красный и зеленый цвета, оставляем синий на максимуме
        red = map(rssi, 0, 49, 255, 0);
        green = red;
        blue = 255;
    }
    else
    { // Переход от синего к красному
        // Увеличиваем красный и уменьшаем синий уровень
        red = map(rssi, 50, 100, 0, 255);
        blue = map(rssi, 50, 100, 255, 0);
    }

    if (invert)
    {
        // Инвертировать цвета: низкие значения RSSI - красные, высокие - белые
        std::swap(red, blue); // Поменять местами красный и синий
    }

    // Конвертация из 8-битных значений в 5/6-битные для формата RGB565
    red = (red * 31) / 255;
    green = (green * 63) / 255;
    blue = (blue * 31) / 255;

    // Сборка цвета в формате RGB565
    return (uint16_t)((red << 11) | (green << 5) | blue);
}

uint16_t invertColor565(uint16_t color)
{
    // Инвертирование 5 бит красного и синего и 6 бит зеленого
    uint8_t r = (~color >> 11) & 0x1F;
    uint8_t g = (~color >> 5) & 0x3F;
    uint8_t b = ~color & 0x1F;

    return (r << 11) | (g << 5) | b;
}

uint16_t contrastColor565(uint16_t bgColor)
{
    // Получаем инвертированный цвет
    uint16_t invertedColor = invertColor565(bgColor);

    // Опционально проверяем яркость фона, чтобы выбрать белый или черный цвет для текста:
    // Вычисляем приближенную яркость для формата RGB565 (более точный расчет будет включать
    // веса для разных каналов, но здесь умножение на 8 и 4 является упрощением)
    int brightness = ((bgColor >> 11) & 0x1F) * 8 + ((bgColor >> 5) & 0x3F) * 4 + (bgColor & 0x1F) * 8;

    // Если фон достаточно яркий, возможно, стоит выбрать черный вместо инвертированного цвета
    if (brightness > (32 + 64 + 32) * 254 / 3)
    {                  // Если яркость выше порогового значения
        return 0x0000; // Черный цвет
    }
    else
    {
        return invertedColor;
    }
}

int getRandomValue()
{
    // вероятность всплеска 5%
    if (esp_random() < (UINT32_MAX * 0.05))
    {
        // Возвращаем случайное значение от 0 до 100
        return esp_random() % 101;
    }
    else
    {
        // Возвращаем значении белого шума от 2 до 8
        return 2 + (esp_random() % 10);
    }
}

void saveSettings(const Settings &settings)
{
    EEPROM.put(SETTINGS_ADDRESS, settings);
    EEPROM.commit();
}

void resetSettings(Settings &settings)
{
    // Настройки по умолчанию
    settings.band_1_2.mute = DEFAULT_MUTE;
    settings.band_1_2.sensitivity = DEFAULT_SENSITIVITY;
    settings.band_2_4.mute = DEFAULT_MUTE;
    settings.band_2_4.sensitivity = DEFAULT_SENSITIVITY;
    settings.band_5_8.mute = DEFAULT_MUTE;
    settings.band_5_8.sensitivity = DEFAULT_SENSITIVITY;
}

Settings loadSettings()
{
    Settings settings;
    EEPROM.get(SETTINGS_ADDRESS, settings);

    // Диагностика для проверки загруженных значений
    Serial.print("Loaded band_1_2.sensitivity: ");
    Serial.println(settings.band_1_2.sensitivity);
    Serial.print("Loaded band_2_4.sensitivity: ");
    Serial.println(settings.band_2_4.sensitivity);
    Serial.print("Loaded band_5_8.sensitivity: ");
    Serial.println(settings.band_5_8.sensitivity);

    // Проверка корректности считанных данных
    if (settings.band_1_2.sensitivity == 0xFF && settings.band_2_4.sensitivity == 0xFF && settings.band_5_8.sensitivity == 0xFF)
    {
        // Данные не валидные, применяем настройки по умолчанию
        Serial.println("EEPROM data invalid, loading default settings.");
        resetSettings(settings);
        saveSettings(settings);
    }
    else
    {
        Serial.println("EEPROM data loaded successfully.");
    }

    return settings;
}

/**
 * @brief Сохраняет данные калибровки RSSI в EEPROM.
 * 
 * @param calibData Структура RSSICalibrationData, содержащая данные для сохранения.
 */
void saveCalibrationData(const RSSICalibrationData &calibData)
{
    EEPROM.put(RSSI_ADDRESS, calibData);
    EEPROM.commit();
}

void resetCalibrationData(RSSICalibrationData &calibData){
        // Данные не валидные, применяем значения по умолчанию
        calibData.band_1_2.minRssi = RSSI_1200_MIN_VAL_DEFAULT;
        calibData.band_1_2.maxRssi = RSSI_1200_MAX_VAL_DEFAULT;
        calibData.band_2_4.minRssi = RSSI_2400_MIN_VAL_DEFAULT;
        calibData.band_2_4.maxRssi = RSSI_2400_MAX_VAL_DEFAULT;
        calibData.band_5_8.minRssi = RSSI_5800_MIN_VAL_DEFAULT;
        calibData.band_5_8.maxRssi = RSSI_5800_MAX_VAL_DEFAULT;
        calibData.calibMode = CALIB_OFF; // По умолчанию калибровка выключена
}

/**
 * @brief Загружает данные калибровки RSSI из EEPROM.
 * 
 * @return RSSICalibrationData Структура, содержащая загруженные данные.
 */
RSSICalibrationData loadCalibrationData()
{
    RSSICalibrationData calibData;
    EEPROM.get(RSSI_ADDRESS, calibData);

    // Проверка корректности считанных данных
    if (calibData.band_1_2.minRssi== 0xFFFF && calibData.band_1_2.maxRssi == 0xFFFF &&
        calibData.band_2_4.minRssi == 0xFFFF && calibData.band_2_4.maxRssi == 0xFFFF &&
        calibData.band_5_8.minRssi == 0xFFFF && calibData.band_5_8.maxRssi == 0xFFFF)
    {
        resetCalibrationData(calibData);
        saveCalibrationData(calibData);
    }

    return calibData;
}

void initDisplayContext(DisplayContext &displayCtx, const Settings &settings)
{
    // Устанавливаем указатель на настройки диапазона 1.2 ГГц
    displayCtx.range_1_2.settings = &settings.band_1_2;
    
    // Устанавливаем указатель на настройки диапазона 2.4 ГГц
    displayCtx.range_2_4.settings = &settings.band_2_4;
    
    // Устанавливаем указатель на настройки диапазона 5.8 ГГц
    displayCtx.range_5_8.settings = &settings.band_5_8;
}