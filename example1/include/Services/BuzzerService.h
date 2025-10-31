#ifndef BUZZER_SERVICE_H
#define BUZZER_SERVICE_H

#include <Arduino.h>
#include "BaseService.h"
#include "Context\DisplayContext.h"
#include <hardware_config.h>

class BuzzerService : public BaseService<DisplayContext>
{
public:
    BuzzerService();
    void init() override;
    void update(DisplayContext &context) override;
    unsigned long getUpdateInterval() override;

private:
    void controlMotor(bool state);
    void playInitTune(); // Метод для воспроизведения стартовой мелодии
    void playTone(int frequency, int duration);
    uint16_t duration = 500;
    ulong lastUpdate = 0;
    ulong lastMotorActionTime = 0;
    bool motorState;
};

#endif // BUZZER_SERVICE_H