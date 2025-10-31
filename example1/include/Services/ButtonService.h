#ifndef BUTTON_SERVICE_H
#define BUTTON_SERVICE_H

#include "BaseService.h"
#include <Arduino.h>
#include <utils.h>

// Класс ButtonService, наследующий BaseService
class ButtonService : public BaseService<Settings>
{
public:
    ButtonService();
    ~ButtonService() override;

    void init() override;
    void update(Settings &context) override;
    unsigned long getUpdateInterval() override;

private:
    // Возможные состояния сервиса
    enum State
    {
        IDLE,
        DEBOUNCE,
        PRESS_FIXED,
        SHORT_PRESS_UP,
        SHORT_PRESS_DOWN,
        LONG_PRESS_UP,
        LONG_PRESS_DOWN,
        SHORT_PRESS_BOTH,
        LONG_PRESS_BOTH,
        RELEASE_DEBOUNCE
    };
    bool upPressed = false;
    bool downPressed = false;
    bool upFixedPressed = false;
    bool downFixedPressed = false;
    volatile State state = IDLE;
    unsigned long lastRepeatTime = 0;
    unsigned long lastDebounceTime = 0;
    unsigned long releaseDebounceTime = 0;

    static void ISR_UP();   // Обработчик прерывания для кнопки вверх
    static void ISR_DOWN(); // Обработчик прерывания для кнопки вниз

    void handleISR_UP();   // Логика обработки прерывания кнопки вверх
    void handleISR_DOWN(); // Логика обработки прерывания кнопки вниз

    void buttonUpPressed();    // Обработка нажатия кнопки вверх
    void buttonUpReleased();   // Обработка отпускания кнопки вверх
    void buttonDownPressed();  // Обработка нажатия кнопки вниз
    void buttonDownReleased(); // Обработка отпускания кнопки вниз

    static ButtonService *instance; // Указатель на экземпляр ButtonService для использования в ISR
};

#endif // BUTTON_SERVICE_H