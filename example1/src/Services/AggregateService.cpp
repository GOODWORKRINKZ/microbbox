#include "Services/AggregateService.h"
#include <cmath>
#include <Arduino.h>
#include <hardware_config.h>

void calculateSlidingWindowAverage(uint8_t source[], uint8_t result[], int size, int k, int z)
{
    // Проверка входных параметров
    if (k < 1)
        k = 1;
    if (z < 1)
        z = 1;

    for (int i = 0; i < size; ++i)
    {
        if (i < (k - 1) / 2 || i >= size - (k - 1) / 2)
        {
            // Проходим без изменений начало и конец массива, где окно не применяется
            result[i] = source[i];
        }
        else
        {
            // Вычисляем среднее значение окна для остальных элементов
            int windowSum = 0;
            for (int j = i - (k - 1) / 2; j <= i + (k - 1) / 2; ++j)
            {
                windowSum += source[j];
            }
            int windowAvg = windowSum / k;

            // Проверяем, выше ли значение z соседей среднего
            bool neighborHigher = false;
            for (int j = 1; j <= z && !neighborHigher; ++j)
            {
                if (i - j >= 0 && source[i - j] > windowAvg)
                {
                    neighborHigher = true;
                    result[i] = source[i - j];
                }
                if (i + j < size && source[i + j] > windowAvg)
                {
                    neighborHigher = true;
                    result[i] = source[i + j];
                }
            }

            // Если соседи не выше среднего, то присваиваем текущему элементу среднее значение или его само значение, если оно было больше
            if (!neighborHigher)
            {
                result[i] = (source[i] > windowAvg) ? source[i] : windowAvg;
            }
        }
    }
}

template <size_t N, uint16_t MIN_FREQ, uint16_t MAX_FREQ>
void insertTopRSSI(RSSIReading top[], int topSize, const FrequencyBand<N, MIN_FREQ, MAX_FREQ> &range)
{
    // Инициализируем массив top пустыми значениями
    for (int i = 0; i < topSize; ++i)
    {
        top[i].frequency = 0;
        top[i].value = 0;
    }
    for (size_t i = 0; i < range.numChannels; ++i)
    {
        uint8_t newRSSI = range.rssi[i];

        // Используем метод для получения частоты
        uint16_t newFrequency = range.getChannelFrequency(i);

        for (int j = 0; j < topSize; ++j)
        {
            if (newRSSI > top[j].value)
            {
                // Смещаем предыдущие значения вниз
                for (int k = topSize - 1; k > j; --k)
                {
                    top[k] = top[k - 1];
                }
                // Вставляем новое значение
                top[j].frequency = newFrequency;
                top[j].value = newRSSI;
                break; // Остановим цикл после вставки
            }
        }
    }
}

template <size_t N, uint16_t MIN_FREQ, uint16_t MAX_FREQ>
void AggregateService::checkAndSetAlert(FrequencyRange &displayRange, const FrequencyBand<N, MIN_FREQ, MAX_FREQ> &radioRange, size_t neighbors)
{
    uint8_t threshold = displayRange.settings->sensitivity;
    if (checkWarnCondition<N>(radioRange.rssi, threshold, neighbors))
    {
        displayRange.alert = true;
    }
    else
    {
        displayRange.alert = false;
    }
}

void AggregateService::init()
{
    // Инициализация, если требуется
#if RADIO_1_2G_ENABLED
    Serial.println("AggregateService range_1_2 will processed");
#endif
#if RADIO_2_4G_ENABLED
    Serial.println("AggregateService range_2_4 will processed");
#endif
#if RADIO_5_8G_ENABLED
    Serial.println("AggregateService range_5_8 will processed");
#endif
}

void AggregateService::update(AggregateContext &context)
{
#if RADIO_1_2G_ENABLED
    insertTopRSSI(context.displayCtx->range_1_2.topFreq, TOP_FREQ_COUNT, context.radioCtx->range_1_2);
    // Агрегируем данные для отображения
    AggregateService::aggregateData(context.radioCtx->range_1_2.rssi, context.radioCtx->range_1_2.numChannels,
                                    context.displayCtx->range_1_2.rssi, DISPLAY_CHANNELS());
    // Обновляем временную метку
    context.displayCtx->range_1_2.timestamp = context.radioCtx->range_1_2.timestamp;
    // Используем RSSI из нулевого элемента topFreq
    context.displayCtx->range_1_2.maxRssi = context.displayCtx->range_1_2.topFreq[0].value;
#endif

#if RADIO_2_4G_ENABLED
    processRange(context.displayCtx->range_2_4, context.radioCtx->range_2_4, rssi_2_4, rssi_2_4_);
#endif

#if RADIO_5_8G_ENABLED
    processRange(context.displayCtx->range_5_8, context.radioCtx->range_5_8, rssi_5_8, rssi_5_8_);
#endif

    // Находим максимальный RSSI среди всех диапазонов
    context.displayCtx->maxRssi = std::max({
#if RADIO_1_2G_ENABLED
        context.displayCtx->range_1_2.maxRssi,
#endif
#if RADIO_2_4G_ENABLED
        context.displayCtx->range_2_4.maxRssi,
#endif
#if RADIO_5_8G_ENABLED
        context.displayCtx->range_5_8.maxRssi
#endif
    });

    context.displayCtx->notification = IDLE_NOTIFICATION;
    context.displayCtx->state = (uint8_t)DeviceState::IDLE;

    if (context.displayCtx->voltage < MIN_VOLTAGE_WARN)
    {
        context.displayCtx->notification = VOLTAGE_NOTIFICATION;
        context.displayCtx->state = (uint8_t)DeviceState::LOW_VOLTAGE;
    }

    // Проверка и установка флагов предупреждений для каждого диапазона частот
#if RADIO_1_2G_ENABLED
    checkAndSetAlert(context.displayCtx->range_1_2, context.radioCtx->range_1_2, 1);
#endif
#if RADIO_2_4G_ENABLED
    checkAndSetAlert(context.displayCtx->range_2_4, context.radioCtx->range_2_4, 10);
#endif
#if RADIO_5_8G_ENABLED
    checkAndSetAlert(context.displayCtx->range_5_8, context.radioCtx->range_5_8, 1);
#endif

    bool alert = false;
    // Установка уведомлений и состояния, если есть активные предупреждения
#if RADIO_1_2G_ENABLED
    if (context.displayCtx->range_1_2.isActiveAlert())
    {
        alert = true;
    }
#endif
#if RADIO_2_4G_ENABLED
    if (context.displayCtx->range_2_4.isActiveAlert())
    {
        alert = true;
    }
#endif
#if RADIO_5_8G_ENABLED
    if (context.displayCtx->range_5_8.isActiveAlert())
    {
        alert = true;
    }
#endif
    if (alert)
    {
        context.displayCtx->notification = ATTENTION_NOTIFICATION;
        context.displayCtx->state = (uint8_t)DeviceState::WARN;
    }
}

template <size_t N>
bool AggregateService::checkWarnCondition(const uint8_t rssi[], uint8_t threshold, size_t neighbors)
{
    for (size_t i = 0; i < N - neighbors; ++i)
    {
        bool isWarn = true;
        for (size_t j = 0; j <= neighbors; ++j)
        {
            if (rssi[i + j] < threshold)
            {
                isWarn = false;
                break;
            }
        }
        if (isWarn)
        {
            return true;
        }
    }
    return false;
}

template <size_t N, uint16_t MIN_FREQ, uint16_t MAX_FREQ>
void AggregateService::processRange(FrequencyRange &displayRange, FrequencyBand<N, MIN_FREQ, MAX_FREQ> &radioRange, uint8_t intermediateBuffer1[], uint8_t intermediateBuffer2[])
{
    // Обновляем топовые RSSI чтения
    insertTopRSSI(displayRange.topFreq, TOP_FREQ_COUNT, radioRange);

    // Применяем порогообразующее значение
    displayRange.threshold = AggregateService::applyThreshold(radioRange.rssi, intermediateBuffer1, N);

    // Вычисляем скользящее среднее
    calculateSlidingWindowAverage(intermediateBuffer1, intermediateBuffer2, N, 9, 4);
    calculateSlidingWindowAverage(intermediateBuffer2, intermediateBuffer1, N, 5, 2);

    // Повторно применяем порогообразующее значение
    displayRange.threshold = AggregateService::applyThreshold(intermediateBuffer1, intermediateBuffer2, N);

    // Агрегируем данные для отображения
    AggregateService::aggregateData(intermediateBuffer2, N, displayRange.rssi, DISPLAY_CHANNELS());

    // Обновляем временную метку
    displayRange.timestamp = radioRange.timestamp;
    displayRange.maxRssi = displayRange.topFreq[0].value; // Используем RSSI из нулевого элемента topFreq
}

unsigned long AggregateService::getUpdateInterval()
{
    return 35;
}

uint8_t AggregateService::applyThreshold(const uint8_t *dataIn, uint8_t *dataOut, size_t dataSize)
{
    float mean = calculateMean(dataIn, dataSize);
    float stddev = calculateStdDev(dataIn, dataSize, mean);

    for (size_t i = 0; i < dataSize; ++i)
    {
        dataOut[i] = (dataIn[i] >= mean + stddev) ? dataIn[i] : 0;
    }
    return mean + stddev;
}

void AggregateService::aggregateData(const uint8_t *dataIn, size_t dataInCount, uint8_t *dataOut, size_t dataOutCount)
{
    if (dataOutCount <= dataInCount)
    {
        // Агрегация с сжатием данных, если имеется больше данных, чем столбиков
        float scale = (float)dataInCount / dataOutCount;
        for (int i = 0; i < dataOutCount; ++i)
        {
            int startIdx = (int)(i * scale);
            int endIdx = (int)((i + 1) * scale);
            int sum = 0;
            for (int j = startIdx; j < endIdx; ++j)
            {
                sum += dataIn[j];
            }
            dataOut[i] = sum / (endIdx - startIdx);
        }
    }
    else
    {
        // Растягивание данных, если столбцов больше чем данные
        float scale = (float)dataOutCount / dataInCount;
        for (int i = 0; i < dataOutCount; ++i)
        {
            int dataIndex = (int)(i / scale);
            dataOut[i] = dataIn[dataIndex];
        }
    }
}

float AggregateService::calculateMean(const uint8_t data[], size_t dataSize)
{
    float sum = 0;
    for (size_t i = 0; i < dataSize; ++i)
    {
        sum += data[i];
    }
    return sum / dataSize;
}

float AggregateService::calculateStdDev(const uint8_t data[], size_t dataSize, float mean)
{
    float variance = 0;
    for (size_t i = 0; i < dataSize; ++i)
    {
        variance += std::pow(data[i] - mean, 2);
    }
    variance /= dataSize;
    return std::sqrt(variance);
}