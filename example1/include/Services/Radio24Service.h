#ifndef RADIO24_SERVICE_H
#define RADIO24_SERVICE_H

#include <vector>
#include "Context/RadioContext.h"
#include "BaseService.h"
#include <CC2500.h>

class Radio24Service : public BaseService<RadioContext>
{
public:
    Radio24Service(RssiBandRange &rssiRange, CalibMode calibMode);
    void init() override;
    void update(RadioContext &context) override;
    unsigned long getUpdateInterval() override {
        return 1; // Интервал обновления можно настроить в зависимости от требуемой частоты измерений
    }

private:
    CC2500 receiver;
    uint16_t currentChannel;
};

#endif // RADIO24_SERVICE_H