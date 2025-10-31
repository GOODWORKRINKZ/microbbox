#ifndef RADIO58_SERVICE_H
#define RADIO58_SERVICE_H

#include <vector>
#include "Context/RadioContext.h"
#include "BaseService.h"
#include <Rx5808.h>

class Radio58Service : public BaseService<RadioContext>
{
public:
    Radio58Service(RssiBandRange &rssiRange, CalibMode calibMode);
    void init() override;
    void update(RadioContext &context) override;
    unsigned long getUpdateInterval() override
    {
        return 20; // Интервал обновления можно настроить в зависимости от требуемой частоты измерений
    }

private:
    Rx5808 receiver;
    uint16_t currentChannel;
};

#endif // RADIO58_SERVICE_H