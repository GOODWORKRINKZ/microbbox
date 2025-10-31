#include "SM1370R.h"
#include "hardware_config.h"

const uint8_t channels_pin_sequence[] = {
    0b0001, // A1
    0b0011, // A2
    0b0101, // A3
    0b0111, // A4
    0b1001, // A5
    0b1000, // B5
    0b1011, // A6
    0b1101, // A7
    0b1111, // A8
};

#define RSSI_OFFSET 200
#define MIN_RSSI_VALUE 0
#define SAVE_SELECTED_CHANNEL_ADDRESS 0
#define GET_RSSI_COUNT 100

SM1370R::SM1370R(RssiBandRange &rssiRange, CalibMode calibMode) : rssiRange_{rssiRange}, calibMode{calibMode}
{
    // Конструктор
}

void SM1370R::init()
{
    // Инициализация аппаратных средств
    pinMode(SM1370R_S1_PIN, INPUT);
    pinMode(SM1370R_CS1_PIN, INPUT);
    pinMode(SM1370R_CS2_PIN, INPUT);
    pinMode(SM1370R_CS3_PIN, INPUT);
    if (calibMode == CALIB_MIN_RSSI)
    {
        rssiRange_.minRssi = MIN_RSSI_VALUE;
    }
    if (calibMode == CALIB_MAX_RSSI)
    {
        rssiRange_.maxRssi = rssiRange_.minRssi + 100;
    }
}

void SM1370R::switchPin(uint8_t pin, bool flag)
{
    pinMode(pin, flag ? OUTPUT : INPUT);
}

uint16_t SM1370R::readRSSI()
{
    uint32_t rssi = 0; // Uint32_t для накопления более крупных значений из нескольких измерений

    for (uint8_t i = 0; i < GET_RSSI_COUNT; i++)
    {
        rssi += analogRead(SM1370R_RSSI_PIN);
    }

    rssi = rssi / GET_RSSI_COUNT; // Усреднённое значение
    if (calibMode == CALIB_MIN_RSSI)
    {
        if (rssiRange_.minRssi == MIN_RSSI_VALUE)
        {
            rssiRange_.minRssi = rssi + RSSI_OFFSET;
        }
        if (rssiRange_.minRssi - RSSI_OFFSET > rssi)
        {
            rssiRange_.minRssi = ((rssiRange_.minRssi - RSSI_OFFSET) + rssi) / 2 + RSSI_OFFSET;
        }
    }
    if (calibMode == CALIB_MAX_RSSI)
    {
        if (rssiRange_.maxRssi < rssi)
        {
            rssiRange_.maxRssi = rssi;
        }
    }
    // Serial.println(rssi);
    uint32_t val =
        map(
            constrain(rssi, rssiRange_.minRssi, rssiRange_.maxRssi),
            rssiRange_.minRssi,
            rssiRange_.maxRssi,
            0,
            100);
    if (val > 100)
    {
        val = 100;
    }
    return val;
}

void SM1370R::setChannel(uint8_t channel)
{
    switchPin(SM1370R_S1_PIN, (channels_pin_sequence[channel] >> 0) & 0x01);
    switchPin(SM1370R_CS1_PIN, (channels_pin_sequence[channel] >> 1) & 0x01);
    switchPin(SM1370R_CS2_PIN, (channels_pin_sequence[channel] >> 2) & 0x01);
    switchPin(SM1370R_CS3_PIN, (channels_pin_sequence[channel] >> 3) & 0x01);

    currentChannel = channel;
}