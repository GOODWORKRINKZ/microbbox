#include "Rx5808.h"
#include <Arduino.h>
#include <globals.h>
#define RSSI_OFFSET 20
#define MIN_RSSI_VALUE 0
#define SPI_ADDRESS_SYNTH_A 0x01
#define SPI_ADDRESS_POWER 0x0A
#define _CHANNEL_REG_FLO(f) ((f - 479) / 2)
#define _CHANNEL_REG_N(f) (_CHANNEL_REG_FLO(f) / 32)
#define _CHANNEL_REG_A(f) (_CHANNEL_REG_FLO(f) % 32)
#define CHANNEL_REG(f) (_CHANNEL_REG_N(f) << 7) | _CHANNEL_REG_A(f)

// Обновленный конструктор, принимающий диапазон частот
Rx5808::Rx5808(RssiBandRange &rssiRange, CalibMode calibMode)
    : SPIDevice(RX5808_CS_PIN), rssiRange_{rssiRange}, calibMode{calibMode}
{
}

void Rx5808::init()
{
    SPIDevice::init();
    pinMode(RX5808_CS_PIN, OUTPUT);
    lockBus();
    select();
    sendBit(0x10);
    sendBit(0x01);
    sendBit(0x00);
    sendBit(0x00);
    // Finished clocking data in
    deselect();
    digitalWrite(HSPI_SCLK_PIN, LOW);
    digitalWrite(HSPI_MOSI_PIN, LOW);
    unlockBus();
    if (calibMode == CALIB_MIN_RSSI)
    {
        rssiRange_.minRssi = MIN_RSSI_VALUE;
    }
    if (calibMode == CALIB_MAX_RSSI)
    {
        rssiRange_.maxRssi = rssiRange_.minRssi + 100;
    }
}

void Rx5808::sendBits(uint32_t bits, uint8_t count)
{
    for (uint8_t i = 0; i < count; i++)
    {
        sendBit(bits & 0x1);
        bits >>= 1;
    }
}

void Rx5808::sendBit(uint8_t value)
{
    digitalWrite(HSPI_SCLK_PIN, LOW);
    delayMicroseconds(1);

    digitalWrite(HSPI_MOSI_PIN, value);
    delayMicroseconds(1);

    digitalWrite(HSPI_SCLK_PIN, HIGH);
    delayMicroseconds(1);

    digitalWrite(HSPI_SCLK_PIN, LOW);
    delayMicroseconds(1);
}

void Rx5808::setChannel(uint16_t channel)
{
    sendRegister(SPI_ADDRESS_SYNTH_A, CHANNEL_REG(MIN_5800_FREQ + channel * (MAX_5800_FREQ - MIN_5800_FREQ) / MAX_CHANNELS_5_8G));
}

void Rx5808::sendRegister(uint8_t address, uint32_t data)
{
    lockBus();
    select();
    delayMicroseconds(5);
    sendBits(address, 4); // Предположим, что адреса у нас 4-битные
    sendBit(1);           // Enable write bit предположим, что это требуется.
    sendBits(data, 20);   // Предположим, что данные у нас 20-битные
    deselect();
    delayMicroseconds(5);
    unlockBus();
}

uint8_t Rx5808::readRSSI()
{
    uint16_t raw = analogRead(RX5808_RSSI_PIN);
    if (calibMode == CALIB_MIN_RSSI)
    {
        if (rssiRange_.minRssi == MIN_RSSI_VALUE)
        {
            rssiRange_.minRssi = raw;
        }
        if (rssiRange_.minRssi - RSSI_OFFSET > raw)
        {
            rssiRange_.minRssi = ((rssiRange_.minRssi - RSSI_OFFSET) + raw) / 2 + RSSI_OFFSET;
        }
    }
    if (calibMode == CALIB_MAX_RSSI)
    {
        if (rssiRange_.maxRssi < raw)
        {
            rssiRange_.maxRssi = raw;
        }
    }
    uint32_t val =
        map(
            constrain(raw, rssiRange_.minRssi, rssiRange_.maxRssi),
            rssiRange_.minRssi,
            rssiRange_.maxRssi,
            0,
            100);
    // Serial.println(raw);
    if (val > 100)
    {
        val = 100;
    }
    return val;
}