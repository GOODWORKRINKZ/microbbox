#ifndef RSSI_CALIBRATION_DATA_H
#define RSSI_CALIBRATION_DATA_H
#include <cstdint>

/**
 * @enum CalibMode
 * @brief Перечисление для режима калибровки RSSI.
 * 
 * Режимы калибровки используются для определения, какой тип калибровки
 * должен быть выполнен.
 */
enum CalibMode {
    CALIB_OFF = 0,      ///< Калибровка выключена
    CALIB_MIN_RSSI = 1, ///< Калибровка минимального RSSI
    CALIB_MAX_RSSI = 2  ///< Калибровка максимального RSSI
};

/**
 * @struct RssiBandRange
 * @brief Структура для хранения предельных значений RSSI для диапазона.
 */
struct RssiBandRange {
    int16_t minRssi; ///< Минимальное значение RSSI
    int16_t maxRssi; ///< Максимальное значение RSSI
};

/**
 * @struct RSSICalibrationData
 * @brief Структура для хранения данных калибровки RSSI.
 * 
 * Эта структура содержит предельные значения RSSI для каждого диапазона
 * частот, а также текущий режим калибровки.
 */
struct RSSICalibrationData {
    RssiBandRange band_1_2; ///< Диапазон 1.2 ГГц
    RssiBandRange band_2_4; ///< Диапазон 2.4 ГГц
    RssiBandRange band_5_8; ///< Диапазон 5.8 ГГц
    CalibMode calibMode;    ///< Режим калибровки RSSI
};

#endif // RSSI_CALIBRATION_DATA_H