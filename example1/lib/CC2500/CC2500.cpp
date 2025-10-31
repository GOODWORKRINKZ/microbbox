#include "CC2500.h"
#include "CC2500_REG.h"
#include <Arduino.h>
#define CC2500_READ_SINGLE 0x80
#define CC2500_WRITE_SINGLE 0x00
#define RSSI_OFFSET 70 // offset for displayed data
#define MIN_RSSI_VALUE -150

#define CS_on digitalWrite(CC2500_CS_PIN, LOW)
#define CS_off digitalWrite(CC2500_CS_PIN, HIGH)
#define MOSI_on digitalWrite(HSPI_MOSI_PIN, HIGH)
#define MOSI_off digitalWrite(HSPI_MOSI_PIN, LOW)
#define MISO_on digitalRead(HSPI_MISO_PIN) == HIGH
#define SCK_on digitalWrite(HSPI_SCLK_PIN, HIGH)
#define SCK_off digitalWrite(HSPI_SCLK_PIN, LOW)

uint8_t _spi_write(uint8_t command)
{
    uint8_t result = 0;
    SCK_off; // Ensure Clock starts LOW
    // CS_on;   // Select the device by setting CS LOW
    for (int i = 7; i >= 0; --i)
    {
        if (command & (1 << i))
        {
            MOSI_on;
        }
        else
        {
            MOSI_off;
        }
        SCK_on;
        _NOP(); // Optionally delay for signal stabilization

        result <<= 1; // Shift result to make room for next bit to read
        if (MISO_on)
        {
            result |= 0x01; // Set the lowest bit of result
        }
        SCK_off; // Finish the Clock cycle
        _NOP();  // Optionally delay for signal stabilization
    }
    // CS_off; // Deselect the device by setting CS HIGH
    return result;
}

void _spi_write_address(uint8_t address, uint8_t data)
{
    CS_on;
    _spi_write(address);
    _NOP();
    _spi_write(data);
    CS_off;
}

uint8_t _spi_read()
{
    uint8_t result = 0;
    // CS_on; // Activate the chip by setting CS LOW
    for (int i = 7; i >= 0; --i)
    {
        // Shift the result left to make room for the next bit,
        // then read the next bit from MISO
        result <<= 1;
        SCK_on;
        // Delay is optional, remove if not needed
        _NOP();

        if (MISO_on)
        {
            result |= 0x01;
        }
        SCK_off;
        // Delay is optional, remove if not needed
        _NOP();
    }
    // CS_off; // Deactivate the chip by setting CS HIGH
    return result;
}

uint8_t _spi_read_address(uint8_t address)
{
    uint8_t result;
    CS_on;
    _spi_write(address);
    result = _spi_read();
    CS_off;
    return (result);
}

CC2500::CC2500(RssiBandRange &rssiRange, CalibMode calibMode)
    : SPIDevice(CC2500_CS_PIN), rssiRange_{rssiRange}, calibMode{calibMode}
{
}

void CC2500::init()
{
    SPIDevice::init();
    pinMode(HSPI_MISO_PIN, INPUT);
    lockBus();
    // Здесь должна быть инициализация и конфигурирование модуля CC2500
    writeReg(0x30, 0x3D);     // software reset for CC2500
    writeReg(FSCTRL1, 0x0F);  // Frequency Synthesizer Control (0x0F)
    writeReg(PKTCTRL0, 0x12); // Packet Automation Control (0x12)
    writeReg(FREQ2, 0x5C);    // Frequency control word, high byte
    writeReg(FREQ1, 0x4E);    // Frequency control word, middle byte
    writeReg(FREQ0, 0xDE);    // Frequency control word, low byte
    writeReg(MDMCFG4, 0x0D);  // Modem Configuration
    writeReg(MDMCFG3, 0x3B);  // Modem Configuration (0x3B)
    writeReg(MDMCFG2, 0x00);  // Modem Configuration 0x30 - OOK modulation, 0x00 - FSK modulation (better sensitivity)
    writeReg(MDMCFG1, 0x23);  // Modem Configuration
    writeReg(MDMCFG0, 0xFF);  // Modem Configuration (0xFF)
    writeReg(MCSM1, 0x0F);    // Always stay in RX mode
    writeReg(MCSM0, 0x04);    // Main Radio Control State Machine Configuration (0x04)
    writeReg(FOCCFG, 0x15);   // Frequency Offset Compensation configuration
    writeReg(AGCCTRL2, 0x83); // AGC Control (0x83)
    writeReg(AGCCTRL1, 0x00); // AGC Control
    writeReg(AGCCTRL0, 0x91); // AGC Control
    writeReg(FSCAL3, 0xEA);   // Frequency Synthesizer Calibration
    writeReg(FSCAL2, 0x0A);   // Frequency Synthesizer Calibration
    writeReg(FSCAL1, 0x00);   // Frequency Synthesizer Calibration
    writeReg(FSCAL0, 0x11);   // Frequency Synthesizer Calibration

    // calibration procedure
    // collect and save calibration data for each displayed channel-
    for (int i = 0; i < MAX_CHANNELS_2_4G; i++)
    {
        writeReg(CHANNR, i);                                  // set channel
        writeReg(SIDLE, 0x3D);                                // idle mode
        writeReg(SCAL, 0x3D);                                 // start manual calibration
        delayMicroseconds(810);                               // wait for calibration
        callibData[i] = readReg(FSCAL1 + CC2500_READ_SINGLE); // read calibration value and store it
    }

    writeReg(CHANNR, 0x00);  // set channel
    writeReg(SFSTXON, 0x3D); // calibrate and wait
    delayMicroseconds(800);  // settling time, refer to datasheet
    writeReg(SRX, 0x3D);     // enable rx
    unlockBus();
    if (calibMode == CALIB_MIN_RSSI)
    {
        rssiRange_.minRssi = MIN_RSSI_VALUE;
    }
    if (calibMode == CALIB_MAX_RSSI)
    {
        rssiRange_.maxRssi = rssiRange_.minRssi + 20;
    }
}

void CC2500::writeReg(uint8_t address, uint8_t value)
{
    // CS_off;
    _spi_write_address(address | CC2500_WRITE_SINGLE, value);
    // CS_on;
}

uint8_t CC2500::readReg(uint8_t address)
{
    // CS_off;
    uint8_t data = _spi_read_address(address);
    // CS_on;
    return data;
}

uint8_t CC2500::readRSSI()
{
    lockBus();
    byte rawRSSI = readReg(REG_RSSI); // Читаем регистр RSSI
    unlockBus();
    // Serial.print("RSSI: ");
    // Serial.println(rawRSSI);

    int rssi_dBm = (rawRSSI >= 128) ? ((rawRSSI - 256) / 2 - RSSI_OFFSET)
                                    : (rawRSSI / 2 - RSSI_OFFSET);
    // Serial.print("RSSI (dBm): ");
    // Serial.println(rssi_dBm);
    //   Преобразует rssi_dBm в проценты или другую шкалу, если это требуется
    //   Здесь для примера показывается перевод в шкалу 0-100
    if (calibMode == CALIB_MIN_RSSI)
    {
        if (rssiRange_.minRssi == MIN_RSSI_VALUE)
        {
            rssiRange_.minRssi = rssi_dBm;
        }
        if (rssiRange_.minRssi > rssi_dBm)
        {
            rssiRange_.minRssi = (rssiRange_.minRssi + rssi_dBm) / 2;
        }
    }
    if (calibMode == CALIB_MAX_RSSI)
    {
        if (rssiRange_.maxRssi < rssi_dBm)
        {
            rssiRange_.maxRssi = rssi_dBm;
        }
    }

    int percentRSSI = map(rssi_dBm, rssiRange_.minRssi, rssiRange_.maxRssi, 0, 100);
    percentRSSI = constrain(percentRSSI, 0, 100); // Обезопасим, чтобы оно не вышло за границы

    return static_cast<uint8_t>(percentRSSI);
}

void CC2500::setChannel(uint8_t channel)
{
    lockBus();
    writeReg(CHANNR, channel);             // set channel
    writeReg(FSCAL1, callibData[channel]); // restore calibration value for this channel
    unlockBus();
}

void CC2500::sendBits(uint32_t bits, uint8_t count)
{
    for (uint8_t i = 0; i < count; ++i)
    {
        sendBit((bits >> (count - 1 - i)) & 0x01);
    }
}

void CC2500::sendBit(uint8_t value)
{
    digitalWrite(HSPI_SCLK_PIN, LOW);
    delayMicroseconds(1);

    digitalWrite(HSPI_MOSI_PIN, value);
    delayMicroseconds(1);

    digitalWrite(HSPI_SCLK_PIN, HIGH);
    delayMicroseconds(1);

    digitalWrite(HSPI_SCLK_PIN, LOW);
    delayMicroseconds(1);
}