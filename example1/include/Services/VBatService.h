#ifndef VBAT_SERVICE_H
#define VBAT_SERVICE_H

#include "BaseService.h"
#include "Context/DisplayContext.h"
#include <SimpleKalmanFilter.h>
#include "hardware_config.h"
const float VOLTAGE_REFERENCE = 3.3; // Напряжение питания ESP32, обычно 3.3 В.
const int ADC_RESOLUTION = 4096;     // 12-битный ADC, значения от 0 до 4095.
const float R1 = 10030.0;            // Сопротивление R1 в Омах.
const float R2 = 950.0;              // Сопротивление R2 в Омах.
const int NUM_SAMPLES = 500;
const int NUM_COEFFS = 10;
const float voltageRanges[NUM_COEFFS] = {5.00, 5.38, 5.76, 6.14, 6.52, 6.90, 7.28, 7.66, 8.04, 8.40};
const float correctionCoeffs[NUM_COEFFS] = {1.063025210084034, 1.063025210084034, 1.063025210084034,
                                            1.063025210084034, 1.063025210084034, 1.063025210084034,
                                            1.063025210084034, 1.063025210084034, 1.063025210084034,
                                            1.063025210084034}; // Примерные корректирующие коэффициенты, которые нужно замерить

class VBatService : public BaseService<DisplayContext>
{
public:
    VBatService();
    void init() override;
    void update(DisplayContext &context) override;
    unsigned long getUpdateInterval() override
    {
        return 5000;
    }

private:
    SimpleKalmanFilter kalmanFilter;
    float applyCorrection(float voltage);
    void updateVbat();

    uint16_t vbatIteration; // Итерация измерения
    ulong vbatSumm = 0;     // Сумма измерений
    float vbat = 0;         // Измеренное напряжение батареи
};

#endif // VBAT_SERVICE_H