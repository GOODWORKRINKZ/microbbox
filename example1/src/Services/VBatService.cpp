#include "Services/VBatService.h"
#include <Arduino.h>

VBatService::VBatService()
    : kalmanFilter(0.8, 0.9, 0.3), vbatIteration(0), vbatSumm(0), vbat(0)
{
}

void VBatService::init()
{
    analogReadResolution(12);
    // pinMode(BATTERY_PIN, INPUT);
    for (size_t i = 0; i < 2000; i++)
    {
        updateVbat();
    }
}

void VBatService::updateVbat()
{
    if (vbatIteration < NUM_SAMPLES)
    {
        vbatSumm += kalmanFilter.updateEstimate(analogRead(BATTERY_PIN));
        vbatIteration++;
    }
    else
    {
        // Вычисляем напряжение на ADC входе
        float voltage = ((float)vbatSumm / NUM_SAMPLES / ADC_RESOLUTION) * VOLTAGE_REFERENCE;
        // Пересчитываем напряжение на аккумуляторе, исходя из делителя.
        vbat = applyCorrection((voltage * (R1 + R2) / R2));
        vbatSumm = 0;
        vbatIteration = 0;
    }
}

void VBatService::update(DisplayContext &context)
{
    for (size_t i = 0; i < NUM_SAMPLES; i++)
    {
        updateVbat();
        delayMicroseconds(random(130, 230));
    }
    context.voltage = vbat;
    
    //Serial.println(vbat);
}

float VBatService::applyCorrection(float voltage)
{
    for (int i = 0; i < NUM_COEFFS - 1; i++)
    {
        if (voltage >= voltageRanges[i] && voltage < voltageRanges[i + 1])
        {
            return voltage * correctionCoeffs[i];
        }
    }
    // Если напряжение выше последнего диапазона, применяем последний коэффициент
    if (voltage >= voltageRanges[NUM_COEFFS - 1])
    {
        return voltage * correctionCoeffs[NUM_COEFFS - 1];
    }

    return voltage; // Если значение не попадает в диапазон, возвращаем без коррекции
}