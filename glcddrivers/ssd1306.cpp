/*
 * GraphLCD driver library
 *
 * ssd1306.c  -  SSD1306 OLED driver class
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2014 Andreas Regel <andreas.regel AT powarman.de>
 */

#include <stdint.h>
#include <syslog.h>
#include <unistd.h>
#include <cstring>

#include <wiringPi.h>
#include <wiringPiSPI.h>

#include "common.h"
#include "config.h"
#include "ssd1306.h"


namespace GLCD
{

const int kLcdWidth  = 128;
const int kLcdHeight = 64;

const int kSpiBus    = 0;

const int kGpioReset = 15;
const int kGpioDC    = 16;

const uint8_t kCmdSetLowerColumn            = 0x00;
const uint8_t kCmdSetHigherColumn           = 0x10;
const uint8_t kCmdSetMemoryAddressingMode   = 0x20;
const uint8_t kCmdSetColumnAddress          = 0x21;
const uint8_t kCmdSetPageAddress            = 0x22;
const uint8_t kCmdSetDisplayStartLine       = 0x40;
const uint8_t kCmdSetContrast               = 0x81;
const uint8_t kCmdSetChargePump             = 0x8D;
const uint8_t kCmdSetSegmentReMap           = 0xA0;
const uint8_t kCmdEntireDisplayOnResume     = 0xA4;
const uint8_t kCmdEntireDisplayOn           = 0xA5;
const uint8_t kCmdSetNormalDisplay          = 0xA6;
const uint8_t kCmdSetInverseDisplay         = 0xA7;
const uint8_t kCmdSetMultiplexRatio         = 0xA8;
const uint8_t kCmdSetDisplayOff             = 0xAE;
const uint8_t kCmdSetDisplayOn              = 0xAF;
const uint8_t kCmdSetPageStart              = 0xB0;
const uint8_t kCmdSetComScanInc             = 0xC0;
const uint8_t kCmdSetComScanDec             = 0xC8;
const uint8_t kCmdSetDisplayOffset          = 0xD3;
const uint8_t kCmdSetDisplayClockDiv        = 0xD5;
const uint8_t kCmdSetPreChargePeriod        = 0xD9;
const uint8_t kCmdSetComPins                = 0xDA;
const uint8_t kCmdSetVComDeselectLevel      = 0xDB;


cDriverSSD1306::cDriverSSD1306(cDriverConfig * config)
:   cDriver(config)
{
    refreshCounter = 0;

    wiringPiSetup();
}

cDriverSSD1306::~cDriverSSD1306()
{
    //delete port;
}

int cDriverSSD1306::Init()
{
    int x;

    width = config->width;
    if (width <= 0)
        width = kLcdWidth;
    height = config->height;
    if (height <= 0)
        height = kLcdHeight;

    for (unsigned int i = 0; i < config->options.size(); i++)
    {
        if (config->options[i].name == "")
        {
        }
    }

    // setup lcd array (wanted state)
    newLCD = new unsigned char*[width];
    if (newLCD)
    {
        for (x = 0; x < width; x++)
        {
            newLCD[x] = new unsigned char[(height + 7) / 8];
            memset(newLCD[x], 0, (height + 7) / 8);
        }
    }
    // setup lcd array (current state)
    oldLCD = new unsigned char*[width];
    if (oldLCD)
    {
        for (x = 0; x < width; x++)
        {
            oldLCD[x] = new unsigned char[(height + 7) / 8];
            memset(oldLCD[x], 0, (height + 7) / 8);
        }
    }

    if (config->device == "")
    {
        return -1;
    }

    pinMode(kGpioReset, OUTPUT);
    pinMode(kGpioDC, OUTPUT);

    digitalWrite(kGpioReset, HIGH);
    digitalWrite(kGpioDC, LOW);

    wiringPiSPISetup(kSpiBus, 1000000);

    /* reset display */
    Reset();
    usleep(1000);

    WriteCommand(kCmdSetDisplayOff);

    if (height == 64)
    {
        WriteCommand(kCmdSetMultiplexRatio, 0x3F);
        WriteCommand(kCmdSetComPins, 0x12);
    }
    else if (height == 32)
    {
        WriteCommand(kCmdSetMultiplexRatio, 0x1F);
        WriteCommand(kCmdSetComPins, 0x02);
    }

    WriteCommand(kCmdSetDisplayOffset, 0x00);
    WriteCommand(kCmdSetDisplayStartLine | 0x00);
    WriteCommand(kCmdSetMemoryAddressingMode, 0x01);
    WriteCommand(kCmdSetSegmentReMap | 0x01);
    WriteCommand(kCmdSetComScanDec);
    WriteCommand(kCmdSetContrast, config->brightness * 255 / 100);
    WriteCommand(kCmdEntireDisplayOnResume);
    WriteCommand(kCmdSetNormalDisplay);
    WriteCommand(kCmdSetDisplayClockDiv, 0x80);
    WriteCommand(kCmdSetChargePump, 0x14);
    WriteCommand(kCmdSetDisplayOn);

    *oldConfig = *config;

    // clear display
    Clear();

    syslog(LOG_INFO, "%s: SSD1306 initialized.\n", config->name.c_str());
    return 0;
}

int cDriverSSD1306::DeInit()
{
    int x;
    // free lcd array (wanted state)
    if (newLCD)
    {
        for (x = 0; x < width; x++)
        {
            delete[] newLCD[x];
        }
        delete[] newLCD;
    }
    // free lcd array (current state)
    if (oldLCD)
    {
        for (x = 0; x < width; x++)
        {
            delete[] oldLCD[x];
        }
        delete[] oldLCD;
    }

    return 0;
}

int cDriverSSD1306::CheckSetup()
{
    if (config->device != oldConfig->device ||
        config->width != oldConfig->width ||
        config->height != oldConfig->height)
    {
        DeInit();
        Init();
        return 0;
    }

    if (config->upsideDown != oldConfig->upsideDown ||
        config->invert != oldConfig->invert)
    {
        oldConfig->upsideDown = config->upsideDown;
        oldConfig->invert = config->invert;
        return 1;
    }
    return 0;
}

void cDriverSSD1306::Clear()
{
    for (int x = 0; x < width; x++)
        memset(newLCD[x], 0, (height + 7) / 8);
}


void cDriverSSD1306::SetPixel(int x, int y, uint32_t data)
{
    if (x >= width || y >= height)
        return;

    if (config->upsideDown)
    {
        x = width - 1 - x;
        y = height - 1 - y;
    }

    int offset = (y % 8);
    if (data == GRAPHLCD_White)
        newLCD[x][y / 8] |= (1 << offset);
    else
        newLCD[x][y / 8] &= ( 0xFF ^ (1 << offset) );
}


#if 0
void cDriverSSD1306::Set8Pixels(int x, int y, unsigned char data)
{
    if (x >= width || y >= height)
        return;

    if (!config->upsideDown)
    {
        int offset = 7 - (y % 8);
        for (int i = 0; i < 8; i++)
        {
            newLCD[x + i][y / 8] |= ((data >> (7 - i)) << offset) & (1 << offset);
        }
    }
    else
    {
        x = width - 1 - x;
        y = height - 1 - y;
        int offset = 7 - (y % 8);
        for (int i = 0; i < 8; i++)
        {
            newLCD[x - i][y / 8] |= ((data >> (7 - i)) << offset) & (1 << offset);
        }
    }
}
#endif

void cDriverSSD1306::Refresh(bool refreshAll)
{
    int x;
    int y;
    uint8_t numPages = (height + 7) / 8;
    unsigned char data[16];

    if (CheckSetup() == 1)
        refreshAll = true;

    if (config->refreshDisplay > 0)
    {
        refreshCounter = (refreshCounter + 1) % config->refreshDisplay;
        if (!refreshAll && !refreshCounter)
            refreshAll = true;
    }

    refreshAll = true;
    if (refreshAll)
    {
        WriteCommand(kCmdSetColumnAddress, 0, width - 1);
        WriteCommand(kCmdSetPageAddress, 0, numPages - 1);
        for (x = 0; x < width; x++)
        {
            for (y = 0; y < numPages; y++)
            {
                data[y] = (newLCD[x][y]) ^ (config->invert ? 0xff : 0x00);
            }
            WriteData(data, numPages);
            memcpy(oldLCD[x], newLCD[x], numPages);
        }
        // and reset RefreshCounter
        refreshCounter = 0;
    }
    else
    {
        // draw only the changed bytes
    }
}

void cDriverSSD1306::SetBrightness(unsigned int percent)
{
    WriteCommand(kCmdSetContrast, percent * 255 / 100);
}

void cDriverSSD1306::Reset()
{
    digitalWrite(kGpioReset, LOW);
    usleep(1000);
    digitalWrite(kGpioReset, HIGH);
}

void cDriverSSD1306::WriteCommand(uint8_t command)
{
    wiringPiSPIDataRW(kSpiBus, &command, 1);
}

void cDriverSSD1306::WriteCommand(uint8_t command, uint8_t argument)
{
    uint8_t buffer[2] = {command, argument};
    wiringPiSPIDataRW(kSpiBus, buffer, 2);
}

void cDriverSSD1306::WriteCommand(uint8_t command, uint8_t argument1, uint8_t argument2)
{
    uint8_t buffer[3] = {command, argument1, argument2};
    wiringPiSPIDataRW(kSpiBus, buffer, 3);
}

void cDriverSSD1306::WriteData(uint8_t * buffer, uint32_t length)
{
    digitalWrite(kGpioDC, HIGH);
    wiringPiSPIDataRW(kSpiBus, buffer, length);
    digitalWrite(kGpioDC, LOW);
}

} // end of namespace
