#ifndef MICROBOX_ROBOT_H
#define MICROBOX_ROBOT_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <esp_camera.h>
#include <Adafruit_NeoPixel.h>
#include "config.h"
#include "FirmwareUpdate.h"

class MicroBoxRobot {
public:
    MicroBoxRobot();
    ~MicroBoxRobot();

    // Основные методы
    bool init();
    void loop();
    void shutdown();

    // WiFi управление
    void startWiFiAP();
    void connectWiFiDHCP(const char* ssid, const char* password);
    bool isWiFiConnected();
    IPAddress getIP();

    // Камера
    bool initCamera();
    void startCameraServer();

    // Моторы
    void setMotorSpeed(int leftSpeed, int rightSpeed);  // -255 до 255
    void moveForward(int speed);
    void moveBackward(int speed);
    void turnLeft(int speed);
    void turnRight(int speed);
    void stopMotors();

    // Светодиоды
    void setLEDColor(int ledIndex, uint32_t color);
    void setAllLEDs(uint32_t color);
    void clearLEDs();
    void updateLEDs();
    
    // Эффекты
    void setEffectMode(EffectMode mode);
    void playPoliceEffect();
    void playFireEffect();
    void playAmbulanceEffect();
    void playMovementAnimation();

    // Бузер
    void playTone(int frequency, int duration);
    void playMelody(const int* melody, const int* noteDurations, int noteCount);
    void stopBuzzer();

    // Управление
    void setControlMode(ControlMode mode);
    void processControlInput(int leftX, int leftY, int rightX, int rightY);

    // Статус
    bool isInitialized() const { return initialized; }
    EffectMode getCurrentEffectMode() const { return currentEffectMode; }
    ControlMode getCurrentControlMode() const { return currentControlMode; }
    
    // Система обновления
    void enterUpdateMode();
    bool isInUpdateMode() const;

private:
    // Внутренние методы
    void initMotors();
    void initLEDs();
    void initBuzzer();
    void initWebServer();
    
    void handleRoot(AsyncWebServerRequest *request);
    void handleStream(AsyncWebServerRequest *request);
    void handleCommand(AsyncWebServerRequest *request);
    void handleNotFound(AsyncWebServerRequest *request);

    // Состояние
    bool initialized;
    bool cameraInitialized;
    bool wifiConnected;
    bool wifiAPMode;
    
    // Объекты
    AsyncWebServer* server;
    Adafruit_NeoPixel* pixels;
    FirmwareUpdate* firmwareUpdate;
    
    // Режимы
    ControlMode currentControlMode;
    EffectMode currentEffectMode;
    
    // Состояние моторов
    int currentLeftSpeed;
    int currentRightSpeed;
    
    // Состояние эффектов
    unsigned long lastEffectUpdate;
    bool effectState;
    
    // Timing
    unsigned long lastLoop;
};

#endif // MICROBOX_ROBOT_H