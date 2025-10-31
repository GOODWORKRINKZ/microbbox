#include <Arduino.h>
#include "Services/Radio12Service.h"
#include <utils.h>

Radio12Service::Radio12Service(RssiBandRange &rssiRange, CalibMode calibMode) : receiver{rssiRange, calibMode}
{
    Serial.println("Radio12Service created");
}

void Radio12Service::init()
{
    receiver.init();
}

void Radio12Service::update(RadioContext &context)
{
    context.range_1_2.rssi[currentChannel] = receiver.readRSSI();
    context.range_1_2.currentChannel = currentChannel;
        currentChannel++;
    receiver.setChannel(currentChannel);
    // Serial.println(currentChannel);
    if (currentChannel >= MAX_CHANNELS_1_2G)
    {
        currentChannel = 0;
        context.range_1_2.timestamp = millis();
    }
}