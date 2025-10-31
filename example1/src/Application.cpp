#include "Application.h"
#include "startXTask.h"

Application::Application(TFT_eSPI &tft, RSSICalibrationData &rssiCalibData)
    : tft(tft), displayService{&tft, &rssiCalibData},
      aggregateContext{&radioContext, &displayContext},
      aggregateService(), buttonService(), buzzerService()
#if RADIO_1_2G_ENABLED
      ,
      radio12Service(rssiCalibData.band_1_2, rssiCalibData.calibMode)
#endif
#if RADIO_2_4G_ENABLED
      ,
      radio24Service(rssiCalibData.band_2_4, rssiCalibData.calibMode)
#endif
#if RADIO_5_8G_ENABLED
      ,
      radio58Service(rssiCalibData.band_5_8, rssiCalibData.calibMode)
#endif
      ,
      rssiCalibData_{rssiCalibData}
{
}

void Application::init()
{
    userSettings = loadSettings();
    initDisplayContext(displayContext, userSettings);
    displayContext.calibMode = (uint8_t)rssiCalibData_.calibMode;
    SPIDevice::initBus();
    pinMode(15, OUTPUT);
    pinMode(12, INPUT);
}

void Application::run()
{
    startXTask(displayService, displayContext, "DisplayTask", 2048 * 2);
    startXTask(buttonService, userSettings, "ButtonTask");
    startXTask(buzzerService, displayContext, "BuzzerTask");

    while (displayService.currentState == DisplayState::BOOT)
    {
        delay(10);
    }

#if RADIO_1_2G_ENABLED
    startXTask(radio12Service, radioContext, "Radio12Task");
#endif

#if RADIO_2_4G_ENABLED
    startXTask(radio24Service, radioContext, "Radio24Task");
#endif

#if RADIO_5_8G_ENABLED
    startXTask(radio58Service, radioContext, "Radio58Task");
#endif

    startXTask(aggregateService, aggregateContext, "AnalysisTask");
    startXTask(vBatService, displayContext, "VBatTask");
}