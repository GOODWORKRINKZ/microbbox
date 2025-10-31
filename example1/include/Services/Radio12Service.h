#ifndef RADIO12_SERVICE_H
#define RADIO12_SERVICE_H

#include <vector>
#include "Context/RadioContext.h"
#include "BaseService.h"
#include <SM1370R.h>

// Заглушки функций радиоустройства (предполагаемые функции)
void initRadioHardware();
void setRadioFrequency(uint32_t frequency);
uint8_t readRadioRSSI(uint8_t channel);

class Radio12Service : public BaseService<RadioContext> {
public:
    Radio12Service(RssiBandRange &rssiRange, CalibMode calibMode);
    void init() override;
    void update(RadioContext &context) override;
    unsigned long getUpdateInterval() override {
        return 100; // Интервал обновления можно настроить в зависимости от требуемой частоты измерений
    }

private:
    SM1370R receiver;
    uint16_t currentChannel;
};

#endif // RADIO12_SERVICE_H