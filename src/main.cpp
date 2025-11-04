#include <Arduino.h>
#include "target_config.h"
#include "hardware_config.h"
#include "IRobot.h"
#include "FirmwareUpdate.h"
#include "WiFiSettings.h"
#include <Preferences.h>
#include <ESPAsyncWebServer.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// Включаем правильную реализацию робота в зависимости от таргета
#ifdef TARGET_CLASSIC
#include "ClassicRobot.h"
#endif

#ifdef TARGET_LINER
#include "LinerRobot.h"
#endif

#ifdef TARGET_BRAIN
#include "BrainRobot.h"
#endif

IRobot* robot = nullptr;

void setup() {
    // КРИТИЧНО! Моторы ПЕРВЫМИ, ещё до Serial!
    // При подключении монитора DTR вызывает Reset, пины в floating - моторы крутятся!
    
#ifdef FEATURE_MOTORS
    // Устанавливаем пины моторов в OUTPUT и LOW с pull-down максимально быстро
    pinMode(MOTOR_LEFT_FWD_PIN, OUTPUT);
    digitalWrite(MOTOR_LEFT_FWD_PIN, LOW);
    pinMode(MOTOR_LEFT_REV_PIN, OUTPUT);
    digitalWrite(MOTOR_LEFT_REV_PIN, LOW);
    pinMode(MOTOR_RIGHT_FWD_PIN, OUTPUT);
    digitalWrite(MOTOR_RIGHT_FWD_PIN, LOW);
    pinMode(MOTOR_RIGHT_REV_PIN, OUTPUT);
    digitalWrite(MOTOR_RIGHT_REV_PIN, LOW);
#endif
    
    // Теперь безопасно инициализировать Serial
    Serial.begin(115200);
    Serial.println("═══════════════════════════════════════");
    Serial.println("  МикРоББокс запускается...");
    Serial.print("  Тип: ");
    Serial.println(ROBOT_NAME);
    Serial.println("═══════════════════════════════════════");
    
#ifdef FEATURE_NEOPIXEL
    // Защита GPIO2 (strapping pin для NeoPixel)
    pinMode(NEOPIXEL_PIN, OUTPUT);
    digitalWrite(NEOPIXEL_PIN, LOW);
#endif
    
    // Отключение детектора сброса напряжения для предотвращения случайных перезагрузок
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
    
    // ПРОВЕРКА: Запущено ли ожидающее OTA обновление?
    if (FirmwareUpdate::isOTAPending()) {
        Serial.println("═══════════════════════════════════════");
        Serial.println("  ОБНАРУЖЕНО ОЖИДАЮЩЕЕ OTA ОБНОВЛЕНИЕ");
        Serial.println("  Запуск безопасного режима OTA...");
        Serial.println("═══════════════════════════════════════");
        
        // Получаем URL обновления
        Preferences otaPrefs;
        if (!otaPrefs.begin("ota", true)) {
            Serial.println("ОШИБКА: Не удалось открыть preferences для чтения URL");
            FirmwareUpdate::clearOTAPending();
        } else {
            String updateUrl = otaPrefs.getString("url", "");
            otaPrefs.end();
            
            if (updateUrl.length() == 0) {
                Serial.println("ОШИБКА: URL обновления не найден");
                FirmwareUpdate::clearOTAPending();
            } else {
                Serial.printf("URL обновления: %s\n", updateUrl.c_str());
                
                // Инициализируем минимальный WiFi для OTA
                Serial.println("Инициализация WiFi для OTA...");
                
                // Загружаем WiFi настройки
                WiFiSettings* wifiSettings = new WiFiSettings();
                if (!wifiSettings->init()) {
                    Serial.println("ОШИБКА: Не удалось загрузить WiFi настройки");
                    FirmwareUpdate::clearOTAPending();
                    delete wifiSettings;
                } else {
                    // Подключаемся к WiFi
                    if (wifiSettings->getMode() == WiFiMode::AP) {
                        Serial.println("ОШИБКА: OTA невозможно в режиме AP");
                        FirmwareUpdate::clearOTAPending();
                        delete wifiSettings;
                    } else {
                        WiFi.mode(WIFI_STA);
                        WiFi.begin(wifiSettings->getSSID().c_str(), wifiSettings->getPassword().c_str());
                        
                        Serial.print("Подключение к WiFi");
                        int attempts = 0;
                        while (WiFi.status() != WL_CONNECTED && attempts < 30) {
                            delay(500);
                            Serial.print(".");
                            attempts++;
                        }
                        Serial.println();
                        
                        if (WiFi.status() != WL_CONNECTED) {
                            Serial.println("ОШИБКА: Не удалось подключиться к WiFi");
                            FirmwareUpdate::clearOTAPending();
                            delete wifiSettings;
                        } else {
                            Serial.print("WiFi подключен. IP: ");
                            Serial.println(WiFi.localIP());
                            
                            // Создаем минимальный веб-сервер для OTA (как в старом коде)
                            AsyncWebServer* server = new AsyncWebServer(80);
                            
                            // Создаем объект FirmwareUpdate и регистрируем OTA endpoint'ы
                            FirmwareUpdate* firmwareUpdate = new FirmwareUpdate();
                            firmwareUpdate->init(server);
                            
                            // Добавляем простой индикатор статуса
                            server->on("/", HTTP_GET, [](AsyncWebServerRequest *request){
                                request->send(200, "text/html",
                                    "<html><body><h1>OTA Update Mode</h1>"
                                    "<p>Device is in safe mode for firmware update.</p>"
                                    "<p>Please wait while update completes...</p>"
                                    "</body></html>");
                            });
                            
                            server->begin();
                            Serial.println("Минимальный веб-сервер запущен");
                            
                            // Запускаем обновление напрямую
                            Serial.println("Запуск загрузки и установки прошивки...");
                            bool success = firmwareUpdate->downloadAndInstallFirmware(updateUrl);
                            
                            // Очищаем флаг OTA
                            FirmwareUpdate::clearOTAPending();
                            
                            if (success) {
                                Serial.println("═══════════════════════════════════════");
                                Serial.println("  ОБНОВЛЕНИЕ УСПЕШНО ЗАВЕРШЕНО!");
                                Serial.println("  Перезагрузка через 2 секунды...");
                                Serial.println("═══════════════════════════════════════");
                            } else {
                                Serial.println("═══════════════════════════════════════");
                                Serial.println("  ОШИБКА ПРИ ОБНОВЛЕНИИ ПРОШИВКИ");
                                Serial.println("  Перезагрузка в нормальном режиме...");
                                Serial.println("═══════════════════════════════════════");
                            }
                            
                            delay(2000);
                            ESP.restart();
                            
                            // Cleanup (unreachable после ESP.restart())
                            delete firmwareUpdate;
                            delete server;
                        }
                        delete wifiSettings;
                    }
                }
            }
        }
        
        // После обработки OTA продолжаем обычную загрузку
        Serial.println("Продолжение обычной загрузки...");
    }
    
    // НОРМАЛЬНАЯ ЗАГРУЗКА: Создание робота нужного типа
    Serial.println("═══════════════════════════════════════");
    Serial.println("  НОРМАЛЬНАЯ ЗАГРУЗКА");
    Serial.println("═══════════════════════════════════════");
    
    // Фабричный паттерн: создаем нужный тип робота
#ifdef TARGET_CLASSIC
    robot = new ClassicRobot();
#endif

#ifdef TARGET_LINER
    robot = new LinerRobot();
#endif

#ifdef TARGET_BRAIN
    robot = new BrainRobot();
#endif

    if (!robot) {
        Serial.println("ОШИБКА: Не удалось создать робота!");
        while (1) {
            delay(1000);
        }
    }
    
    // Инициализация робота
    if (!robot->init()) {
        Serial.println("ОШИБКА: Не удалось инициализировать робота!");
        while (1) {
            delay(1000);
        }
    }
    
    Serial.println("═══════════════════════════════════════");
    Serial.print("  ");
    Serial.print(robot->getRobotTypeString());
    Serial.println(" робот готов к работе!");
    Serial.print("  IP адрес: ");
    Serial.println(robot->getIP());
    Serial.print("  Имя: ");
    Serial.println(robot->getDeviceName());
    Serial.println("═══════════════════════════════════════");
}

void loop() {
    if (robot) {
        robot->loop();
    }
    
    // Небольшая задержка для стабильности
    delay(10);
}
