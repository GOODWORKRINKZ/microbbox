#include <Arduino.h>
#include <WiFi.h>
#include "Application.h"
#include "Widgets/QRCodeSprite.h" // Включаем наш новый класс для отображения QR-кода
#include "globals.h"              // Включаем глобальные определения
#include "FirmwareUpdateMode.h"   // Включаем класс для режима обновления
#include <startXTask.h>
#include <EEPROM.h>

TFT_eSPI tft = TFT_eSPI(); // Создаем объект TFT_eSPI
Application *app = nullptr;
FirmwareUpdateMode *firmwareUpdateMode = nullptr;
RSSICalibrationData rssiCalibData; // Глобальная переменная для rssiCalibData
ulong lastUpdate = 0;
void setup()
{
    Serial.begin(115200);
    Serial.println(APP_TARGET);
    EEPROM.begin(sizeof(Settings) + sizeof(RSSICalibrationData));
    pinMode(BUTTON_UP_PIN, INPUT);
    pinMode(BUTTON_DOWN_PIN, INPUT);

    if (digitalRead(BUTTON_UP_PIN) == LOW && digitalRead(BUTTON_DOWN_PIN) == LOW)
    {
        // Войти в режим прошивки
        firmwareUpdateMode = new FirmwareUpdateMode(tft);
        firmwareUpdateMode->init();
    }
    else
    {
        rssiCalibData = loadCalibrationData();
        app = new Application(tft, rssiCalibData);
        // Обычная загрузка
        app->init();
        app->run();
    }
}

void loop()
{
    if (firmwareUpdateMode != nullptr)
    {
        if (millis() - lastUpdate > firmwareUpdateMode->getUpdateInterval())
        {
            firmwareUpdateMode->update();
            lastUpdate = millis();
        }
    }
    else if (rssiCalibData.calibMode > 0)
    {
        if (millis() > 3 * 60 * 1000)
        {
            /*RSSICalibrationData storedData = loadCalibrationData();
            switch (rssiCalibData.calibMode)
            {
            case CALIB_MIN_RSSI:
                storedData.band_1_2.minRssi = rssiCalibData.band_1_2.minRssi;
                storedData.band_2_4.minRssi = rssiCalibData.band_2_4.minRssi;
                storedData.band_5_8.minRssi = rssiCalibData.band_5_8.minRssi;
                break;
            case CALIB_MAX_RSSI:
                storedData.band_1_2.maxRssi = rssiCalibData.band_1_2.maxRssi;
                storedData.band_2_4.maxRssi = rssiCalibData.band_2_4.maxRssi;
                storedData.band_5_8.maxRssi = rssiCalibData.band_5_8.maxRssi;
                break;

            default:
                break;
            }*/
            rssiCalibData.calibMode = CALIB_OFF;
            saveCalibrationData(rssiCalibData);
            ESP.restart();
        }
    }
}