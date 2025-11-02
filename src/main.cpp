#include <Arduino.h>
#include "target_config.h"
#include "hardware_config.h"
#include "IRobot.h"
#include "FirmwareUpdate.h"
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
        Serial.println("  Загрузка в безопасном режиме...");
        Serial.println("═══════════════════════════════════════");
        
        // TODO: Реализовать безопасный режим для OTA
        Serial.println("Безопасный режим OTA пока не реализован");
        Serial.println("Перезагрузка в нормальном режиме через 5 секунд...");
        delay(5000);
        ESP.restart();
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
    Serial.print(robot->getRobotType());
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
