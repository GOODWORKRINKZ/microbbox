#include "Services/ButtonService.h"

#define DEBOUNCE_DELAY 30
#define LONG_PRESS_DELAY 1000
#define RESET_PRESS_DELAY 10000
#define REPEAT_INTERVAL 50

void ensureSingleSelection(Settings &settings)
{
    bool anySelected = false;

#if RADIO_1_2G_ENABLED
    anySelected |= settings.band_1_2.selected;
#endif
#if RADIO_2_4G_ENABLED
    anySelected |= settings.band_2_4.selected;
#endif
#if RADIO_5_8G_ENABLED
    anySelected |= settings.band_5_8.selected;
#endif

    if (!anySelected)
    {
#if RADIO_1_2G_ENABLED
        settings.band_1_2.selected = true;
#endif
    }

    int selectedCount = 0;

#if RADIO_1_2G_ENABLED
    selectedCount += settings.band_1_2.selected;
#endif
#if RADIO_2_4G_ENABLED
    selectedCount += settings.band_2_4.selected;
#endif
#if RADIO_5_8G_ENABLED
    selectedCount += settings.band_5_8.selected;
#endif

    if (selectedCount > 1)
    {
#if RADIO_1_2G_ENABLED
        settings.band_1_2.selected = true;
#endif
#if RADIO_2_4G_ENABLED
        settings.band_2_4.selected = false;
#endif
#if RADIO_5_8G_ENABLED
        settings.band_5_8.selected = false;
#endif
    }
}

BandSettings *getSelectedBand(Settings &settings)
{
    ensureSingleSelection(settings);
#if RADIO_1_2G_ENABLED
    if (settings.band_1_2.selected)
        return &settings.band_1_2;
#endif
#if RADIO_2_4G_ENABLED
    if (settings.band_2_4.selected)
        return &settings.band_2_4;
#endif
#if RADIO_5_8G_ENABLED
    if (settings.band_5_8.selected)
        return &settings.band_5_8;
#endif
    return nullptr;
}

void selectNextBand(Settings &settings)
{
    ensureSingleSelection(settings);
#if RADIO_1_2G_ENABLED
    if (settings.band_1_2.selected)
    {
        settings.band_1_2.selected = false;
#if RADIO_2_4G_ENABLED
        settings.band_2_4.selected = true;
#elif defined(RADIO_5_8G_ENABLED)
        settings.band_5_8.selected = true;
#endif
        return;
    }
#endif
#if RADIO_2_4G_ENABLED
    if (settings.band_2_4.selected)
    {
        settings.band_2_4.selected = false;
#if RADIO_5_8G_ENABLED
        settings.band_5_8.selected = true;
#elif defined(RADIO_1_2G_ENABLED)
        settings.band_1_2.selected = true;
#endif
        return;
    }
#endif
#if RADIO_5_8G_ENABLED
    if (settings.band_5_8.selected)
    {
        settings.band_5_8.selected = false;
#if RADIO_1_2G_ENABLED
        settings.band_1_2.selected = true;
#elif defined(RADIO_2_4G_ENABLED)
        settings.band_2_4.selected = true;
#endif
    }
#endif
}

void selectPreviousBand(Settings &settings)
{
    ensureSingleSelection(settings);
#if RADIO_1_2G_ENABLED
    if (settings.band_1_2.selected)
    {
        settings.band_1_2.selected = false;
#if RADIO_5_8G_ENABLED
        settings.band_5_8.selected = true;
#elif defined(RADIO_2_4G_ENABLED)
        settings.band_2_4.selected = true;
#endif
        return;
    }
#endif
#if RADIO_2_4G_ENABLED
    if (settings.band_2_4.selected)
    {
        settings.band_2_4.selected = false;
#if RADIO_1_2G_ENABLED
        settings.band_1_2.selected = true;
#elif defined(RADIO_5_8G_ENABLED)
        settings.band_5_8.selected = true;
#endif
        return;
    }
#endif
#if RADIO_5_8G_ENABLED
    if (settings.band_5_8.selected)
    {
        settings.band_5_8.selected = false;
#if RADIO_2_4G_ENABLED
        settings.band_2_4.selected = true;
#elif defined(RADIO_1_2G_ENABLED)
        settings.band_1_2.selected = true;
#endif
    }
#endif
}

void incrementSensitivity(BandSettings &band)
{
    if (band.sensitivity < 100)
    {
        band.sensitivity += 1;
    }
}

void decrementSensitivity(BandSettings &band)
{
    if (band.sensitivity > 0)
    {
        band.sensitivity -= 1;
    }
}

void resetToFactorySettings(Settings &settings)
{
    resetSettings(settings);
}

// Инициализация статического экземпляра указателя
ButtonService *ButtonService::instance = nullptr;

// Конструктор
ButtonService::ButtonService()
{
    instance = this;
}

// Деструктор
ButtonService::~ButtonService()
{
    detachInterrupt(digitalPinToInterrupt(BUTTON_DOWN_PIN));
    detachInterrupt(digitalPinToInterrupt(BUTTON_UP_PIN));
}

// Метод инициализации
void ButtonService::init()
{
    pinMode(BUTTON_DOWN_PIN, INPUT);
    pinMode(BUTTON_UP_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(BUTTON_UP_PIN), ISR_UP, CHANGE);
    attachInterrupt(digitalPinToInterrupt(BUTTON_DOWN_PIN), ISR_DOWN, CHANGE);
}

void ButtonService::update(Settings &settings)
{
    unsigned long currentMillis = millis();
    BandSettings *selectedBand = getSelectedBand(settings);

    switch (state)
    {
    case DEBOUNCE:
        if ((currentMillis - lastDebounceTime) >= DEBOUNCE_DELAY)
        {
            if (!upPressed && !downPressed)
            {
                state = IDLE;
                break;
            }
            upFixedPressed = upPressed;
            downFixedPressed = downPressed;
            state = PRESS_FIXED;
        }
        break;

    case PRESS_FIXED:
        if (upFixedPressed && downFixedPressed)
        {
            if (!upPressed && !downPressed)
            {
                if ((currentMillis - lastDebounceTime) <= LONG_PRESS_DELAY)
                {
                    state = SHORT_PRESS_BOTH;
                }
            }
            else if ((currentMillis - lastDebounceTime) >= RESET_PRESS_DELAY)
            {
                resetToFactorySettings(settings);
                saveSettings(settings);
                state = LONG_PRESS_BOTH;
            }
            break;
        }

        if (upFixedPressed)
        {
            if (!upPressed)
            {
                state = SHORT_PRESS_UP;
            }
            else if ((currentMillis - lastDebounceTime) >= LONG_PRESS_DELAY)
            {
                state = LONG_PRESS_UP;
            }
        }

        if (downFixedPressed)
        {
            if (!downPressed)
            {
                state = SHORT_PRESS_DOWN;
            }
            else if ((currentMillis - lastDebounceTime) >= LONG_PRESS_DELAY)
            {
                state = LONG_PRESS_DOWN;
            }
        }
        break;

    case SHORT_PRESS_UP:
        selectPreviousBand(settings);
        saveSettings(settings);
        releaseDebounceTime = millis();
        state = RELEASE_DEBOUNCE; // Возврат к IDLE после обработки
        break;

    case SHORT_PRESS_DOWN:
        selectNextBand(settings);
        saveSettings(settings);
        releaseDebounceTime = millis();
        state = RELEASE_DEBOUNCE; // Возврат к IDLE после обработки
        break;

    case SHORT_PRESS_BOTH:
        if (selectedBand)
        {
            selectedBand->mute = !selectedBand->mute;
            saveSettings(settings);
        }
        releaseDebounceTime = millis();
        state = RELEASE_DEBOUNCE; // Возврат к IDLE после обработки
        break;

    case LONG_PRESS_UP:
        if ((currentMillis - lastRepeatTime) > 50)
        {
            incrementSensitivity(*selectedBand);
            saveSettings(settings);
            lastRepeatTime = currentMillis;
        }
        if (!upPressed)
        {
            releaseDebounceTime = millis();
            state = RELEASE_DEBOUNCE; // Возврат к IDLE после обработки
        }
        break;

    case LONG_PRESS_DOWN:
        if ((currentMillis - lastRepeatTime) > 50)
        {
            decrementSensitivity(*selectedBand);
            saveSettings(settings);
            lastRepeatTime = currentMillis;
        }
        if (!downPressed)
        {
            releaseDebounceTime = millis();
            state = RELEASE_DEBOUNCE; // Возврат к IDLE после обработки
        }
        break;

    case LONG_PRESS_BOTH:
        if (!upPressed && !downPressed)
        {
            releaseDebounceTime = millis();
            state = RELEASE_DEBOUNCE; // Возврат к IDLE после обработки
        }
        break;
    case RELEASE_DEBOUNCE:
        if ((millis() - releaseDebounceTime) > DEBOUNCE_DELAY)
        {
            state = IDLE;
        }
        break;
    default:
        break;
    }
}

unsigned long ButtonService::getUpdateInterval()
{
    return 10; // Интервал обновления в миллисекундах
}

void ButtonService::ISR_UP()
{
    instance->handleISR_UP();
}

void ButtonService::ISR_DOWN()
{
    instance->handleISR_DOWN();
}

void ButtonService::handleISR_UP()
{
    if (digitalRead(BUTTON_UP_PIN) == LOW)
    {
        buttonUpPressed();
    }
    else
    {
        buttonUpReleased();
    }
}

void ButtonService::handleISR_DOWN()
{
    if (digitalRead(BUTTON_DOWN_PIN) == LOW)
    {
        buttonDownPressed();
    }
    else
    {
        buttonDownReleased();
    }
}

void ButtonService::buttonUpPressed()
{
    upPressed = true;
    if (state == IDLE)
    {
        state = DEBOUNCE;
        lastDebounceTime = millis();
    }
}

void ButtonService::buttonUpReleased()
{
    upPressed = false;
}

void ButtonService::buttonDownPressed()
{
    downPressed = true;
    if (state == IDLE)
    {
        state = DEBOUNCE;
        lastDebounceTime = millis();
    }
}

void ButtonService::buttonDownReleased()
{
    downPressed = false;
}