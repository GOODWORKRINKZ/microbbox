#include "Widgets/StatusBarWidget.h"
#include "Fonts/FontDejaVu20.h"
#include "globals.h"
#include "RSSICalibrationData.h"

#define BG_FLICK_TIME 500

StatusBarWidget::StatusBarWidget(TFT_eSPI *display)
    : Widget(display),
      bgSprite(TFT_eSprite(display)),
      deviceNameSprite(TFT_eSprite(display)), // Передаем уже существующий экземпляр TFT_eSPI
      notificationSprite(TFT_eSprite(display)),
      batterySprite(TFT_eSprite(display)),
      speakerSprite(TFT_eSprite(display))
{
    bgSprite.createSprite(TFT_WIDTH, STATUS_BAR_HEIGHT);
    TFT_eSprite tmp = TFT_eSprite(display);
    tmp.createSprite(TFT_WIDTH, STATUS_BAR_HEIGHT);
    tmp.loadFont(FontDejaVu20);
    deviceNamePartWidth = tmp.textWidth(DEVICE_NAME, 2) + PADING;
    tmp.unloadFont();
    tmp.deleteSprite();
    deviceNameSprite.createSprite(deviceNamePartWidth, STATUS_BAR_HEIGHT);
    deviceNameSprite.fillScreen(TFT_TRANSPARENT);
    deviceNameSprite.setTextColor(TFT_WHITE, TFT_TRANSPARENT);
    notificationSprite.createSprite(TFT_WIDTH, STATUS_BAR_HEIGHT);
    notificationSprite.fillScreen(TFT_TRANSPARENT);
    notificationSprite.setTextColor(TFT_WHITE, TFT_TRANSPARENT);
    batterySprite.createSprite(BATTERY_PART_WIDTH, STATUS_BAR_HEIGHT);
    deviceNameSprite.loadFont(FontDejaVu20);
    notificationSprite.loadFont(FontDejaVu20);
}

StatusBarWidget::~StatusBarWidget()
{
    // Удаление спрайтов
    deviceNameSprite.deleteSprite();
    notificationSprite.deleteSprite();
    batterySprite.deleteSprite();
}

void StatusBarWidget::update(DisplayContext &context)
{
    if (lastState != context.state || (lastState && (millis() - lastBgFlick > BG_FLICK_TIME)))
    {
        lastState = context.state;
        isBgInvalidate = true;
        if (context.calibMode == CALIB_OFF)
        {
            drawDeviceName();
        }
        drawBg();
        lastBgFlick = millis();
    }
    switch (context.calibMode)
    {
    case CALIB_MIN_RSSI:
        deviceNameSprite.fillScreen(TFT_BLACK);
        isBgInvalidate = true;
        deviceNameSprite.drawNumber(3 * 60 - millis() / 1000, 0, 0, 2);
        drawNotification("КАЛ. МИН");
        break;
    case CALIB_MAX_RSSI:
        deviceNameSprite.fillScreen(TFT_BLACK);
        isBgInvalidate = true;
        deviceNameSprite.drawNumber(3 * 60 - millis() / 1000, 0, 0, 2);
        drawNotification("КАЛ. МАКС");
        break;
    default:
        drawNotification(context.notification);
        break;
    }
    drawBattery(context.voltage);
}

void StatusBarWidget::draw()
{
    if (isBgInvalidate)
    {
        isBgInvalidate = false;
        deviceNameSprite.pushToSprite(&bgSprite, 2, 2, TFT_TRANSPARENT);
    }
    notificationSprite.pushToSprite(&bgSprite, deviceNameSprite.width() + 1, 2, TFT_TRANSPARENT);
    batterySprite.pushToSprite(&bgSprite, TFT_WIDTH - BATTERY_PART_WIDTH, 0, TFT_TRANSPARENT);
    bgSprite.pushSprite(0, 0);
}

void StatusBarWidget::drawDeviceName()
{
    deviceNameSprite.fillScreen(TFT_BLACK);
    deviceNameSprite.drawString(DEVICE_NAME, 0, 0, 2);
}

void StatusBarWidget::drawBg()
{
    uint16_t foregroundColor = TFT_BLACK; // Цвет фона статуса
    if (lastState != 0)
    {
        foregroundColor = wrongStateFlockFlag ? TFT_RED : 0x98a3;
        wrongStateFlockFlag = wrongStateFlockFlag ? false : true;
        // Serial.println(foregroundColor);
    }
    bgSprite.fillRoundRect(deviceNameSprite.width() + PADING * 2, 0, TFT_WIDTH - BATTERY_PART_WIDTH - PADING * 2 - deviceNameSprite.width() - PADING * 2, STATUS_BAR_HEIGHT, 6, foregroundColor);
    bgSprite.drawRoundRect(deviceNameSprite.width() + PADING * 2, 0, TFT_WIDTH - BATTERY_PART_WIDTH - PADING * 2 - deviceNameSprite.width() - PADING * 2, STATUS_BAR_HEIGHT, 5, TFT_WHITE);
    bgSprite.drawLine(deviceNameSprite.width() - 1, 2, deviceNameSprite.width() - 1, STATUS_BAR_HEIGHT - 3, TFT_BLACK);
}

void StatusBarWidget::drawNotification(const char *notification)
{
    notificationSprite.fillScreen(TFT_TRANSPARENT);
    int16_t textWidth = notificationSprite.textWidth(notification, 1);                                                                                                       // Получаем ширину текста
    int16_t x = (((deviceNameSprite.width() + PADING * 2) * 2 + (TFT_WIDTH - BATTERY_PART_WIDTH - PADING * 2 - deviceNameSprite.width() - PADING * 2)) / 2 - textWidth) / 2; // Рассчитываем координаты X для центрирования 71+124
    notificationSprite.drawString(notification, x, 0, 1);
}

void StatusBarWidget::drawBattery(float batteryLevel)
{
    batterySprite.fillScreen(TFT_TRANSPARENT);
    // Константы для размеров батареи
    const int batteryWidth = BATTERY_PART_WIDTH - 15;             // Увеличенная ширина батареи
    const int batteryHeight = 18;                                 // Увеличенная высота батареи
    const int batteryX = (BATTERY_PART_WIDTH - batteryWidth) / 2; // X координата для центрирования батареи
    const int batteryY = (STATUS_BAR_HEIGHT - batteryHeight) / 2; // Y координата для центрирования батареи
    const int borderThickness = 2;                                // Толщина рамки
    const int terminalWidth = 4;                                  // Ширина клеммы батареи
    const int terminalHeight = 8;                                 // Высота клеммы батареи

    // Преобразуем напряжение в уровень заряда
    int batteryLevelMapped = map(batteryLevel * 100, 600, 810, 0, 100);
    if (batteryLevelMapped > 100)
        batteryLevelMapped = 100;
    if (batteryLevelMapped < 0)
        batteryLevelMapped = 0;

    // Интерполяция цвета от красного к зеленому через желтый
    uint8_t r = (batteryLevelMapped < 50) ? 255 : map(batteryLevelMapped, 50, 100, 255, 0);
    uint8_t g = (batteryLevelMapped < 50) ? map(batteryLevelMapped, 0, 50, 0, 255) : 255;
    uint8_t b = 0;
    uint16_t fillColor = tft->color565(r, g, b);

    // Нарисовать контур батареи
    batterySprite.drawRect(batteryX, batteryY, batteryWidth, batteryHeight, TFT_WHITE);
    // Нарисовать клемму батареи
    batterySprite.fillRect(batteryX + batteryWidth, batteryY + (batteryHeight - terminalHeight) / 2, terminalWidth, terminalHeight, TFT_WHITE);

    // Рассчитать ширину заполненной части
    int filledWidth = (batteryWidth - 2 * borderThickness) * batteryLevelMapped / 100.0;

    // Нарисовать заполненную часть батареи
    batterySprite.fillRect(batteryX + 1, batteryY + 1, filledWidth + 2, batteryHeight - 1 * borderThickness, fillColor);
    if (batteryLevelMapped < 100)
    {
        batterySprite.fillRect(batteryX + filledWidth + 1, batteryY + 1, (batteryWidth - 2 * borderThickness) - filledWidth + 1, batteryHeight - 1 * borderThickness, TFT_DARKGREY);
    }
}