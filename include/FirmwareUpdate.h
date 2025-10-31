#ifndef FIRMWARE_UPDATE_H
#define FIRMWARE_UPDATE_H

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Update.h>
#include <esp_camera.h>
#include "config.h"

class FirmwareUpdate {
public:
    FirmwareUpdate();
    ~FirmwareUpdate();

    // Основные методы
    bool init();
    void loop();
    void shutdown();

    // Режимы обновления
    void startUpdateMode(); // Запуск режима обновления
    void exitUpdateMode();  // Выход из режима обновления

    // Статус
    bool isInUpdateMode() const { return updateModeActive; }
    bool isUpdating() const { return updating; }
    String getUpdateStatus() const { return updateStatus; }

private:
    enum class UpdateState {
        IDLE,
        WAITING_FOR_CLIENT,
        CLIENT_CONNECTED,
        UPLOADING,
        UPDATING,
        SUCCESS,
        FAILED
    };

    // Внутренние методы
    void initUpdateWebServer();
    void handleUpdateRoot(AsyncWebServerRequest *request);
    void handleUpdateUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
    void handleUpdateStatus(AsyncWebServerRequest *request);
    void handleUpdateRestart(AsyncWebServerRequest *request);

    void showUpdateScreen();
    void updateProgress(int progress);
    void showUpdateResult(bool success, const String& message);

    // Состояние
    bool updateModeActive;
    bool updating;
    UpdateState currentState;
    String updateStatus;
    
    // Веб-сервер для обновления
    AsyncWebServer* updateServer;
    
    // Статистика обновления
    size_t updateSize;
    size_t updateReceived;
    unsigned long updateStartTime;
    int currentProgress;
};

#endif // FIRMWARE_UPDATE_H