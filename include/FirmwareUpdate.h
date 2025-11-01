#ifndef FIRMWARE_UPDATE_H
#define FIRMWARE_UPDATE_H

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Update.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include "config.h"

// Структура для информации о релизе
struct ReleaseInfo {
    String version;
    String releaseName;
    String releaseNotes;
    String downloadUrl;
    String publishedAt;
    bool isNewer;
};

class FirmwareUpdate {
public:
    FirmwareUpdate();
    ~FirmwareUpdate();

    // Основные методы
    bool init(AsyncWebServer* server);
    void loop();
    void shutdown();

    // Проверка обновлений
    bool checkForUpdates(ReleaseInfo& releaseInfo);
    ReleaseInfo getCurrentVersionInfo();
    
    // Настройки автообновления
    void setAutoUpdateEnabled(bool enabled);
    bool isAutoUpdateEnabled() const;
    void setDontOfferUpdates(bool dontOffer);
    bool isDontOfferUpdates() const;
    
    // Обновление
    void registerUpdateHandlers(AsyncWebServer* server);
    bool downloadAndInstallFirmware(const String& url);  // Public для safe mode
    
    // Статус
    bool isUpdating() const { return updating; }
    String getUpdateStatus() const { return updateStatus; }
    int getUpdateProgress() const { return currentProgress; }
    
    // Safe mode для OTA обновлений
    static bool isOTAPending();
    static void setOTAPending(bool pending);
    static void clearOTAPending();

private:
    enum class UpdateState {
        IDLE,
        DOWNLOADING,
        UPLOADING,
        SUCCESS,
        FAILED
    };

    // Внутренние методы для обработки загрузки
    void handleUpdateUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
    void handleUpdateStatus(AsyncWebServerRequest *request);
    void handleCheckUpdates(AsyncWebServerRequest *request);
    void handleCurrentVersion(AsyncWebServerRequest *request);
    void handleDownloadAndInstall(AsyncWebServerRequest *request);
    
    // Парсинг GitHub API
    bool parseGitHubRelease(const String& json, ReleaseInfo& releaseInfo);
    String extractJsonValue(const String& json, const String& key);
    bool isVersionNewer(const String& current, const String& latest);

    void updateProgress(int progress);

    // Состояние
    bool updating;
    UpdateState currentState;
    String updateStatus;
    
    // Статистика обновления
    size_t updateSize;
    size_t updateReceived;
    unsigned long updateStartTime;
    int currentProgress;
    
    // Отложенная перезагрузка
    bool shouldReboot;
    unsigned long rebootScheduledTime;
    
    // Preferences для настроек
    Preferences preferences;
    bool autoUpdateEnabled;
    bool dontOfferUpdates;
};

#endif // FIRMWARE_UPDATE_H