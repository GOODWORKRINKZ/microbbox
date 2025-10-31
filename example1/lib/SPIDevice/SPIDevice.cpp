#include "SPIDevice.h"
#include <freertos/semphr.h>
#include <hardware_config.h>

SemaphoreHandle_t SPIDevice::spi_bus_mutex = nullptr;

void SPIDevice::initBus()
{
    pinMode(HSPI_MOSI_PIN, OUTPUT);
    pinMode(HSPI_MISO_PIN, INPUT);
    pinMode(HSPI_SCLK_PIN, OUTPUT);    
    pinMode(RX5808_CS_PIN, OUTPUT);
    pinMode(CC2500_CS_PIN, OUTPUT);
    digitalWrite(RX5808_CS_PIN,HIGH);
    digitalWrite(CC2500_CS_PIN,HIGH);
    spi_bus_mutex = xSemaphoreCreateMutex();
}

void SPIDevice::lockBus()
{
    if (spi_bus_mutex != nullptr)
    {
        xSemaphoreTake(spi_bus_mutex, portMAX_DELAY);
    }
}

void SPIDevice::unlockBus()
{
    if (spi_bus_mutex != nullptr)
    {
        xSemaphoreGive(spi_bus_mutex);
    }
}