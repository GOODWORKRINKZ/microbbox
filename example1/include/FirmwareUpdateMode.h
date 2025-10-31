#ifndef FIRMWARE_UPDATE_MODE_H
#define FIRMWARE_UPDATE_MODE_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <TFT_eSPI.h>
#include "qr_codes.h"
#include "embedded_resources.h"
#include "globals.h"
#include "Widgets/QRCodeSprite.h"
#include "Fonts/FontDejaVu20.h"
#include "Services/BaseService.h"
#include <DNSServer.h>
#include "hardware_config.h"
#include "RSSICalibrationData.h"

enum class UpdateState {
    WAIT_FOR_CLIENT,
    CLIENT_CONNECTED,
    UPLOADING_FILE,
    UPDATING,
    UPDATE_FAILED,
    UPDATE_SUCCESS
};

class FirmwareUpdateMode {
public:
    FirmwareUpdateMode(TFT_eSPI &tft);
    void init();
    void update();
    unsigned long getUpdateInterval();

private:
    TFT_eSPI &tft;
    DNSServer dnsServer;  // Добавляем DNSServer
    AsyncWebServer server;
    UpdateState state;

    void showWiFiScreen();
    void showWebLinkScreen();
    void initServer();

    void setState(UpdateState newState);
    void displayUpdateSuccess();
    void displayUpdateFailed();
    void displayResetEffect();  // Новый метод для отображения эффекта перезагрузки
    void drawProgressBar(float progress);

    // Обработчики маршрутов
    static void handleFilinupRequest(AsyncWebServerRequest *request);
    static void handleLogoRequest(AsyncWebServerRequest *request);
    static void handleFaviconRequest(AsyncWebServerRequest *request);
    static void handleUpdateRequest(AsyncWebServerRequest *request);
    static void handleUpdateUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
    static void handleStylesRequest(AsyncWebServerRequest *request);
    static void handleScriptRequest(AsyncWebServerRequest *request);

    // Обработчики событий
    void userConnected();
    void userDisconnected();
    void fileUploaded();
    void updateSuccess();
    void updateFailed();

    // Методы для обработки кнопок
    void checkButtons();
    void handleCalibMode(CalibMode mode);
    void handleReboot();

    static FirmwareUpdateMode* instance;  // Для доступа к текущему экземпляру
};

#endif // FIRMWARE_UPDATE_MODE_H