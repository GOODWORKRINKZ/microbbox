#ifndef SM1370R_H
#define SM1370R_H

#include <Arduino.h>
#include <RSSICalibrationData.h>

class SM1370R {
public:
    SM1370R(RssiBandRange &rssiRange, CalibMode calibMode);
    void init();
    void setChannel(uint8_t channel);
    uint16_t readRSSI();
    
private:
    RssiBandRange &rssiRange_; // Поле для хранения диапазона RSSI
    CalibMode calibMode;
private:
    void switchPin(uint8_t pin, bool flag);
    uint8_t currentChannel = 0;
};

#endif // SM1370R_H