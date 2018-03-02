/*
 * GraphLCD driver library
 *
 * usbserlcd.c  -  USBserLCD driver class
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2017 Manuel Reimer <manuel.reimer@gmx.de>
 */

#include <syslog.h>
#include <cstring>
#include <cstdlib>
#include <unistd.h>

#include "common.h"
#include "config.h"
#include "port.h"
#include "usbserlcd.h"
#include "stdint.h"

const int BAUD_RATE = 150000;
const int BOOT_TIME = 300000;

// Possible serial package types
enum glcd_pkgtype: char {
  PKGTYPE_DATA,
  PKGTYPE_BRIGHTNESS
};

namespace GLCD
{

cDriverUSBserLCD::cDriverUSBserLCD(cDriverConfig * config)
:   cDriver(config)
{
    port = new cSerialPort();
    refreshCounter = 0;
    FS = 8;
    brightness = 255;
}

cDriverUSBserLCD::~cDriverUSBserLCD()
{
    delete port;
}

int cDriverUSBserLCD::Init()
{
    width = config->width;
    if (width <= 0)
        width = 240;
    height = config->height;
    if (height <= 0)
        height = 128;

    // setup lcd array (wanted state)
    newLCD = new unsigned char*[(width + (FS - 1)) / FS];
    if (newLCD)
    {
        for (int x = 0; x < (width + (FS - 1)) / FS; x++)
        {
            newLCD[x] = new unsigned char[height];
            memset(newLCD[x], 0, height);
        }
    }
    // setup lcd array (current state)
    oldLCD = new unsigned char*[(width + (FS - 1)) / FS];
    if (oldLCD)
    {
        for (int x = 0; x < (width + (FS - 1)) / FS; x++)
        {
            oldLCD[x] = new unsigned char[height];
            memset(oldLCD[x], 0, height);
        }
    }

    if (config->device == "")
        return -1;

    if (port->Open(config->device.c_str()) != 0)
        return -1;

    port->SetBaudRate(BAUD_RATE);

    *oldConfig = *config;

    // clear display
    Clear();

    // Disable the "hangup" signal to prevent Arduino reboots on connect.
    // We can't catch the first reboot with this (Hangup flag was set).
    // In this case, wait some milliseconds and do a full refresh.
    if (port->DisableHangup()) {
        usleep(BOOT_TIME);
        Refresh(true);
    }

    syslog(LOG_INFO, "%s: USBserLCD initialized.\n", config->name.c_str());
    return 0;
}

int cDriverUSBserLCD::DeInit()
{
    int x;
    // free lcd array (wanted state)
    if (newLCD)
    {
        for (x = 0; x < (width + (FS - 1)) / FS; x++)
        {
            delete[] newLCD[x];
        }
        delete[] newLCD;
    }
    // free lcd array (current state)
    if (oldLCD)
    {
        for (x = 0; x < (width + (FS - 1)) / FS; x++)
        {
            delete[] oldLCD[x];
        }
        delete[] oldLCD;
    }

    if (port->Close() != 0)
        return -1;
    return 0;
}

int cDriverUSBserLCD::CheckSetup()
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

void cDriverUSBserLCD::Clear()
{
    for (int x = 0; x < (width + (FS - 1)) / FS; x++)
        memset(newLCD[x], 0, height);
}


void cDriverUSBserLCD::SetPixel(int x, int y, uint32_t data)
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
        newLCD[x / 8][y] |= (1 << pos);
    else
        newLCD[x / 8][y] &= ( 0xFF ^ (1 << pos) );
}

void cDriverUSBserLCD::Refresh(bool refreshAll)
{
    int x,y;
    uint16_t addr = 0;

    if (CheckSetup() == 1)
        refreshAll = true;

    if (config->refreshDisplay > 0)
    {
        refreshCounter = (refreshCounter + 1) % config->refreshDisplay;
        if (!refreshAll && !refreshCounter)
            refreshAll = true;
    }

    // draw all
    cDriverUSBserLCDBuffer full_seq(8500);
    std::string bytes = "";
    bytes.reserve(8500);
    for (y = 0; y < height; y++)
    {
        for (x = 0; x < (width + (FS - 1)) / FS; x++)
        {
            char byte = (newLCD[x][y]) ^ (config->invert ? 0xff : 0x00);
            bytes.append(&byte, 1);
            if (refreshAll)
                oldLCD[x][y] = newLCD[x][y];
        }
    }
    full_seq.Append(bytes, 0);

    if (refreshAll) {
        port->WriteData(full_seq);
        // and reset RefreshCounter
        refreshCounter = 0;
        return;
    }

    // draw only the changed bytes
    cDriverUSBserLCDBuffer part_seq(8500);
    bool cs = false;
    bytes = "";
    for (y = 0; y < height; y++)
    {
        for (x = 0; x < (width + (FS - 1)) / FS; x++)
        {
            if (oldLCD[x][y] != newLCD[x][y])
            {
                if (!cs)
                {
                    if (width % FS == 0)
                        addr = (y * (width / FS)) + x;
                    else
                        addr = (y * (width / FS + 1)) + x;
                    bytes = "";
                    cs = true;
                }
                char byte = (newLCD[x][y]) ^ (config->invert ? 0xff : 0x00);
                bytes.append(&byte, 1);
                oldLCD[x][y] = newLCD[x][y];
            }
            else if (bytes != "")
            {
                part_seq.Append(bytes, addr);
                cs = false;
                bytes = "";
            }
        }
    }

    if (bytes != "")
        part_seq.Append(bytes, addr);

    // Send the smaller data block.
    if (part_seq.GetLength() < full_seq.GetLength())
        port->WriteData(part_seq);
    else
        port->WriteData(full_seq);
}

void cDriverUSBserLCD::SetBrightness(unsigned int percent)
{
    unsigned char brightness = 255 * percent / 100;
    std::string pkg = "GLCD";
    pkg += PKGTYPE_BRIGHTNESS;
    pkg += brightness;
    port->WriteData(pkg);
}


cDriverUSBserLCDBuffer::cDriverUSBserLCDBuffer(int aExpectedBytes)
{
    buffer = "";
    buffer.reserve(aExpectedBytes);
}
void  cDriverUSBserLCDBuffer::Append(std::string aBytes, uint16_t aAddress)
{
    buffer += "GLCD";
    buffer += PKGTYPE_DATA;
    buffer.append((char*)&aAddress, 2);
    uint16_t len = aBytes.length();
    buffer.append((char*)&len, 2);
    buffer.append(aBytes);
}
int  cDriverUSBserLCDBuffer::GetLength() const
{
    return buffer.length();
}
cDriverUSBserLCDBuffer::operator std::string() const
{
    return buffer;
}


} // end of namespace
