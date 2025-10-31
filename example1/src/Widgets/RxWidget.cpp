#include "Widgets/RxWidget.h"
#include <cstdint>
#include "Fonts/FontDejaVu20.h"

#define GRAPH_HEIGHT_VAL 40
#define CORNER_ROUNDIN_ANGLE 15
#define GRAPH_HEIGHT(x) map(GRAPH_HEIGHT_VAL, 0, 100, 0, x)
#define SHOW_SENSITIVITY_TIME 2000

// Функция рисует график столбиками на заданном спрайте
void drawGraphWithSprites(TFT_eSprite &spr, uint8_t *rssiArray, size_t arraySize, uint8_t sensitivity, bool showSensitivity, bool attention)
{
    // Очистка спрайта
    if (attention)
    {
        spr.fillSprite(0x03df);
    }
    else
    {
        spr.fillSprite(TFT_BLACK);
    }

    int16_t width = spr.width();
    int16_t height = spr.height();
    // Определяем ширину каждого столбика
    int dx = (arraySize > 1) ? (width / arraySize) : width;

    for (size_t i = 0; i < arraySize; ++i)
    {
        // Высота столбика, соответствующая значению RSSI
        int barHeight = (rssiArray[i] * height / 100);
        // Получение цвета столбика
        uint16_t color = rssiToColor565(rssiArray[i]);

        // Рисуем столбик: верхняя левая точка (i * dx, height - barHeight), ширина dx, высота barHeight
        spr.fillRect(i * dx, height - barHeight, dx, barHeight, color);
    }
    uint8_t sensitivityHeight = map(sensitivity, 0, 100, height - 1, 2);
    spr.drawLine(0, sensitivityHeight, spr.width(), sensitivityHeight, TFT_RED);
    if (!showSensitivity)
    {
        return;
    }
    // Буфер для преобразования числа в строку
    char sensitivityStr[8]; // достаточно для хранения числа до 65535 и завершающего нулевого символа
    // Преобразуем число в строку
    sprintf(sensitivityStr, "%u%%", sensitivity);
    spr.drawCentreString(sensitivityStr, width / 2, height / 2, 2);
}

uint16_t RxWidget::instanceCounter = 0; // Счетчик экземпляров виджета

RxWidget::RxWidget(TFT_eSPI *display, DataGetter<FrequencyRange> frequencyRangeGetter, const String &name, uint16_t totalModules,
    RssiBandRange &rssiRange)
    : Widget(display), getFrequencyRange(frequencyRangeGetter), name(name),
      bgSprite(TFT_eSprite(display)), graphSprite(TFT_eSprite(display)), waterfallSprite(WaterfallSprite(display)), lastTimestamp(0),
      rssiRange{rssiRange}
{
    Serial.print("RxWidget created: ");
    Serial.println(name);
    // Увеличиваем счетчик и присваиваем номер экземпляра
    instanceNumber = instanceCounter++;

    // Вычисляем высоту виджета в зависимости от общего числа модулей
    widgetHeight = (TFT_HEIGHT - STATUS_BAR_HEIGHT - totalModules * SPACING) / totalModules;

    // Виджет имеет фиксированную ширину, равную ширине экрана
    // Создаем спрайт для виджета
    waterfallSprite.createSprite(DISPLAY_CHANNELS(), (widgetHeight - GRAPH_HEIGHT(widgetHeight) - 3));
    waterfallSprite.fillScreen(TFT_BLACK);
    waterfallSprite.loadFont(FontDejaVu20);
}

// Обновление виджета на основе изменения данных
void RxWidget::update(DisplayContext &context)
{
    bgSprite.createSprite(LEFT_SIDE_WIDTH, widgetHeight);
    bgSprite.setTextColor(TFT_WHITE, DD_SILVER, false);
    graphSprite.createSprite(DISPLAY_CHANNELS(), GRAPH_HEIGHT(widgetHeight));
    graphSprite.fillScreen(TFT_BLACK);
    // Получаем массив RSSI через функтор
    FrequencyRange frequencyRange = getFrequencyRange(context);

    // Рисуем рамку вокруг виджета в зависимости от выбранного состояния
    borderColor = frequencyRange.settings->selected ? 0x03df : TFT_WHITE;
    bgSprite.fillScreen(frequencyRange.settings->selected ? 0x03df : TFT_BLACK);
    // Очищаем и готовим фон для виджета с закругленными углами
    bgSprite.fillRoundRect(0, 0, bgSprite.width(), widgetHeight, CORNER_ROUNDIN_ANGLE, frequencyRange.settings->selected ? TFT_BLACK : DD_SILVER);
    bgSprite.drawRoundRect(0, 0, bgSprite.width(), widgetHeight, CORNER_ROUNDIN_ANGLE, borderColor);
    bgSprite.drawLine(LEFT_SIDE_WIDTH / 2, 0, bgSprite.width(), 0, borderColor);
    bgSprite.drawLine(LEFT_SIDE_WIDTH / 2, widgetHeight - 1, bgSprite.width(), widgetHeight - 1, borderColor);
    bgSprite.setTextColor(TFT_WHITE, frequencyRange.settings->selected ? TFT_BLACK : DD_SILVER, false);
    bgSprite.drawCentreString(name, LEFT_SIDE_WIDTH / 2, PADING * 2, 2);

    // Отображаем топ частоты
    for (size_t i = 0; i < TOP_FREQ_COUNT; i++)
    {
        uint16_t height = 20 * (i + 1);
        if (widgetHeight - 20 < height)
        {
            break;
        }
        bgSprite.setTextColor(contrastColor565(rssiToColor565(frequencyRange.topFreq[i].value)), rssiToColor565(frequencyRange.topFreq[i].value));
        if (frequencyRange.topFreq[i].value > 0)
        {
            // Буфер для преобразования числа в строку
            char frequencyStr[8]; // достаточно для хранения числа до 65535 и завершающего нулевого символа
            // Преобразуем число в строку
            sprintf(frequencyStr, "%u", frequencyRange.topFreq[i].frequency);
            bgSprite.drawCentreString(frequencyStr, LEFT_SIDE_WIDTH / 2, PADING * 2 + height, 2);
        }
        else
        {
            bgSprite.drawCentreString("0000", LEFT_SIDE_WIDTH / 2, PADING * 2 + height, 2);
        }
    }
    if (lastSensitivity != frequencyRange.settings->sensitivity)
    {
        lastSensitivity = frequencyRange.settings->sensitivity;
        lastSensitivityUpdate = millis();
    }
    drawGraphWithSprites(graphSprite, frequencyRange.rssi, DISPLAY_CHANNELS(), frequencyRange.settings->sensitivity,
                         millis() - lastSensitivityUpdate < SHOW_SENSITIVITY_TIME, frequencyRange.alert);

    if (frequencyRange.timestamp > lastTimestamp)
    {
        waterfallSprite.drawWaterfall(frequencyRange.rssi);
        lastTimestamp = frequencyRange.timestamp;
    }
    if (frequencyRange.settings->mute)
    {
        waterfallSprite.setTextColor(rssiToColor565(millis() % 100), TFT_TRANSPARENT, false);
        waterfallSprite.drawCentreString("НЕ АКТИВНО", millis() % 40 - 20 + waterfallSprite.width() / 2, waterfallSprite.height() / 2 - 20, 2);
    }
    if (context.calibMode > 0)
    {
        // Буфер для преобразования числа в строку
        char rssiRangeStr[10]; // достаточно для хранения числа до 65535 и завершающего нулевого символа
        // Преобразуем число в строку
        sprintf(rssiRangeStr, "%d", rssiRange.maxRssi);
        graphSprite.drawString(rssiRangeStr, 0, 0, 2);
        sprintf(rssiRangeStr, "%d", rssiRange.minRssi);
        graphSprite.drawString(rssiRangeStr, 0, graphSprite.height() / 2, 2);
    }
}

void RxWidget::draw()
{
    // Сначала очищаем спрайт

    // Рисуем рамку вокруг виджета, если он активен
    // TODO: Определить, является ли виджет активным, и установить соответствующий цвет рамки

    // Рисуем название и данные RSSI
    // TODO: Логика отрисовки названия, списка частот и диаграмм

    // Выводим спрайт на экран в соответствующее место
    bgSprite.pushSprite(0, STATUS_BAR_HEIGHT + instanceNumber * widgetHeight + (instanceNumber + 1) * SPACING, TFT_TRANSPARENT);
    graphSprite.pushSprite(LEFT_SIDE_WIDTH + 1, STATUS_BAR_HEIGHT + instanceNumber * widgetHeight + (instanceNumber + 1) * SPACING + 1);
    waterfallSprite.pushSprite(LEFT_SIDE_WIDTH + 1, STATUS_BAR_HEIGHT + instanceNumber * widgetHeight + (instanceNumber + 1) * SPACING + graphSprite.height() + 2);

    // линия между водопадом и столбиками
    tft->drawLine(LEFT_SIDE_WIDTH + 1, STATUS_BAR_HEIGHT + instanceNumber * widgetHeight + (instanceNumber + 1) * SPACING + graphSprite.height() + 1, TFT_WIDTH,
                  STATUS_BAR_HEIGHT + instanceNumber * widgetHeight + (instanceNumber + 1) * SPACING + graphSprite.height() + 1, borderColor);

    // три линии верх низ правый бок
    tft->drawLine(LEFT_SIDE_WIDTH / 2, STATUS_BAR_HEIGHT + instanceNumber * widgetHeight + (instanceNumber + 1) * SPACING,
                  TFT_WIDTH, STATUS_BAR_HEIGHT + instanceNumber * widgetHeight + (instanceNumber + 1) * SPACING, borderColor);
    tft->drawLine(LEFT_SIDE_WIDTH / 2, STATUS_BAR_HEIGHT + instanceNumber * widgetHeight + (instanceNumber + 1) * SPACING + widgetHeight - 1,
                  TFT_WIDTH, STATUS_BAR_HEIGHT + instanceNumber * widgetHeight + (instanceNumber + 1) * SPACING + widgetHeight - 1, borderColor);
    tft->drawLine(TFT_WIDTH - 1, STATUS_BAR_HEIGHT + instanceNumber * widgetHeight + (instanceNumber + 1) * SPACING, TFT_WIDTH - 1,
                  STATUS_BAR_HEIGHT + instanceNumber * widgetHeight + (instanceNumber + 1) * SPACING + widgetHeight - 1, borderColor);

    bgSprite.deleteSprite();
    graphSprite.deleteSprite();
}