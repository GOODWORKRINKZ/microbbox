#include "MX1508MotorController.h"
#include "WiFiSettings.h"
#include <Arduino.h>

#ifdef FEATURE_MOTORS

MX1508MotorController::MX1508MotorController() :
    initialized_(false),
    currentLeftSpeed_(0),
    currentRightSpeed_(0),
    lastCommandTime_(0),
    watchdogTriggered_(false),
    wifiSettings_(nullptr)
{
}

MX1508MotorController::~MX1508MotorController() {
    shutdown();
}

bool MX1508MotorController::init() {
    if (initialized_) {
        return true;
    }
    
    DEBUG_PRINTLN("Инициализация MX1508 Motor Controller...");
    
    // Настройка пинов моторов
    pinMode(MOTOR_LEFT_FWD_PIN, OUTPUT);
    pinMode(MOTOR_LEFT_REV_PIN, OUTPUT);
    pinMode(MOTOR_RIGHT_FWD_PIN, OUTPUT);
    pinMode(MOTOR_RIGHT_REV_PIN, OUTPUT);
    
    // Установка начального состояния (все LOW)
    digitalWrite(MOTOR_LEFT_FWD_PIN, LOW);
    digitalWrite(MOTOR_LEFT_REV_PIN, LOW);
    digitalWrite(MOTOR_RIGHT_FWD_PIN, LOW);
    digitalWrite(MOTOR_RIGHT_REV_PIN, LOW);
    
    // Настройка PWM каналов
    ledcSetup(MOTOR_PWM_CHANNEL_LF, MOTOR_PWM_FREQ, MOTOR_PWM_RESOLUTION);
    ledcSetup(MOTOR_PWM_CHANNEL_LR, MOTOR_PWM_FREQ, MOTOR_PWM_RESOLUTION);
    ledcSetup(MOTOR_PWM_CHANNEL_RF, MOTOR_PWM_FREQ, MOTOR_PWM_RESOLUTION);
    ledcSetup(MOTOR_PWM_CHANNEL_RR, MOTOR_PWM_FREQ, MOTOR_PWM_RESOLUTION);
    
    // Привязка каналов к пинам
    ledcAttachPin(MOTOR_LEFT_FWD_PIN, MOTOR_PWM_CHANNEL_LF);
    ledcAttachPin(MOTOR_LEFT_REV_PIN, MOTOR_PWM_CHANNEL_LR);
    ledcAttachPin(MOTOR_RIGHT_FWD_PIN, MOTOR_PWM_CHANNEL_RF);
    ledcAttachPin(MOTOR_RIGHT_REV_PIN, MOTOR_PWM_CHANNEL_RR);
    
    // Установка начальной скорости (0)
    ledcWrite(MOTOR_PWM_CHANNEL_LF, 0);
    ledcWrite(MOTOR_PWM_CHANNEL_LR, 0);
    ledcWrite(MOTOR_PWM_CHANNEL_RF, 0);
    ledcWrite(MOTOR_PWM_CHANNEL_RR, 0);
    
    initialized_ = true;
    DEBUG_PRINTLN("MX1508 Motor Controller инициализирован");
    return true;
}

void MX1508MotorController::update() {
    if (!initialized_) {
        return;
    }
    
    // Проверка таймаута команд (watchdog)
    if (lastCommandTime_ > 0 && (millis() - lastCommandTime_ > MOTOR_COMMAND_TIMEOUT_MS)) {
        // Автоостановка моторов если нет команд
        if (currentLeftSpeed_ != 0 || currentRightSpeed_ != 0) {
            DEBUG_PRINTLN("Motor watchdog: остановка моторов");
            watchdogTriggered_ = true;  // Устанавливаем флаг срабатывания watchdog
            stop();
            // Не сбрасываем флаг здесь - он будет сброшен при следующей команде
        }
    }
}

void MX1508MotorController::shutdown() {
    if (initialized_) {
        stop();
        initialized_ = false;
    }
}

void MX1508MotorController::setSpeed(int leftSpeed, int rightSpeed) {
    if (!initialized_) {
        return;
    }
    
    // Ограничение скорости
    leftSpeed = constrainSpeed(leftSpeed);
    rightSpeed = constrainSpeed(rightSpeed);
    
    applyMotorSpeed(leftSpeed, rightSpeed);
    
    currentLeftSpeed_ = leftSpeed;
    currentRightSpeed_ = rightSpeed;
    lastCommandTime_ = millis();
    watchdogTriggered_ = false;  // Сбрасываем флаг при получении новой команды
}

void MX1508MotorController::setMotorPWM(int throttlePWM, int steeringPWM) {
    if (!initialized_) {
        return;
    }
    
    // Преобразование PWM (1000-2000) в скорость (-100 до +100)
    // 1500 = центр (0 скорость)
    int throttle = map(throttlePWM, 1000, 2000, -100, 100);
    int steering = map(steeringPWM, 1000, 2000, -100, 100);
    
    // Дифференциальное управление
    int leftSpeed = throttle + steering;
    int rightSpeed = throttle - steering;
    
    DEBUG_PRINTF("BEFORE settings: L=%d R=%d", leftSpeed, rightSpeed);
    
    // Применяем настройки моторов из WiFiSettings
    if (wifiSettings_) {
        DEBUG_PRINTF("Motor settings: swap=%d invertL=%d invertR=%d", 
                     wifiSettings_->getMotorSwapLeftRight(),
                     wifiSettings_->getMotorInvertLeft(),
                     wifiSettings_->getMotorInvertRight());
        
        // ВАЖНО: Сначала применяем инверсию, ПОТОМ swap
        // Инверсия применяется к логическим левому/правому моторам
        if (wifiSettings_->getMotorInvertLeft()) {
            leftSpeed = -leftSpeed;
            DEBUG_PRINTLN("Applied LEFT invert");
        }
        
        if (wifiSettings_->getMotorInvertRight()) {
            rightSpeed = -rightSpeed;
            DEBUG_PRINTLN("Applied RIGHT invert");
        }
        
        // Меняем местами левый и правый ПОСЛЕ инверсии
        // Теперь инвертированные настройки идут вместе с моторами
        if (wifiSettings_->getMotorSwapLeftRight()) {
            int temp = leftSpeed;
            leftSpeed = rightSpeed;
            rightSpeed = temp;
            DEBUG_PRINTLN("Applied SWAP");
        }
    } else {
        DEBUG_PRINTLN("WARNING: wifiSettings_ is NULL!");
    }
    
    // Детальные логи для диагностики
    DEBUG_PRINTF("AFTER settings: L=%d R=%d", leftSpeed, rightSpeed);
    
    setSpeed(leftSpeed, rightSpeed);
}

void MX1508MotorController::stop() {
    if (!initialized_) {
        return;
    }
    
    // Note: We deliberately do NOT clear watchdogTriggered_ flag here.
    // If watchdog triggered before this stop(), the flag should remain set
    // so that the next motor command will be forced to apply.
    // The flag is only cleared when a new motor command is received via setSpeed().
    
    ledcWrite(MOTOR_PWM_CHANNEL_LF, 0);
    ledcWrite(MOTOR_PWM_CHANNEL_LR, 0);
    ledcWrite(MOTOR_PWM_CHANNEL_RF, 0);
    ledcWrite(MOTOR_PWM_CHANNEL_RR, 0);
    
    currentLeftSpeed_ = 0;
    currentRightSpeed_ = 0;
    // Reset lastCommandTime_ to prevent watchdog from firing repeatedly.
    // Watchdog will only fire again after new commands arrive and another timeout occurs.
    lastCommandTime_ = 0;
}

void MX1508MotorController::getCurrentSpeed(int& leftSpeed, int& rightSpeed) const {
    leftSpeed = currentLeftSpeed_;
    rightSpeed = currentRightSpeed_;
}

void MX1508MotorController::applyMotorSpeed(int leftSpeed, int rightSpeed) {
    // Расчет максимального значения PWM с учетом ограничения мощности
    const int maxPWM = (1 << MOTOR_PWM_RESOLUTION) - 1; // 8191 для 13-бит
    const int limitedMaxPWM = (maxPWM * MOTOR_MAX_POWER_PERCENT) / 100;
    
    int leftPWM = 0;
    int rightPWM = 0;
    
    // Левый мотор
    if (leftSpeed > 0) {
        leftPWM = map(leftSpeed, 0, 100, 0, limitedMaxPWM);
        ledcWrite(MOTOR_PWM_CHANNEL_LF, leftPWM);
        ledcWrite(MOTOR_PWM_CHANNEL_LR, 0);
    } else if (leftSpeed < 0) {
        leftPWM = map(-leftSpeed, 0, 100, 0, limitedMaxPWM);
        ledcWrite(MOTOR_PWM_CHANNEL_LF, 0);
        ledcWrite(MOTOR_PWM_CHANNEL_LR, leftPWM);
    } else {
        ledcWrite(MOTOR_PWM_CHANNEL_LF, 0);
        ledcWrite(MOTOR_PWM_CHANNEL_LR, 0);
    }
    
    // Правый мотор
    if (rightSpeed > 0) {
        rightPWM = map(rightSpeed, 0, 100, 0, limitedMaxPWM);
        ledcWrite(MOTOR_PWM_CHANNEL_RF, rightPWM);
        ledcWrite(MOTOR_PWM_CHANNEL_RR, 0);
    } else if (rightSpeed < 0) {
        rightPWM = map(-rightSpeed, 0, 100, 0, limitedMaxPWM);
        ledcWrite(MOTOR_PWM_CHANNEL_RF, 0);
        ledcWrite(MOTOR_PWM_CHANNEL_RR, rightPWM);
    } else {
        ledcWrite(MOTOR_PWM_CHANNEL_RF, 0);
        ledcWrite(MOTOR_PWM_CHANNEL_RR, 0);
    }
    
    DEBUG_PRINTF("Motor PWM: L=%d (%s) R=%d (%s) [max=%d]\n", 
                 leftPWM, leftSpeed > 0 ? "FWD" : (leftSpeed < 0 ? "REV" : "STOP"),
                 rightPWM, rightSpeed > 0 ? "FWD" : (rightSpeed < 0 ? "REV" : "STOP"),
                 limitedMaxPWM);
}

int MX1508MotorController::constrainSpeed(int speed) const {
    return constrain(speed, -100, 100);
}

bool MX1508MotorController::wasWatchdogTriggered() const {
    return watchdogTriggered_;
}

#endif // FEATURE_MOTORS
