#ifndef RX5808_H
#define RX5808_H

#include "SPIDevice.h"
#include "hardware_config.h"
#include <RSSICalibrationData.h>

class Rx5808 : public SPIDevice {
public:
    Rx5808(RssiBandRange &rssiRange, CalibMode calibMode);
    uint8_t readRSSI();
    void init();
    void setChannel(uint16_t channel);
    
private:
    RssiBandRange &rssiRange_; // Поле для хранения диапазона RSSI
    CalibMode calibMode;
protected:
    void sendRegister(uint8_t address, uint32_t data);
    void sendBits(uint32_t bits, uint8_t count) override;
    void sendBit(uint8_t value) override;
};

#endif // RX5808_H