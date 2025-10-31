#ifndef SPI_DEVICE_H
#define SPI_DEVICE_H

#include <Arduino.h>

class SPIDevice
{
protected:
    uint8_t CS_pin;
    virtual void sendBit(uint8_t value) = 0;
    virtual void sendBits(uint32_t bits, uint8_t count) = 0;
    void select() { digitalWrite(CS_pin, LOW); }
    void deselect() { digitalWrite(CS_pin, HIGH); }
    static void lockBus();
    static void unlockBus();

public:
    SPIDevice(uint8_t cs_pin) : CS_pin(cs_pin) {}
    virtual ~SPIDevice() {}
    virtual void init()
    {
        pinMode(CS_pin, OUTPUT);
    }
    static void initBus();

private:
    static SemaphoreHandle_t spi_bus_mutex;
};

#endif // SPI_DEVICE_H