/*
 * GraphLCD driver library
 *
 * ili9341.c  -  ILI9341 TFT driver class
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2015 Andreas Regel <andreas.regel AT powarman.de>
 */

#include <stdint.h>
#include <syslog.h>
#include <unistd.h>
#include <cstring>

#include <wiringPi.h>
#include <wiringPiSPI.h>

#include "common.h"
#include "config.h"
#include "ili9341.h"


namespace GLCD
{

const int kLcdWidth  = 320;
const int kLcdHeight = 240;

const int kSpiBus    = 0;

const int kGpioReset = 23;
const int kGpioDC    = 24;

const uint8_t kCmdNop                       = 0x00;
const uint8_t kCmdSleepOut                  = 0x11;
const uint8_t kCmdGammaSet                  = 0x26;
const uint8_t kCmdDisplayOn                 = 0x29;
const uint8_t kCmdColumnAddressSet          = 0x2A;
const uint8_t kCmdPageAddressSet            = 0x2B;
const uint8_t kCmdMemoryWrite               = 0x2C;
const uint8_t kCmdMemoryAccessControl       = 0x36;
const uint8_t kCmdPixelFormatSet            = 0x3A;
const uint8_t kCmdFrameRateControl          = 0xB1;
const uint8_t kCmdDisplayFunctionControl    = 0xB6;

const uint8_t kCmdPowerControl1             = 0xC0;
const uint8_t kCmdPowerControl2             = 0xC1;
const uint8_t kCmdVcomControl1              = 0xC5;
const uint8_t kCmdVcomControl2              = 0xC7;
const uint8_t kCmdPositiveGammaControl      = 0xE0;
const uint8_t kCmdNegativeGammaControl      = 0xE1;

const uint8_t kCmdPowerControlA             = 0xCB;
const uint8_t kCmdPowerControlB             = 0xCF;
const uint8_t kCmdDriverTimingControlA      = 0xE8;
const uint8_t kCmdDriverTimingControlB      = 0xEA;
const uint8_t kCmdPowerOnSequenceControl    = 0xED;
const uint8_t kCmdUnknownEF                 = 0xEF;
const uint8_t kCmdEnable3G                  = 0xF2;
const uint8_t kCmdPumpRatioControl          = 0xF7;


cDriverILI9341::cDriverILI9341(cDriverConfig * config)
:   cDriver(config)
{
    refreshCounter = 0;

    wiringPiSetupGpio();
}

cDriverILI9341::~cDriverILI9341()
{
}

int cDriverILI9341::Init()
{
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
    newLCD = new uint32_t[width * height];
    if (newLCD)
    {
        memset(newLCD, 0, width * height * sizeof(uint32_t));
    }
    // setup lcd array (current state)
    oldLCD = new uint32_t[width * height];
    if (oldLCD)
    {
        memset(oldLCD, 0, width * height * sizeof(uint32_t));
    }

    if (config->device == "")
    {
        return -1;
    }

    pinMode(kGpioReset, OUTPUT);
    pinMode(kGpioDC, OUTPUT);

    digitalWrite(kGpioReset, HIGH);
    digitalWrite(kGpioDC, LOW);

    wiringPiSPISetup(kSpiBus, 64000000);

    /* reset display */
    Reset();

    WriteCommand(kCmdUnknownEF);
    WriteData(0x03);
    WriteData(0x80);
    WriteData(0x02);
    WriteCommand(kCmdPowerControlB);
    WriteData(0x00);
    WriteData(0xC1);
    WriteData(0x30);
    WriteCommand(kCmdPowerOnSequenceControl);
    WriteData(0x64);
    WriteData(0x03);
    WriteData(0x12);
    WriteData(0x81);
    WriteCommand(kCmdDriverTimingControlA);
    WriteData(0x85);
    WriteData(0x00);
    WriteData(0x78);
    WriteCommand(kCmdPowerControlA);
    WriteData(0x39);
    WriteData(0x2C);
    WriteData(0x00);
    WriteData(0x34);
    WriteData(0x02);
    WriteCommand(kCmdPumpRatioControl);
    WriteData(0x20);
    WriteCommand(kCmdDriverTimingControlB);
    WriteData(0x00);
    WriteData(0x00);

    WriteCommand(kCmdPowerControl1);
    WriteData(0x23);
    WriteCommand(kCmdPowerControl2);
    WriteData(0x10);
    WriteCommand(kCmdVcomControl1);
    WriteData(0x3e);
    WriteData(0x28);
    WriteCommand(kCmdVcomControl2);
    WriteData(0x86);
    WriteCommand(kCmdMemoryAccessControl);
    WriteData(0x48);
    WriteCommand(kCmdPixelFormatSet);
    WriteData(0x55);
    WriteCommand(kCmdFrameRateControl);
    WriteData(0x00);
    WriteData(0x18);
    WriteCommand(kCmdDisplayFunctionControl);
    WriteData(0x08);
    WriteData(0x82);
    WriteData(0x27);
    WriteCommand(kCmdEnable3G);
    WriteData(0x00);
    WriteCommand(kCmdGammaSet);
    WriteData(0x01);
    WriteCommand(kCmdPositiveGammaControl);
    WriteData(0x0F);
    WriteData(0x31);
    WriteData(0x2B);
    WriteData(0x0C);
    WriteData(0x0E);
    WriteData(0x08);
    WriteData(0x4E);
    WriteData(0xF1);
    WriteData(0x37);
    WriteData(0x07);
    WriteData(0x10);
    WriteData(0x03);
    WriteData(0x0E);
    WriteData(0x09);
    WriteData(0x00);
    WriteCommand(kCmdNegativeGammaControl);
    WriteData(0x00);
    WriteData(0x0E);
    WriteData(0x14);
    WriteData(0x03);
    WriteData(0x11);
    WriteData(0x07);
    WriteData(0x31);
    WriteData(0xC1);
    WriteData(0x48);
    WriteData(0x08);
    WriteData(0x0F);
    WriteData(0x0C);
    WriteData(0x31);
    WriteData(0x36);
    WriteData(0x0F);
    WriteCommand(kCmdSleepOut);
    usleep(120000);
    WriteCommand(kCmdDisplayOn);

    *oldConfig = *config;

    // clear display
    Clear();

    syslog(LOG_INFO, "%s: ILI9341 initialized.\n", config->name.c_str());
    return 0;
}

int cDriverILI9341::DeInit()
{
    // free lcd array (wanted state)
    if (newLCD)
    {
        delete[] newLCD;
    }
    // free lcd array (current state)
    if (oldLCD)
    {
        delete[] oldLCD;
    }

    return 0;
}

int cDriverILI9341::CheckSetup()
{
    if (config->device != oldConfig->device ||
        config->width != oldConfig->width ||
        config->height != oldConfig->height)
    {
        DeInit();
        Init();
        return 0;
    }

    if (config->upsideDown != oldConfig->upsideDown)
    {
        oldConfig->upsideDown = config->upsideDown;
        return 1;
    }
    return 0;
}

void cDriverILI9341::Clear()
{
    memset(newLCD, 0, width * height * sizeof(uint32_t));
}


void cDriverILI9341::SetPixel(int x, int y, uint32_t data)
{
    if (x >= width || y >= height)
        return;

    if (config->upsideDown)
    {
        x = width - 1 - x;
        y = height - 1 - y;
    }

    newLCD[y * width + x] = data;
}


void cDriverILI9341::Refresh(bool refreshAll)
{
    int y;

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
        SetWindow(0, 0, height - 1, width - 1);
        WriteCommand(kCmdMemoryWrite);
        for (y = 0; y < width; y++)
        {
            uint8_t line[height * 2];
            uint32_t * pixel = &newLCD[width - 1 - y];

            for (int x = 0; x < (height * 2); x += 2)
            {
                line[x] = ((*pixel & 0x00F80000) >> 16)
                        | ((*pixel & 0x0000E000) >> 13);
                line[x + 1] = ((*pixel & 0x00001C00) >> 5)
                            | ((*pixel & 0x000000F8) >> 3);
                pixel += width;
            }
            WriteData((uint8_t *) line, height * 2);
        }
        memcpy(oldLCD, newLCD, width * height * sizeof(uint32_t));
        // and reset RefreshCounter
        refreshCounter = 0;
    }
    else
    {
        // draw only the changed bytes
    }
}

void cDriverILI9341::SetBrightness(unsigned int percent)
{
    //WriteCommand(kCmdSetContrast, percent * 255 / 100);
}

void cDriverILI9341::Reset()
{
    digitalWrite(kGpioReset, HIGH);
    usleep(100000);
    digitalWrite(kGpioReset, LOW);
    usleep(100000);
    digitalWrite(kGpioReset, HIGH);
    usleep(100000);
}

void cDriverILI9341::SetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    WriteCommand(kCmdColumnAddressSet);
    WriteData(x0 >> 8);
    WriteData(x0);
    WriteData(x1 >> 8);
    WriteData(x1);
    WriteCommand(kCmdPageAddressSet);
    WriteData(y0 >> 8);
    WriteData(y0);
    WriteData(y1 >> 8);
    WriteData(y1);
}

void cDriverILI9341::WriteCommand(uint8_t command)
{
    digitalWrite(kGpioDC, LOW);
    wiringPiSPIDataRW(kSpiBus, &command, 1);
    digitalWrite(kGpioDC, HIGH);
}

void cDriverILI9341::WriteData(uint8_t data)
{
    wiringPiSPIDataRW(kSpiBus, &data, 1);
}

void cDriverILI9341::WriteData(uint8_t * buffer, uint32_t length)
{
    wiringPiSPIDataRW(kSpiBus, buffer, length);
}

} // end of namespace
