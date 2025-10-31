#ifndef STATUS_BAR_WIDGET_H
#define STATUS_BAR_WIDGET_H

#include <TFT_eSPI.h>
#include "Widget.h" // Базовый класс виджета
#include "Context/DisplayContext.h"
#include "common.h"

class StatusBarWidget : public Widget
{
public:
    StatusBarWidget(TFT_eSPI *tft);
    ~StatusBarWidget();

    void update(DisplayContext &context) override;
    void draw() override;

private:
    TFT_eSprite deviceNameSprite;
    TFT_eSprite notificationSprite;
    TFT_eSprite batterySprite;
    TFT_eSprite speakerSprite;
    TFT_eSprite bgSprite;
    uint8_t lastState = 0;
    bool wrongStateFlockFlag = 0;
    bool isBgInvalidate = 0;
    int deviceNamePartWidth = 0;
    ulong lastBgFlick = 0;

    // Внутренние функции для отрисовки каждой части виджета
    void drawDeviceName();
    void drawBg();
    void drawNotification(const char *notification);
    void drawBattery(float batteryLevel);
};

#endif // STATUS_BAR_WIDGET_H