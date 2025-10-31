#include <Arduino.h>
#include "Services/Radio58Service.h"
#include <utils.h>

Radio58Service::Radio58Service(RssiBandRange &rssiRange, CalibMode calibMode) : receiver{rssiRange, calibMode}
{
    Serial.println("Radio58Service created");
}

void Radio58Service::init()
{
    receiver.init();
}

void Radio58Service::update(RadioContext &context)
{
    receiver.readRSSI();
    context.range_5_8.rssi[currentChannel] = receiver.readRSSI();
    context.range_5_8.currentChannel = currentChannel;
    currentChannel++;
    receiver.setChannel(currentChannel);
    if (currentChannel >= MAX_CHANNELS_5_8G)
    {
        currentChannel = 0;
        context.range_5_8.timestamp = millis();
    }
}