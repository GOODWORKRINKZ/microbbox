#include <Arduino.h>
#include "Services/Radio24Service.h"
#include <utils.h>

Radio24Service::Radio24Service(RssiBandRange &rssiRange, CalibMode calibMode) : receiver{rssiRange, calibMode}
{
    Serial.println("Radio24Service created");
}

void Radio24Service::init()
{
    receiver.init();
}

void Radio24Service::update(RadioContext &context)
{
    //receiver.readRSSI();
    context.range_2_4.rssi[currentChannel] = receiver.readRSSI();
    context.range_2_4.currentChannel = currentChannel;
    currentChannel++;
    receiver.setChannel(currentChannel);
    //Serial.println(currentChannel);
    if (currentChannel >= MAX_CHANNELS_2_4G)
    {
        currentChannel = 0;
        context.range_2_4.timestamp = millis();
    }
}