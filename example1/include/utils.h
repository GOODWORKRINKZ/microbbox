// utils.h
#pragma once
#include <cstdint>
#include "Settings.h"
#include "Context\DisplayContext.h"
#include "RSSICalibrationData.h"

uint16_t rssiToColor565(uint8_t rssi, bool invert = true);
uint16_t contrastColor565(uint16_t bgColor);
int getRandomValue();
Settings loadSettings();
void saveSettings(const Settings &settings);
void initDisplayContext(DisplayContext &displayCtx, const Settings &settings);
void resetSettings(Settings &settings);
void resetRssiCalib(Settings &settings);
/**
 * @brief Сохраняет данные калибровки RSSI в EEPROM.
 * 
 * @param calibData Структура RSSICalibrationData, содержащая данные для сохранения.
 */
void saveCalibrationData(const RSSICalibrationData &calibData);

/**
 * @brief Загружает данные калибровки RSSI из EEPROM.
 * 
 * @return RSSICalibrationData Структура, содержащая загруженные данные.
 */
RSSICalibrationData loadCalibrationData();

void resetCalibrationData(RSSICalibrationData &calibData);