#include <Arduino.h>
#include "MicroBoxRobot.h"
#include "config.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

MicroBoxRobot* robot = nullptr;

void setup() {
    Serial.begin(115200);
    Serial.println("МикроББокс запускается...");
    
    // ЭКСТРЕННАЯ ОСТАНОВКА МОТОРОВ И ЗАЩИТА GPIO перед инициализацией!
    // GPIO2 - strapping pin, должен быть LOW при загрузке (для NeoPixel это OK)
    pinMode(NEOPIXEL_PIN, OUTPUT);
    digitalWrite(NEOPIXEL_PIN, LOW);
    
    // Устанавливаем все пины моторов в LOW для предотвращения вращения при старте
    pinMode(MOTOR_LEFT_FWD_PIN, OUTPUT);
    pinMode(MOTOR_LEFT_REV_PIN, OUTPUT);
    pinMode(MOTOR_RIGHT_FWD_PIN, OUTPUT);
    pinMode(MOTOR_RIGHT_REV_PIN, OUTPUT);
    
    digitalWrite(MOTOR_LEFT_FWD_PIN, LOW);
    digitalWrite(MOTOR_LEFT_REV_PIN, LOW);
    digitalWrite(MOTOR_RIGHT_FWD_PIN, LOW);
    digitalWrite(MOTOR_RIGHT_REV_PIN, LOW);
    
    // Отключение детектора сброса напряжения для предотвращения случайных перезагрузок
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
    
    // Создание и инициализация робота
    robot = new MicroBoxRobot();
    
    if (!robot->init()) {
        Serial.println("ОШИБКА: Не удалось инициализировать робота!");
        while (1) {
            delay(1000);
        }
    }
    
    Serial.println("МикроББокс готов к работе!");
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
