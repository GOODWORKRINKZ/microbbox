#ifndef ANALYSIS_SERVICE_H
#define ANALYSIS_SERVICE_H

#include "BaseService.h"
#include "Context/AggregateContext.h"
#include <cstdint>

// Класс для управления сервисом агрегации данных
class AggregateService : public BaseService<AggregateContext>
{
public:
    AggregateService() = default;  // Конструктор по умолчанию
    virtual ~AggregateService() override = default;  // Деструктор по умолчанию

    // Инициализация сервиса
    virtual void init() override;

    // Обновление данных контекста
    virtual void update(AggregateContext &context) override;

    // Получение интервала обновления
    virtual unsigned long getUpdateInterval() override;

private:
    // Применение порогового значения
    uint8_t applyThreshold(const uint8_t *dataIn, uint8_t *dataOut, size_t dataSize);

    // Агрегация данных
    void aggregateData(const uint8_t *dataIn, size_t dataInCount, uint8_t *dataOut, size_t dataOutCount);

    // Рассчет среднего значения
    float calculateMean(const uint8_t data[], size_t dataSize);

    // Рассчет среднеквадратичного отклонения
    float calculateStdDev(const uint8_t data[], size_t dataSize, float mean);

    // Обработка диапазона частот
    template <size_t N, uint16_t MIN_FREQ, uint16_t MAX_FREQ>
    void processRange(FrequencyRange &displayRange, 
                      FrequencyBand<N, MIN_FREQ, MAX_FREQ> &radioRange, 
                      uint8_t intermediateBuffer1[], uint8_t intermediateBuffer2[]);

    // Проверка состояния предупреждений
    template <size_t N>
    bool checkWarnCondition(const uint8_t rssi[], uint8_t threshold, size_t neighbors);

    // Установка состояния предупреждений
    template <size_t N, uint16_t MIN_FREQ, uint16_t MAX_FREQ>
    void checkAndSetAlert(FrequencyRange &displayRange, const FrequencyBand<N, MIN_FREQ, MAX_FREQ> &radioRange, size_t neighbors);

#ifdef RADIO_1_2G_ENABLED
    // Массивы данных для диапазона 1.2 ГГц
    uint8_t rssi_1_2[MAX_CHANNELS_1_2G];
    uint8_t rssi_1_2_[MAX_CHANNELS_1_2G];
#endif

#ifdef RADIO_2_4G_ENABLED
    // Массивы данных для диапазона 2.4 ГГц
    uint8_t rssi_2_4[MAX_CHANNELS_2_4G];
    uint8_t rssi_2_4_[MAX_CHANNELS_2_4G];
#endif

#ifdef RADIO_5_8G_ENABLED
    // Массивы данных для диапазона 5.8 ГГц
    uint8_t rssi_5_8[MAX_CHANNELS_5_8G];
    uint8_t rssi_5_8_[MAX_CHANNELS_5_8G];
#endif
};

#endif // ANALYSIS_SERVICE_H