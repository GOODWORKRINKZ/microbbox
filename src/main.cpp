#include <Arduino.h>
#include "MicroBoxRobot.h"
#include "FirmwareUpdate.h"
#include "config.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

MicroBoxRobot* robot = nullptr;

void setup() {
    // КРИТИЧНО! Моторы ПЕРВЫМИ, ещё до Serial!
    // При подключении монитора DTR вызывает Reset, пины в floating - моторы крутятся!
    
    // Устанавливаем пины моторов в OUTPUT и LOW с pull-down максимально быстро
    pinMode(MOTOR_LEFT_FWD_PIN, OUTPUT);
    digitalWrite(MOTOR_LEFT_FWD_PIN, LOW);
    pinMode(MOTOR_LEFT_REV_PIN, OUTPUT);
    digitalWrite(MOTOR_LEFT_REV_PIN, LOW);
    pinMode(MOTOR_RIGHT_FWD_PIN, OUTPUT);
    digitalWrite(MOTOR_RIGHT_FWD_PIN, LOW);
    pinMode(MOTOR_RIGHT_REV_PIN, OUTPUT);
    digitalWrite(MOTOR_RIGHT_REV_PIN, LOW);
    
    // Теперь безопасно инициализировать Serial
    Serial.begin(115200);
    Serial.println("МикРоББокс запускается...");
    
    // Защита GPIO2 (strapping pin для NeoPixel)
    pinMode(NEOPIXEL_PIN, OUTPUT);
    digitalWrite(NEOPIXEL_PIN, LOW);
    
    // Отключение детектора сброса напряжения для предотвращения случайных перезагрузок
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
    
    // ПРОВЕРКА: Запущено ли ожидающее OTA обновление?
    if (FirmwareUpdate::isOTAPending()) {
        Serial.println("═══════════════════════════════════════");
        Serial.println("  ОБНАРУЖЕНО ОЖИДАЮЩЕЕ OTA ОБНОВЛЕНИЕ");
        Serial.println("  Загрузка в безопасном режиме...");
        Serial.println("═══════════════════════════════════════");
        
        // Создание робота и инициализация в безопасном режиме для OTA
        robot = new MicroBoxRobot();
        
        if (!robot->initSafeModeForOTA()) {
            Serial.println("ОШИБКА: Не удалось выполнить OTA обновление!");
            Serial.println("Перезагрузка в нормальном режиме через 5 секунд...");
            delay(5000);
            ESP.restart();
        }
        
        // Если мы здесь, OTA обновление завершено и устройство перезагрузится
        // Этот код не должен быть достигнут
        return;
    }
    
    // НОРМАЛЬНАЯ ЗАГРУЗКА: Полная инициализация со всеми компонентами
    Serial.println("═══════════════════════════════════════");
    Serial.println("  НОРМАЛЬНАЯ ЗАГРУЗКА");
    Serial.println("═══════════════════════════════════════");
    
    // Создание и инициализация робота
    robot = new MicroBoxRobot();
    
    if (!robot->init()) {
        Serial.println("ОШИБКА: Не удалось инициализировать робота!");
        while (1) {
            delay(1000);
        }
    }
    
    Serial.println("МикРоББокс готов к работе!");
    Serial.print("IP адрес: ");
    Serial.println(robot->getIP());
}

void loop() {
    if (robot) {
        robot->loop();
    }
    
    // Небольшая задержка для стабильности
    delay(10);
}
