#ifndef BASE_ROBOT_H
#define BASE_ROBOT_H

#include "IRobot.h"
#include "IMotorController.h"
#include "WiFiSettings.h"
#include "FirmwareUpdate.h"
#include <ESPAsyncWebServer.h>
#include <WiFi.h>

// ═══════════════════════════════════════════════════════════════
// БАЗОВЫЙ КЛАСС ДЛЯ ВСЕХ ТИПОВ РОБОТОВ
// ═══════════════════════════════════════════════════════════════
// Содержит общую функциональность, которая есть у всех роботов

class BaseRobot : public IRobot {
public:
    BaseRobot();
    virtual ~BaseRobot();
    
    // IRobot interface
    bool init() override;
    void update() override;
    void shutdown() override;
    bool isInitialized() const override { return initialized_; }
    void loop() override;
    
    IPAddress getIP() const override;
    String getDeviceName() const override;
    
protected:
    // Методы для наследников
    virtual bool initSpecificComponents() = 0;
    virtual void updateSpecificComponents() = 0;
    virtual void shutdownSpecificComponents() = 0;
    virtual void setupWebHandlers(AsyncWebServer* server) = 0;
    
    // Общие методы
    bool initWiFi();
    bool initWebServer();
    bool initMDNS();
    bool initCamera();
    
    void startWiFiAP();
    bool connectToSavedWiFi();
    bool connectWiFiDHCP(const char* ssid, const char* password);
    
    // Общий обработчик главной страницы
    void handleRoot(AsyncWebServerRequest* request);
    
    // Общие поля
    bool initialized_;
    bool cameraInitialized_;
    bool wifiConnected_;
    bool wifiAPMode_;
    String deviceName_;
    
    // Объекты
    AsyncWebServer* server_;
    WiFiSettings* wifiSettings_;
    FirmwareUpdate* firmwareUpdate_;
    IMotorController* motorController_;
};

#endif // BASE_ROBOT_H
