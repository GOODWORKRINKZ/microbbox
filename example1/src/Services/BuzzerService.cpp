#define NOTE_B5 988
#define NOTE_G5 784
#define NOTE_E4 329
#define NOTE_G4 392
#define NOTE_A4 440
#define NOTE_B4 493
#define NOTE_G4 392
#define NOTE_B5 988
#define NOTE_C5 523
#define NOTE_D5 587
#define NOTE_D4 294
#define NOTE_F4 349
#define NOTE_G4 392
#define NOTE_A4 440
// Константы для управления интервалами работы и паузы мотора
const unsigned long MOTOR_ON_INTERVAL = 500;  // Время работы мотора, мс
const unsigned long MOTOR_OFF_INTERVAL = 300; // Время паузы мотора, мс
#include "Services/BuzzerService.h"

BuzzerService::BuzzerService() : lastUpdate(0), duration(0), motorState(false)
{
    // Конструктор
}

void BuzzerService::init()
{
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(MOTOR_PIN, OUTPUT);
    playInitTune();
    digitalWrite(MOTOR_PIN, HIGH);
    lastMotorActionTime = millis();
}

void BuzzerService::update(DisplayContext &context)
{
    unsigned long currentTime = millis();
    controlMotor(false);

    if (context.state == (uint8_t)DeviceState::WARN)
    {
        if ((currentTime - lastUpdate) < duration)
        {
            return;
        }

        duration = map(context.maxRssi, 0, 100, 3000, 100); // Чем выше значение, тем короче длительность
        lastUpdate = currentTime;

        // Издаём ноты "БИ - БУ"
        controlMotor(true);
        playTone(NOTE_B5, std::min(duration / 4, 50)); // "БИ"
        delay(duration / 4);
        playTone(NOTE_G5, std::min(duration / 4, 50)); // "БУ"
        delay(duration / 4);
    }
    else
    {
        noTone(BUZZER_PIN); // Останавливаем звук, если состояние не WARN
        digitalWrite(MOTOR_PIN, LOW);
    }
}

unsigned long BuzzerService::getUpdateInterval()
{
    return 50; // Обновляем через каждые 50 миллисекунд
}

void BuzzerService::playInitTune()
{
    // Проигрываем три гармоничные ноты
    int melody[] = {NOTE_E4, NOTE_G4, NOTE_B4};
    int noteDuration = 250; // Продолжительность каждой ноты (250 мс = четверть секунды)

    for (int thisNote = 0; thisNote < 3; ++thisNote)
    {
        // Проигрываем текущую ноту
        tone(BUZZER_PIN, melody[thisNote], noteDuration);

        // Ожидаем полную длительность ноты плюс паузу в 10%
        delay(noteDuration * 1.10);

        // Останавливаем звучание перед следующей нотой
        noTone(BUZZER_PIN);
    }
    digitalWrite(MOTOR_PIN, LOW);
}

void BuzzerService::playTone(int frequency, int duration)
{
    tone(BUZZER_PIN, frequency, duration);
}

void BuzzerService::controlMotor(bool state)
{
    unsigned long currentTime = millis();
    
    if (state)
    {
        if (!motorState && (currentTime - lastMotorActionTime) >= MOTOR_OFF_INTERVAL)
        {
            digitalWrite(MOTOR_PIN, HIGH);
            lastMotorActionTime = currentTime; // Обновляем время последнего включения мотора
            motorState = true;
        }
    }
    else
    {
        if (motorState && (currentTime - lastMotorActionTime) >= MOTOR_ON_INTERVAL)
        {
            digitalWrite(MOTOR_PIN, LOW);
            lastMotorActionTime = currentTime; // Обновляем время последнего выключения мотора
            motorState = false;
        }
    }
}