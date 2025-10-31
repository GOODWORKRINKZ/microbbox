#ifndef DISPLAY_SERVICE_H
#define DISPLAY_SERVICE_H

#include <TFT_eSPI.h>
#include <vector>
#include "Context/DisplayContext.h"
#include "Widgets/Widget.h"
#include "BaseService.h"
#include <RSSICalibrationData.h>

// Перечисление состояний отображения дисплея
enum class DisplayState
{
    BOOT, // Экран загрузки
    MAIN  // Основной экран интерфейса
};

class DisplayService : public BaseService<DisplayContext>
{
public:
    DisplayService(TFT_eSPI *tft, RSSICalibrationData *rssisCallibrationData);
    void init();
    void showBootScreen();
    void update(DisplayContext &context);
    unsigned long getUpdateInterval() override
    {
        return 1000 / 25; // обновление экрана с частотой 30 кадров в секунду
    }
    DisplayState currentState;

private:
    TFT_eSPI *tft;
    unsigned long lastChange = 0;
    const uint8_t BOOT_IMAGES_FRAMES = 9;
    const unsigned long bootFrameInterval = 300; // Интервал отображения загрузочного экрана
    const unsigned long bootInterval = 2000;     // Интервал отображения загрузочного экрана
    RSSICalibrationData *rssisCallibrationData;
    std::vector<Widget *> widgets; // Вектор указателей на виджеты
};

#endif // DISPLAY_SERVICE_H