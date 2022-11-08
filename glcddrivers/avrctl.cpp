/*
 * GraphLCD driver library
 *
 * avrctl.c  -  AVR controlled LCD driver class
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2005-2010 Andreas Regel <andreas.regel AT powarman.de>
 * (c) 2011      Wolfgang Astleitner <mrwastl AT users.sourceforge.net>
 */

#include <stdint.h>
#include <syslog.h>
#include <cstring>

#include "common.h"
#include "config.h"
#include "port.h"
#include "avrctl.h"


namespace GLCD
{

/* command header:
**  8 bits  sync byte (0xAA for sent commands, 0x55 for received commands)
**  8 bits  command id
** 16 bits  command length (excluding header)
*/
const unsigned char CMD_HDR_SYNC    = 0;
const unsigned char CMD_HDR_COMMAND = 1;
const unsigned char CMD_HDR_LENGTH  = 2;
const unsigned char CMD_DATA_START  = 4;

const unsigned char CMD_SYNC_SEND = 0xAA;
const unsigned char CMD_SYNC_RECV = 0x55;

const unsigned char CMD_SYS_SYNC = 0x00;
const unsigned char CMD_SYS_ACK  = 0x01;

const unsigned char CMD_DISP_CLEAR_SCREEN   = 0x10;
const unsigned char CMD_DISP_SWITCH_SCREEN  = 0x11;
const unsigned char CMD_DISP_SET_BRIGHTNESS = 0x12;
const unsigned char CMD_DISP_SET_COL_DATA   = 0x13;
const unsigned char CMD_DISP_SET_ROW_DATA   = 0x14;
const unsigned char CMD_DISP_UPDATE         = 0x15;

const int kBufferWidth  = 256;
const int kBufferHeight = 128;


cDriverAvrCtl::cDriverAvrCtl(cDriverConfig * config)
:   cDriver(config)
{
    port = new cSerialPort();

    //width = config->width;
    //height = config->height;
    refreshCounter = 0;
}

cDriverAvrCtl::~cDriverAvrCtl()
{
    delete port;
}

int cDriverAvrCtl::Init()
{
    int x;

    width = config->width;
    if (width <= 0)
        width = 256;
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
    newLCD = new unsigned char*[kBufferWidth];
    if (newLCD)
    {
        for (x = 0; x < kBufferWidth; x++)
        {
            newLCD[x] = new unsigned char[(kBufferHeight + 7) / 8];
            memset(newLCD[x], 0, (kBufferHeight + 7) / 8);
        }
    }
    // setup lcd array (current state)
    oldLCD = new unsigned char*[kBufferWidth];
    if (oldLCD)
    {
        for (x = 0; x < kBufferWidth; x++)
        {
            oldLCD[x] = new unsigned char[(kBufferHeight + 7) / 8];
            memset(oldLCD[x], 0, (kBufferHeight + 7) / 8);
        }
    }

    if (config->device == "")
    {
        return -1;
    }
    if (port->Open(config->device.c_str()) != 0)
        return -1;

    *oldConfig = *config;

    // clear display
    Clear();

    syslog(LOG_INFO, "%s: AvrCtl initialized.\n", config->name.c_str());
    return 0;
}

int cDriverAvrCtl::DeInit()
{
    int x;
    // free lcd array (wanted state)
    if (newLCD)
    {
        for (x = 0; x < kBufferWidth; x++)
        {
            delete[] newLCD[x];
        }
        delete[] newLCD;
    }
    // free lcd array (current state)
    if (oldLCD)
    {
        for (x = 0; x < kBufferWidth; x++)
        {
            delete[] oldLCD[x];
        }
        delete[] oldLCD;
    }

    if (port->Close() != 0)
        return -1;
    return 0;
}

int cDriverAvrCtl::CheckSetup()
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

void cDriverAvrCtl::Clear()
{
    for (int x = 0; x < kBufferWidth; x++)
        memset(newLCD[x], 0, (kBufferHeight + 7) / 8);
}


void cDriverAvrCtl::SetPixel(int x, int y, uint32_t data)
{
    if (x >= width || y >= height)
        return;

    if (config->upsideDown)
    {
        x = width - 1 - x;
        y = height - 1 - y;
    }

    int offset = 7 - (y % 8);
    if (data == GRAPHLCD_White)
        newLCD[x][y / 8] |= (1 << offset);
    else
        newLCD[x][y / 8] &= ( 0xFF ^ (1 << offset) );
}


#if 0
void cDriverAvrCtl::Set8Pixels(int x, int y, unsigned char data)
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

void cDriverAvrCtl::Refresh(bool refreshAll)
{
    int x;
    int y;
    int i;
    int num = kBufferWidth / 2;
    unsigned char data[16*num];

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
        for (x = 0; x < kBufferWidth; x += num)
        {
            for (i = 0; i < num; i++)
            {
                for (y = 0; y < (kBufferHeight + 7) / 8; y++)
                {
                    data[i * ((kBufferHeight + 7) / 8) + y] = (newLCD[x + i][y]) ^ (config->invert ? 0xff : 0x00);
                }
                memcpy(oldLCD[x + i], newLCD[x + i], (kBufferHeight + 7) / 8);
            }
            CmdDispSetColData(x, 0, 16 * num, data);
        }
        CmdDispUpdate();
        CmdDispSwitchScreen();
        // and reset RefreshCounter
        refreshCounter = 0;
    }
    else
    {
        // draw only the changed bytes
    }
}

void cDriverAvrCtl::SetBrightness(unsigned int percent)
{
  CmdDispSetBrightness(percent);
}

int cDriverAvrCtl::WaitForAck(void)
{
    uint8_t cmd[4];
    int len;
    int timeout = 10000;

    len = 0;
    while (len < 4 && timeout > 0)
    {
        len += port->ReadData(&cmd[len]);
        timeout--;
    }
    if (timeout == 0)
        return 0;
    return 1;
}

void cDriverAvrCtl::CmdSysSync(void)
{
    uint8_t cmd[4];

    cmd[CMD_HDR_SYNC] = CMD_SYNC_SEND;
    cmd[CMD_HDR_COMMAND] = CMD_SYS_SYNC;
    cmd[CMD_HDR_LENGTH] = 0;
    cmd[CMD_HDR_LENGTH+1] = 0;

    port->WriteData(cmd, 4);
    WaitForAck();
}

void cDriverAvrCtl::CmdDispClearScreen(void)
{
    uint8_t cmd[4];

    cmd[CMD_HDR_SYNC] = CMD_SYNC_SEND;
    cmd[CMD_HDR_COMMAND] = CMD_DISP_CLEAR_SCREEN;
    cmd[CMD_HDR_LENGTH] = 0;
    cmd[CMD_HDR_LENGTH+1] = 0;

    port->WriteData(cmd, 4);
    WaitForAck();
}

void cDriverAvrCtl::CmdDispSwitchScreen(void)
{
    uint8_t cmd[4];

    cmd[CMD_HDR_SYNC] = CMD_SYNC_SEND;
    cmd[CMD_HDR_COMMAND] = CMD_DISP_SWITCH_SCREEN;
    cmd[CMD_HDR_LENGTH] = 0;
    cmd[CMD_HDR_LENGTH+1] = 0;

    port->WriteData(cmd, 4);
    WaitForAck();
}

void cDriverAvrCtl::CmdDispSetBrightness(uint8_t percent)
{
    uint8_t cmd[5];

    cmd[CMD_HDR_SYNC] = CMD_SYNC_SEND;
    cmd[CMD_HDR_COMMAND] = CMD_DISP_SET_BRIGHTNESS;
    cmd[CMD_HDR_LENGTH] = 0;
    cmd[CMD_HDR_LENGTH+1] = 1;
    cmd[CMD_DATA_START] = percent;

    port->WriteData(cmd, 5);
    WaitForAck();
}

void cDriverAvrCtl::CmdDispSetColData(uint16_t column, uint16_t offset, uint16_t length, uint8_t * data)
{
    uint8_t cmd[2560];

    cmd[CMD_HDR_SYNC] = CMD_SYNC_SEND;
    cmd[CMD_HDR_COMMAND] = CMD_DISP_SET_COL_DATA;
    cmd[CMD_HDR_LENGTH] = (length + 6) >> 8;
    cmd[CMD_HDR_LENGTH+1] = (length + 6);
    cmd[CMD_DATA_START] = column >> 8;
    cmd[CMD_DATA_START+1] = column;
    cmd[CMD_DATA_START+2] = offset >> 8;
    cmd[CMD_DATA_START+3] = offset;
    cmd[CMD_DATA_START+4] = length >> 8;
    cmd[CMD_DATA_START+5] = length;
    memcpy(&cmd[CMD_DATA_START+6], data, length);

    port->WriteData(cmd, length+10);
    WaitForAck();
}

void cDriverAvrCtl::CmdDispUpdate(void)
{
    uint8_t cmd[4];

    cmd[CMD_HDR_SYNC] = CMD_SYNC_SEND;
    cmd[CMD_HDR_COMMAND] = CMD_DISP_UPDATE;
    cmd[CMD_HDR_LENGTH] = 0;
    cmd[CMD_HDR_LENGTH+1] = 0;

    port->WriteData(cmd, 4);
    WaitForAck();
}

}
