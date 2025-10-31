#ifndef CC2500_H
#define CC2500_H

#include "SPIDevice.h"
#include "hardware_config.h"
#include <RSSICalibrationData.h>

class CC2500 : public SPIDevice
{
public:
    CC2500(RssiBandRange &rssiRange, CalibMode calibMode);
    void init() override;
    uint8_t readRSSI();
    void setChannel(uint8_t channel);

private:
    RssiBandRange &rssiRange_; // Поле для хранения диапазона RSSI
    CalibMode calibMode;
protected:
    void writeReg(uint8_t address, uint8_t value);
    uint8_t readReg(uint8_t address);
    void sendBits(uint32_t bits, uint8_t count) override;
    void sendBit(uint8_t value) override;
    uint8_t callibData[MAX_CHANNELS_2_4G];
};

#endif // CC2500_H