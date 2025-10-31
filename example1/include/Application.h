#ifndef APPLICATION_H
#define APPLICATION_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "Services/BaseService.h"
#include "Services/DisplayService.h"
#include "Context/DisplayContext.h"
#include "Services/VBatService.h"
#include "Services/AggregateService.h"
#include "SPIDevice.h"
#include "Services/ButtonService.h"
#include "Services/BuzzerService.h"
#include "Settings.h"

#if RADIO_1_2G_ENABLED
#include "Services/Radio12Service.h"
#endif

#if RADIO_2_4G_ENABLED
#include "Services/Radio24Service.h"
#endif

#if RADIO_5_8G_ENABLED
#include "Services/Radio58Service.h"
#endif

class Application
{
public:
    Application(TFT_eSPI &tft, RSSICalibrationData &rssiCalibData);
    void init();
    void run();

    DisplayService &getDisplayService() { return displayService; }

private:
    TFT_eSPI &tft;
    RSSICalibrationData &rssiCalibData_;
    DisplayService displayService;
    VBatService vBatService;
    DisplayContext displayContext;
    Settings userSettings;
    RadioContext radioContext;
    AggregateContext aggregateContext;
    AggregateService aggregateService;
    ButtonService buttonService;
    BuzzerService buzzerService;

#if RADIO_1_2G_ENABLED
    Radio12Service radio12Service;
#endif

#if RADIO_2_4G_ENABLED
    Radio24Service radio24Service;
#endif

#if RADIO_5_8G_ENABLED
    Radio58Service radio58Service;
#endif
};

#endif // APPLICATION_H