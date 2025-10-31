#ifndef RX_WIDGET_H
#define RX_WIDGET_H

#include <TFT_eSPI.h>
#include <functional>
#include "Widget.h"
#include "Context/DisplayContext.h"
#include "common.h"
#include "WaterfallSprite.h"

class RxWidget : public Widget
{
public:
    // Определение типа для функтора, который возвращает массив RSSI
    template <typename T>
    using DataArrayGetter = std::function<T *(DisplayContext &context)>;
    template <typename T>
    using DataGetter = std::function<T(DisplayContext &context)>;

    // Конструктор виджета
    // tft - указатель на объект TFT_eSPI для работы с дисплеем
    // frequencyRangeGetter - функтор для получения диапазона частот
    // name - название полосы (например, "1.2G")
    // totalModules - общее количество модулей, используется для расчета высоты виджета
    RxWidget(TFT_eSPI *tft, DataGetter<FrequencyRange> frequencyRangeGetter, const String &name,
             uint16_t totalModules, RssiBandRange &rssiRange);

    // Метод для обновления данных виджета
    void update(DisplayContext &context) override;

    // Метод для отрисовки виджета на дисплее
    void draw() override;

private:
    static uint16_t instanceCounter;              // Статический счетчик экземпляров
    uint16_t instanceNumber = 0;                  // Номер текущего экземпляра виджета
    uint16_t widgetHeight = 0;                    // Высота виджета
    uint16_t borderColor = 0;                     // Цвет рамки виджета
    String name;                                  // Название полосы (например, "1.2G")
    DataGetter<FrequencyRange> getFrequencyRange; // Функтор для получения диапазона частот

    TFT_eSprite bgSprite;            // Спрайт для фона виджета
    TFT_eSprite graphSprite;         // Спрайт для графика RSSI
    WaterfallSprite waterfallSprite; // Спрайт для отображения "водопада" RSSI
    unsigned long lastTimestamp = 0; // Временная метка последнего обновления
    uint8_t lastSensitivity = 0;
    ulong lastSensitivityUpdate = 0;
    RssiBandRange &rssiRange;
};

#endif // RX_WIDGET_H