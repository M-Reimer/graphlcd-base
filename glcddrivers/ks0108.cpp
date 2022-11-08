/*
 * GraphLCD driver library
 *
 * ks0108.c  -  KS0108 driver class
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2003-2011 Andreas 'randy' Weinberger <vdr AT smue.org>
 * (c) 2011      Wolfgang Astleitner <mrwastl AT users.sourceforge.net>
 */

#include <syslog.h>
#include <sys/time.h>
#include <cstring>

#include "common.h"
#include "config.h"
#include "ks0108.h"
#include "port.h"


namespace GLCD
{

// commands
const unsigned char kSEAD = 0x40; // Set (X) Address
const unsigned char kSEPA = 0xb8; // Set (Y) Page
const unsigned char kSEDS = 0xc0; // Set Display Start Line
const unsigned char kDIOF = 0x3e; // Display off
const unsigned char kDION = 0x3f; // Display on

const unsigned char kCEHI = 0x01; // Chip Enable on
const unsigned char kCELO = 0x00;
const unsigned char kCDHI = 0x08; // Command/Data Register Select
const unsigned char kCDLO = 0x00;

const unsigned char kCS1HI = 0x02; // ChipSelect 1
const unsigned char kCS1LO = 0x00;
const unsigned char kCS2HI = 0x00; // ChipSelect 2
const unsigned char kCS2LO = 0x04;


cDriverKS0108::cDriverKS0108(cDriverConfig * config)
:   cDriver(config)
{
    port = new cParallelPort();

    refreshCounter = 0;
    timeForLCDInNs = 50;
    control = 1;
}

cDriverKS0108::~cDriverKS0108()
{
    delete port;
}

int cDriverKS0108::Init()
{
    int x;
    int i;
    struct timeval tv1, tv2;

    if (config->width <= 128) {
        width = 128;
    } else if (config->width > 192) {
        width = 256;
    } else if (config->width > 128) {
        width = 192;
    }

    if (config->height <= 64) {
        height = 64;
    } else if (config->height > 64) {
        height = 128;
        width = 128; // force 2* 128x64 display
    }

    if (width == 128 && height == 64) {
        CS1 = kCS2HI | kCS1LO;
        CS2 = kCS2LO | kCS1HI;
        CS3 = -1; // invalid
        CS4 = -1;
    } else { // multiplexed via 74LS42
        CS1 = kCS2HI | kCS1HI;
        CS2 = kCS2HI | kCS1LO;
        CS3 = kCS2LO | kCS1HI;
        CS4 = kCS2LO | kCS1LO;
    }

    SEAD = kSEAD;
    SEPA = kSEPA;
    SEDS = kSEDS;
    DIOF = kDIOF;
    DION = kDION;

    CEHI = kCEHI;
    CELO = kCELO;
    CDHI = kCDHI;
    CDLO = kCDLO;

    for (unsigned int i = 0; i < config->options.size(); i++)
    {
        if (config->options[i].name == "Control")
        {
            if (config->options[i].value == "0")
                control = 0;
            else if (config->options[i].value == "1")
                control = 1;
            else
                syslog(LOG_ERR, "%s error: unknown control setting %s, using default (%d)!\n",
                       config->name.c_str(), config->options[i].value.c_str(), control);
        }
    }

    // setup linear lcd array
    LCD = new unsigned char *[(width + 7) / 8];
    if (LCD)
    {
        for (x = 0; x < (width + 7) / 8; x++)
        {
            LCD[x] = new unsigned char[height];
            memset(LCD[x], 0, height);
        }
    }
    // setup the lcd array for the paged ks0108
    LCD_page = new unsigned char *[width];
    if (LCD_page)
    {
        for (x = 0; x < width; x++)
        {
            LCD_page[x] = new unsigned char[(height + 7) / 8];
            memset(LCD_page[x], 0, (height + 7) / 8);
        }
    }

    if (config->device == "")
    {
        // use DirectIO
        if (port->Open(config->port) != 0)
            return -1;
        uSleep(10);
    }
    else
    {
        // use ppdev
        if (port->Open(config->device.c_str()) != 0)
            return -1;
    }

    if (nSleepInit() != 0)
    {
        syslog(LOG_DEBUG, "%s: INFO: cannot change wait parameters (cDriver::Init)\n", config->name.c_str());
        useSleepInit = false;
    }
    else
    {
        useSleepInit = true;
    }

    syslog(LOG_DEBUG, "%s: benchmark started.\n", config->name.c_str());
    gettimeofday(&tv1, 0);
    for (i = 0; i < 1000; i++)
    {
        port->WriteData(i % 0x100);
    }
    gettimeofday(&tv2, 0);
    if (useSleepInit)
        nSleepDeInit();
    timeForPortCmdInNs = (tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec);
    syslog(LOG_DEBUG, "%s: benchmark stopped. Time for Command: %ldns\n", config->name.c_str(), timeForPortCmdInNs);

    // initialize graphic mode
    InitGraphic();

    port->Release();

    *oldConfig = *config;

    // clear display
    Clear();

    syslog(LOG_INFO, "%s: KS0108 initialized.\n", config->name.c_str());
    return 0;
}

int cDriverKS0108::DeInit()
{
    int x;

    // free linear lcd array
    if (LCD)
    {
        for (x = 0; x < (width + 7) / 8; x++)
        {
            delete[] LCD[x];
        }
        delete[] LCD;
    }
    // free paged lcd array
    if (LCD_page)
    {
        for (x = 0; x < width; x++)
        {
            delete[] LCD_page[x];
        }
        delete[] LCD_page;
    }
    if (port->Close() != 0)
        return -1;
    return 0;
}

int cDriverKS0108::CheckSetup()
{
    if (config->device != oldConfig->device ||
        config->port != oldConfig->port ||
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

int cDriverKS0108::InitGraphic()
{
    // init controllers
    if (CS1 > -1) {
        KS0108Cmd(SEDS, 1);
        KS0108Cmd(SEPA, 1);
        KS0108Cmd(SEAD, 1);
        KS0108Cmd(DION, 1);
    }
    if (CS2 > -1) {
        KS0108Cmd(SEDS, 2);
        KS0108Cmd(SEPA, 2);
        KS0108Cmd(SEAD, 2);
        KS0108Cmd(DION, 2);
    }
    if (CS3 > -1) {
        KS0108Cmd(SEDS, 3);
        KS0108Cmd(SEPA, 3);
        KS0108Cmd(SEAD, 3);
        KS0108Cmd(DION, 3);
    }
    if (CS4 > -1) {
        KS0108Cmd(SEDS, 4);
        KS0108Cmd(SEPA, 4);
        KS0108Cmd(SEAD, 4);
        KS0108Cmd(DION, 4);
    }
    return 0;
}

void cDriverKS0108::KS0108Cmd(unsigned char data, int cs)
{
    if (useSleepInit)
        nSleepInit();
    switch (cs) {
        case 1:
            if (control == 1)
                port->WriteControl(CDHI | CS1 | CELO);
            else
                port->WriteControl(CDHI | CS1 | CEHI);
            nSleep((timeForLCDInNs + timeForPortCmdInNs) + 100 * config->adjustTiming);
            port->WriteData(data);
            nSleep((timeForLCDInNs + timeForPortCmdInNs) + 100 * config->adjustTiming);
            if (control == 1)
            {
                port->WriteControl(CDHI | CS1 | CEHI);
                nSleep((timeForLCDInNs + timeForPortCmdInNs) + 100 * config->adjustTiming);
            }
            port->WriteControl(CDHI | CS1 | CELO);
            nSleep((timeForLCDInNs + timeForPortCmdInNs) + 100 * config->adjustTiming);
            break;
        case 2:
            if (control == 1)
                port->WriteControl(CDHI | CS2 | CELO);
            else
                port->WriteControl(CDHI | CS2 | CEHI);
            nSleep((timeForLCDInNs + timeForPortCmdInNs) + 100 * config->adjustTiming);
            port->WriteData(data);
            nSleep((timeForLCDInNs + timeForPortCmdInNs) + 100 * config->adjustTiming);
            if (control == 1)
            {
                port->WriteControl(CDHI | CS2 | CEHI);
                nSleep((timeForLCDInNs + timeForPortCmdInNs) + 100 * config->adjustTiming);
            }
            port->WriteControl(CDHI | CS2 | CELO);
            nSleep((timeForLCDInNs + timeForPortCmdInNs) + 100 * config->adjustTiming);
            break;
        case 3:
            if (control == 1)
                port->WriteControl(CDHI | CS3 | CELO);
            else
                port->WriteControl(CDHI | CS3 | CEHI);
            nSleep((timeForLCDInNs + timeForPortCmdInNs) + 100 * config->adjustTiming);
            port->WriteData(data);
            nSleep((timeForLCDInNs + timeForPortCmdInNs) + 100 * config->adjustTiming);
            if (control == 1)
            {
                port->WriteControl(CDHI | CS3 | CEHI);
                nSleep((timeForLCDInNs + timeForPortCmdInNs) + 100 * config->adjustTiming);
            }
            port->WriteControl(CDHI | CS3 | CELO);
            nSleep((timeForLCDInNs + timeForPortCmdInNs) + 100 * config->adjustTiming);
            break;
        case 4:
            if (control == 1)
                port->WriteControl(CDHI | CS4 | CELO);
            else
                port->WriteControl(CDHI | CS4 | CEHI);
            nSleep((timeForLCDInNs + timeForPortCmdInNs) + 100 * config->adjustTiming);
            port->WriteData(data);
            nSleep((timeForLCDInNs + timeForPortCmdInNs) + 100 * config->adjustTiming);
            if (control == 1)
            {
                port->WriteControl(CDHI | CS4 | CEHI);
                nSleep((timeForLCDInNs + timeForPortCmdInNs) + 100 * config->adjustTiming);
            }
            port->WriteControl(CDHI | CS4 | CELO);
            nSleep((timeForLCDInNs + timeForPortCmdInNs) + 100 * config->adjustTiming);
            break;
    }
    if (useSleepInit)
        nSleepDeInit();
}

void cDriverKS0108::KS0108Data(unsigned char data, int cs)
{
    if (useSleepInit)
        nSleepInit();
    switch (cs) {
        case 1:
            if (control == 1)
                port->WriteControl(CDLO | CS1 | CELO);
            else
                port->WriteControl(CDLO | CS1 | CEHI);
            nSleep((timeForLCDInNs + timeForPortCmdInNs) + 100 * config->adjustTiming);
            port->WriteData(data);
            nSleep((timeForLCDInNs + timeForPortCmdInNs) + 100 * config->adjustTiming);
            if (control == 1)
            {
                port->WriteControl(CDLO | CS1 | CEHI);
                nSleep((timeForLCDInNs + timeForPortCmdInNs) + 100 * config->adjustTiming);
            }
            port->WriteControl(CDLO | CS1 | CELO);
            nSleep((timeForLCDInNs + timeForPortCmdInNs) + 100 * config->adjustTiming);
            break;
        case 2:
            if (control == 1)
                port->WriteControl(CDLO | CS2 | CELO);
            else
                port->WriteControl(CDLO | CS2 | CEHI);
            nSleep((timeForLCDInNs + timeForPortCmdInNs) + 100 * config->adjustTiming);
            port->WriteData(data);
            nSleep((timeForLCDInNs + timeForPortCmdInNs) + 100 * config->adjustTiming);
            if (control == 1)
            {
                port->WriteControl(CDLO | CS2 | CEHI);
                nSleep((timeForLCDInNs + timeForPortCmdInNs) + 100 * config->adjustTiming);
            }
            port->WriteControl(CDLO | CS2 | CELO);
            nSleep((timeForLCDInNs + timeForPortCmdInNs) + 100 * config->adjustTiming);
            break;
        case 3:
            if (control == 1)
                port->WriteControl(CDLO | CS3 | CELO);
            else
                port->WriteControl(CDLO | CS3 | CEHI);
            nSleep((timeForLCDInNs + timeForPortCmdInNs) + 100 * config->adjustTiming);
            port->WriteData(data);
            nSleep((timeForLCDInNs + timeForPortCmdInNs) + 100 * config->adjustTiming);
            if (control == 1)
            {
                port->WriteControl(CDLO | CS3 | CEHI);
                nSleep((timeForLCDInNs + timeForPortCmdInNs) + 100 * config->adjustTiming);
            }
            port->WriteControl(CDLO | CS3 | CELO);
            nSleep((timeForLCDInNs + timeForPortCmdInNs) + 100 * config->adjustTiming);
            break;
        case 4:
            if (control == 1)
                port->WriteControl(CDLO | CS4 | CELO);
            else
                port->WriteControl(CDLO | CS4 | CEHI);
            nSleep((timeForLCDInNs + timeForPortCmdInNs) + 100 * config->adjustTiming);
            port->WriteData(data);
            nSleep((timeForLCDInNs + timeForPortCmdInNs) + 100 * config->adjustTiming);
            if (control == 1)
            {
                port->WriteControl(CDLO | CS4 | CEHI);
                nSleep((timeForLCDInNs + timeForPortCmdInNs) + 100 * config->adjustTiming);
            }
            port->WriteControl(CDLO | CS4 | CELO);
            nSleep((timeForLCDInNs + timeForPortCmdInNs) + 100 * config->adjustTiming);
            break;
    }
    if (useSleepInit)
        nSleepDeInit();
}

void cDriverKS0108::Clear()
{
    for (int x = 0; x < (width + 7) / 8; x++)
        memset(LCD[x], 0, height);
}


void cDriverKS0108::SetPixel(int x, int y, uint32_t data)
{
    if (x >= width || y >= height)
        return;

    int pos = x % 8;
    if (config->upsideDown)
    {
        x = width - 1 - x;
        y = height - 1 - y;
    } else {
        pos = 7 - pos; // reverse bit position
    }

    if (data == GRAPHLCD_White)
        LCD[x / 8][y] |= (1 << pos);
    else
        LCD[x / 8][y] &= ( 0xFF ^ (1 << pos) );
}


#if 0
void cDriverKS0108::Set8Pixels(int x, int y, unsigned char data)
{
    if (x >= width || y >= height)
        return;

    if (!config->upsideDown)
    {
        // normal orientation
        LCD[x / 8][y] = LCD[x / 8][y] | data;
    }
    else
    {
        // upside down orientation
        x = width - 1 - x;
        y = height - 1 - y;
        LCD[x / 8][y] = LCD[x / 8][y] | ReverseBits(data);
    }
}
#endif

void cDriverKS0108::Refresh(bool refreshAll)
{
    int   x,y;
    int   xx,yy;
    unsigned char dByte, oneBlock[8];

    if (CheckSetup() > 0)
        refreshAll = true;

    if (config->refreshDisplay > 0)
    {
        refreshCounter = (refreshCounter + 1) % config->refreshDisplay;
        if (!refreshAll && !refreshCounter)
            refreshAll=true;
    }

    refreshAll = true; // differential update is not yet supported

    if (refreshAll)
    {
        // draw all

        // convert the linear lcd array to the paged array for the display
        for (y = 0; y < (height + 7) / 8; y++)
        {
            for (x = 0; x < (width + 7) / 8; x++)
            {
                for (yy = 0; yy < 8; yy++)
                {
                    oneBlock[yy] = LCD[x][yy + (y * 8)] ^ (config->invert ? 0xff : 0x00);
                }
                for (xx = 0; xx < 8; xx++)
                {
                    dByte = 0;
                    for (yy = 0; yy < 8; yy++)
                    {
                        if (oneBlock[yy] & bitmask[xx])
                        {
                            dByte += (1 << yy);
                        }
                    }
                    LCD_page[x * 8 + xx][y] = dByte;
                }
            }
        }

        port->Claim();

        if (width == 128 && height == 64) {
            for (y = 0; y < 64/8; y++) {
                KS0108Cmd(SEPA + y, 1);
                KS0108Cmd(SEAD, 1);

                for (x = 0; x < 64; x++) {
                    KS0108Data(LCD_page[x][y], 1);
                }
            }

            for (y = 0; y < 64/8; y++) {
                KS0108Cmd(SEPA + y, 2);
                KS0108Cmd(SEAD, 2);

                for (x = 64; x < 128; x++) {
                    KS0108Data(LCD_page[x][y], 2);
                }
            }
        }

        if (width > 128 && height == 64) {
            for (y = 0; y < 64/8; y++) {
                KS0108Cmd(SEPA + y, 1);
                KS0108Cmd(SEAD, 1);

                for (x = 0; x < 64; x++) {
                    KS0108Data(LCD_page[x][y], 1);
                }
            }

            for (y = 0; y < 64/8; y++) {
                KS0108Cmd(SEPA + y, 2);
                KS0108Cmd(SEAD, 2);
                for (x = 64; x < 128; x++) {
                    KS0108Data(LCD_page[x][y], 2);
                }
            }

            for (y = 0; y < 64/8; y++) {
                KS0108Cmd(SEPA + y, 3);
                KS0108Cmd(SEAD, 3);

                for (x = 128; x < 192; x++) {
                    KS0108Data(LCD_page[x][y], 3);
                }
            }

            for (y = 0; y < 64/8; y++) {
                if (width > 192) {
                    KS0108Cmd(SEPA + y, 4);
                    KS0108Cmd(SEAD, 4);
                    for (x = 192; x < 256; x++) {
                        KS0108Data(LCD_page[x][y], 4);
                    }
                }
            }
        }

        if (width == 128 && height == 128) {
            for (y = 0; y < 64/8; y++) {
                KS0108Cmd(SEPA + y, 1);
                KS0108Cmd(SEAD, 1);

                for (x = 0; x < 64; x++) {
                    KS0108Data(LCD_page[x][y], 1);
                }
            }

            for (y = 0; y < 64/8; y++) {
                KS0108Cmd(SEPA + y, 2);
                KS0108Cmd(SEAD, 2);

                for (x = 64; x < 128; x++) {
                    KS0108Data(LCD_page[x][y], 2);
                }
            }

            for (y = 0; y < 64/8; y++) {
                KS0108Cmd(SEPA + y, 3);
                KS0108Cmd(SEAD, 3);

                for (x = 0; x < 64; x++) {
                    KS0108Data(LCD_page[x][y+8], 3);
                }
            }

            for (y = 0; y < 64/8; y++) {
                KS0108Cmd(SEPA + y, 4);
                KS0108Cmd(SEAD, 4);

                for (x = 64; x < 128; x++) {
                    KS0108Data(LCD_page[x][y+8], 4);
                }
            }
        }
        port->WriteData(0);
        port->Release();
    }
    else
    {
        // draw only the changed bytes
    }
}

} // end of namespace
