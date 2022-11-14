/*
 * GraphLCD driver library
 *
 * hd61830.c  -  HD61830 driver class
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2001-2004 Carsten Siebholz <c.siebholz AT t-online.de>
 * (c) 2005-2010 Andreas Regel <andreas.regel AT powarman.de>
 * (c) 2011      Wolfgang Astleitner <mrwastl AT users.sourceforge.net>
 */

#include <syslog.h>
#include <sys/time.h>
#include <cstring>

#include "common.h"
#include "config.h"
#include "hd61830.h"
#include "port.h"


namespace GLCD
{

// commands
#define MCNT    0x00
#define CPIT    0x01
#define NOCH    0x02
#define NOTD    0x03
#define CPOS    0x04

#define DSAL    0x08
#define DSAH    0x09
#define CACL    0x0A
#define CACH    0x0B

#define WDDI    0x0C
#define RDDI    0x0D

#define CBIT    0x0E
#define SBIT    0x0F

// control bits for DirectIO
#define EN      0x01
#define ENHI    0x00
#define ENLO    0x01

#define RW      0x02
#define RWHI    0x00
#define RWLO    0x02

#define RS      0x04
#define RSHI    0x04
#define RSLO    0x00


cDriverHD61830::cDriverHD61830(cDriverConfig * config)
:   cDriver(config)
{
    port = new cParallelPort();

    useSleepInit = false;

    refreshCounter = 0;
    timeForPortCmdInNs = 0;
}

cDriverHD61830::~cDriverHD61830()
{
    delete port;
}

int cDriverHD61830::Init()
{
    int i;
    int x;
    struct timeval tv1, tv2;

    width = config->width;
    if (width <= 0)
        width = 240;
    height = config->height;
    if (height <= 0)
        height = 128;

    for (unsigned int i = 0; i < config->options.size(); i++)
    {
        if (config->options[i].name == "")
        {
        }
    }

    // setup lcd array (wanted state)
    newLCD = new unsigned char *[(width + 7) / 8];
    if (newLCD)
    {
        for (x = 0; x < (width + 7) / 8; x++)
        {
            newLCD[x] = new unsigned char[height];
            memset(newLCD[x], 0, height);
        }
    }
    // setup lcd array (current state)
    oldLCD = new unsigned char*[(width + 7) / 8];
    if (oldLCD)
    {
        for (x = 0; x < (width + 7) / 8; x++)
        {
            oldLCD[x] = new unsigned char[height];
            memset(oldLCD[x], 0, height);
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
        port->WriteData(1 % 0x100);
    }
    gettimeofday(&tv2, 0);
    if (useSleepInit)
        nSleepDeInit();
    timeForPortCmdInNs = (tv2.tv_sec-tv1.tv_sec) * 1000000 + (tv2.tv_usec-tv1.tv_usec);
    syslog(LOG_DEBUG, "%s: benchmark stopped. Time for Port Command: %ldns\n", config->name.c_str(), timeForPortCmdInNs);

    // initialize graphic mode
    InitGraphic();

    port->Release();

    *oldConfig = *config;

    // clear display
    Clear();

    syslog(LOG_INFO, "%s: HD61830 initialized.\n", config->name.c_str());
    return 0;
}

int cDriverHD61830::DeInit()
{
    int x;

    // free lcd array (wanted state)
    if (newLCD)
    {
        for (x = 0; x < (width + 7) / 8; x++)
        {
            delete[] newLCD[x];
        }
        delete[] newLCD;
    }
    // free lcd array (current state)
    if (oldLCD)
    {
        for (x = 0; x < (width + 7) / 8; x++)
        {
            delete[] oldLCD[x];
        }
        delete[] oldLCD;
    }
    if (port->Close() != 0)
        return -1;
    return 0;
}

int cDriverHD61830::CheckSetup()
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

int cDriverHD61830::InitGraphic()
{
    Write(MCNT, 0x32); // set Mode Control Register
    // DISP ON, MASTER ON, BLINK OFF, CURSOR OFF, GRAPHIC-Mode, int.Clock
    Write(CPIT, 0x07); // set Character Pitch Register
    // 8 pixels per byte
    Write(NOCH, std::max(1, (width + 7) / 8 - 1)); // set Number-Of-Characters Register
    // (width - 1) / 8 bytes per line horizontally
    Write(NOTD, std::max(1, height - 1)); // set Number-Of-Time-Divisions Register
    // height - 1
    Write(CPOS, 0x00); // set Cursor Position Register
    // optional, because we havn't enabled a cursor
    Write(DSAL, 0x00); // set Display Start Address Register (Low Order Byte)
    Write(DSAH, 0x00); // set Display Start Address Register (High Order Byte)
    Write(CACL, 0x00); // set Cursor Address Counter Register (Low Order Byte)
    Write(CACH, 0x00); // set Cursor Address Counter Register (High Order Byte)

    return 0;
}

void cDriverHD61830::Write(unsigned char cmd, unsigned char data)
{
    if (useSleepInit)
        nSleepInit();

    // set RS high (instruction), RW low (write) and E low
    port->WriteControl(RSHI | RWLO | ENLO);
    nSleep(140 - timeForPortCmdInNs + 100 * config->adjustTiming);

    // Output the actual command
    port->WriteData(cmd);

    // set E high
    port->WriteControl(RSHI | RWLO | ENHI);
    nSleep(450 - timeForPortCmdInNs + 100 * config->adjustTiming);

    // set E low
    port->WriteControl(RSHI | RWLO | ENLO);
    nSleep(450 - timeForPortCmdInNs + 100 * config->adjustTiming);


    // set RS low (data), RW low (write) and E low
    port->WriteControl(RSLO | RWLO | ENLO);
    nSleep(140 - timeForPortCmdInNs + 100 * config->adjustTiming);

    // Output the actual data
    port->WriteData(data);

    // set E high
    port->WriteControl(RSLO | RWLO | ENHI);
    nSleep(450 - timeForPortCmdInNs + 100 * config->adjustTiming);

    // set E low
    port->WriteControl(RSLO | RWLO | ENLO);
    nSleep(450 - timeForPortCmdInNs + 100 * config->adjustTiming);

    switch (cmd)
    {
        case MCNT:
        case CPIT:
        case NOCH:
        case NOTD:
        case CPOS:
        case DSAL:
        case DSAH:
        case CACL:
        case CACH:
            nSleep(4000 - std::max(450l, timeForPortCmdInNs) + 100 * config->adjustTiming);
            break;
        case WDDI:
        case RDDI:
            nSleep(6000 - std::max(450l, timeForPortCmdInNs) + 100 * config->adjustTiming);
            break;
        case CBIT:
        case SBIT:
            nSleep(36000 - std::max(450l, timeForPortCmdInNs) + 100 * config->adjustTiming);
            break;
    }
    if (useSleepInit)
        nSleepDeInit();
}

void cDriverHD61830::Clear()
{
    for (int x = 0; x < (width + 7) / 8; x++)
        memset(newLCD[x], 0, height);
}


void cDriverHD61830::SetPixel(int x, int y, uint32_t data)
{
    if (x >= width || y >= height)
        return;

    int pos = x % 8;
    if (config->upsideDown)
    {
        x = width - 1 - x;
        y = height - 1 - y;
    //} else {
        pos = 7 - pos; // reverse bit position
    }

    if (data == GRAPHLCD_White)
        newLCD[x / 8][y] |= ( 1 << pos );
    else
        newLCD[x / 8][y] &= ( 0xFF ^ ( 1 << pos ) );
}


#if 0
void cDriverHD61830::Set8Pixels(int x, int y, unsigned char data)
{
    if (x >= width || y >= height)
        return;

    if (!config->upsideDown)
    {
        // normal orientation
        newLCD[x / 8][y] = newLCD[x / 8][y] | ReverseBits(data);
    }
    else
    {
        // upside down orientation
        x = width - 1 - x;
        y = height - 1 - y;
        newLCD[x / 8][y] = newLCD[x / 8][y] | data;
    }
}
#endif

void cDriverHD61830::Refresh(bool refreshAll)
{
    int x;
    int y;
    int pos = 0;

    if (CheckSetup() > 0)
        refreshAll = true;

    if (config->refreshDisplay > 0)
    {
        refreshCounter = (refreshCounter + 1) % config->refreshDisplay;
        if (!refreshAll && !refreshCounter)
            refreshAll = true;
    }

    port->Claim();

    if (refreshAll)
    {
        // draw all

        for (y = 0; y < height; y++)
        {
            for (x = 0; x < (width + 7) / 8; x++)
            {
                // (re-setting the cursor position
                //  might be removed, when the graphic glitches are solved)
                Write(CACL, (pos % 0x100));
                Write(CACH, (pos / 0x100));
                Write(WDDI, (newLCD[x][y]) ^ (config->invert ? 0xff : 0x00));
                oldLCD[x][y] = newLCD[x][y];
                pos++;
            }
        }
        // and reset RefreshCounter
        refreshCounter = 0;
    }
    else
    {
        // draw only the changed bytes

        bool cs = false;
        for (y = 0; y < height; y++)
        {
            for (x = 0; x < (width + 7) / 8; x++)
            {
                if (newLCD[x][y] != oldLCD[x][y])
                {
                    if (!cs)
                    {
                        Write(CACL, (pos % 0x100));
                        Write(CACH, (pos / 0x100));
                        cs = true;
                    }
                    Write(WDDI, (newLCD[x][y]) ^ (config->invert ? 0xff : 0x00));
                    oldLCD[x][y] = newLCD[x][y];
                }
                else
                {
                    cs = false;
                }
                pos++;
            }
        }
    }
    port->Release();
}

} // end of namespace
