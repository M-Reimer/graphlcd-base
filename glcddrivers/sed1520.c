/*
 * GraphLCD driver library
 *
 * sed1520.c  -  SED1520 driver class
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
#include "port.h"
#include "sed1520.h"


namespace GLCD
{

// commands
const unsigned char kSEAD = 0x00; // Set (X) Column Address
const unsigned char kSEPA = 0xb8; // Set (Y) Page Address
const unsigned char kSEDS = 0xc0;  // Set Display Start Line
const unsigned char kDION = 0xaf; // Display on
const unsigned char kDIOF = 0xae; // Display off

// control bits for DirectIO
#define CE1     0x01
#define CE1HI   0x01  // Chip Enable 1 on
#define CE1LO   0x00  // Chip Enable 1 off

const unsigned char kCS1HI = 0x00; // Chip Select 1
const unsigned char kCS1LO = 0x01;
const unsigned char kCS2HI = 0x04; // Chip Select 2
const unsigned char kCS2LO = 0x00;
const unsigned char kCDHI  = 0x08; // Command/Data Register Select
const unsigned char kCDLO  = 0x00;
const unsigned char kLEDHI = 0x02; // LED Backlight (not supported currently)
const unsigned char kLEDLO = 0x00;


cDriverSED1520::cDriverSED1520(cDriverConfig * config)
:   cDriver(config)
{
    port = new cParallelPort();

    refreshCounter = 0;
}

cDriverSED1520::~cDriverSED1520()
{
    delete port;
}

int cDriverSED1520::Init()
{
    int x;
    int i;
    struct timeval tv1, tv2;

    if (!(config->width % 8) == 0) {
        width = config->width + (8 - (config->width % 8));
    } else {
        width = config->width;
    }

    if (!(config->height % 8) == 0) {
        height = config->height + (8 - (config->height % 8));
    } else {
        height = config->height;
    }

    if (width <= 0)
        width = 120;
    if (height <= 0)
        height = 32;

    SEAD = kSEAD;
    SEPA = kSEPA;
    SEDS = kSEDS;
    DION = kDION;
    DIOF = kDIOF;
    LED  = kLEDHI;
    CDHI = kCDHI;
    CDLO = kCDLO;
    CS1HI = kCS1HI;
    CS1LO = kCS1LO;
    CS2HI = kCS2HI;
    CS2LO = kCS2LO;

    for (unsigned int i = 0; i < config->options.size(); i++)
    {
        if (config->options[i].name == "")
        {
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
    // setup the lcd array for the paged sed1520
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

    syslog(LOG_INFO, "%s: SED1520 initialized.\n", config->name.c_str());
    return 0;
}

int cDriverSED1520::DeInit()
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
    // free the lcd array for the paged sed1520
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

int cDriverSED1520::CheckSetup()
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

int cDriverSED1520::InitGraphic()
{
    // initialize controller1, set display start 0, set page 0, set y address 0, display on
    SED1520Cmd(SEDS, 1);
    SED1520Cmd(SEPA, 1);
    SED1520Cmd(SEAD, 1);
    SED1520Cmd(DION, 1);

    // initialize controller2, set display start 0, set page 0, set y address 0, display on
    SED1520Cmd(SEDS, 2);
    SED1520Cmd(SEPA, 2);
    SED1520Cmd(SEAD, 2);
    SED1520Cmd(DION, 2);

    return 0;
}

void cDriverSED1520::SED1520Cmd(unsigned char data, int cmdcs)
{
    if (useSleepInit)
        nSleepInit();

    switch (cmdcs)
    {
        case 1:
            port->WriteControl(CDHI | CS1LO | CS2LO | LEDHI);
            nSleep(450 - timeForPortCmdInNs + 100 * config->adjustTiming);
            port->WriteData(data);
            nSleep(650 - timeForPortCmdInNs + 100 * config->adjustTiming);
            port->WriteControl(CDHI | CS1HI | CS2LO | LEDHI);
            nSleep(450 - timeForPortCmdInNs + 100 * config->adjustTiming);
            break;
        case 2:
            port->WriteControl(CDHI | CS1LO | CS2LO | LED);
            nSleep(450 - timeForPortCmdInNs + 100 * config->adjustTiming);
            port->WriteData(data);
            nSleep(650 - timeForPortCmdInNs + 100 * config->adjustTiming);
            port->WriteControl(CDHI | CS1LO | CS2HI | LED);
            nSleep(450 - timeForPortCmdInNs + 100 * config->adjustTiming);
            break;
    }
    if (useSleepInit)
        nSleepDeInit();
}

void cDriverSED1520::SED1520Data(unsigned char data, int datacs)
{
    if (useSleepInit)
        nSleepInit();

    switch (datacs)
    {
        case 1:
            port->WriteControl(CDLO | CS1LO | CS2LO | LEDHI);
            nSleep(450 - timeForPortCmdInNs + 100 * config->adjustTiming);
            port->WriteData(data);
            nSleep(650 - timeForPortCmdInNs + 100 * config->adjustTiming);
            port->WriteControl(CDLO | CS1HI | CS2LO | LEDHI);
            nSleep(450 - timeForPortCmdInNs + 100 * config->adjustTiming);
            break;
        case 2:
            port->WriteControl(CDLO | CS1LO | CS2LO | LED);
            nSleep(450 - timeForPortCmdInNs + 100 * config->adjustTiming);
            port->WriteData(data);
            nSleep(650 - timeForPortCmdInNs + 100 * config->adjustTiming);
            port->WriteControl(CDLO | CS1LO | CS2HI | LED);
            nSleep(450 - timeForPortCmdInNs + 100 * config->adjustTiming);
            break;
    }
    if (useSleepInit)
        nSleepDeInit();
}

void cDriverSED1520::Clear()
{
    for (int x = 0; x < (width + 7) / 8; x++)
        memset(LCD[x], 0, height);
}


void cDriverSED1520::SetPixel (int x, int y, uint32_t data)
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
        LCD[x / 8][y] |= ( 1 << pos );
    else
        LCD[x / 8][y] &= (0xFF ^ ( 1 << pos ));
}


#if 0
void cDriverSED1520::Set8Pixels (int x, int y, unsigned char data)
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

void cDriverSED1520::Refresh(bool refreshAll)
{
    int x,y,xx,yy;
    unsigned char dByte, oneBlock[8];

    if (CheckSetup() > 0)
        refreshAll = true;

    if (config->refreshDisplay > 0)
    {
        refreshCounter = (refreshCounter + 1) % config->refreshDisplay;
        if (!refreshAll && !refreshCounter)
            refreshAll = true;
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

        // send lcd_soll data to display, controller 1
        // set page and start address
        for (y = 0; y < (height + 7) / 8; y++)
        {
            SED1520Cmd(SEAD, 1);
            SED1520Cmd(SEPA + y, 1);
            SED1520Data(0x00 ^ (config->invert ? 0xff : 0x00), 1); // fill first row with zero

            for (x = 0; x < width / 2 + 1; x++)
            {
                SED1520Data(LCD_page[x][y], 1);
            }

            SED1520Cmd(SEAD, 2);
            SED1520Cmd(SEPA + y, 2);

            for (x = width / 2; x < width; x++)
            {
                SED1520Data(LCD_page[x][y], 2);
            }

            SED1520Data(0x00 ^ (config->invert ? 0xff : 0x00), 2); // fill last row with zero
        }
        port->Release();
    }
    else
    {
        // draw only the changed bytes
    }
}

} // end of namespace
