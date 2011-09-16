/*
 * GraphLCD driver library
 *
 * gu256x64-3900.c  -  8-bit driver module for Noritake GU256X64x-3900
 *                     VFD displays. The VFD is either operating in
 *                     8-bit mode connected to a single PC parallel
 *                     port or in serial mode connected to a single PC
 *                     serial port.
 *
 * based on:
 *   gu256x64-372 driver module for graphlcd
 *     (c) 2004 Andreas 'randy' Weinberger <randy AT smue.org>
 *   gu256x32f driver module for graphlcd
 *     (c) 2003 Andreas Brachold <vdr04 AT deltab.de>
 *   HD61830 device
 *     (c) 2001-2003 by Carsten Siebholz <c.siebholz AT t-online.de>
 *   lcdproc 0.4 driver hd44780-ext8bit
 *     (c) 1999, 1995 Benjamin Tse <blt AT comports.com>
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2004      Ralf Mueller (ralf AT bj-ig.de)
 * (c) 2005-2010 Andreas Regel <andreas.regel AT powarman.de>
 * (c) 2011      Wolfgang Astleitner <mrwastl AT users.sourceforge.net> 
 */

#include <errno.h>
#include <fcntl.h>
#include <syslog.h>
#include <unistd.h>
#include <termios.h>
#include <sys/time.h>
#include <cstring>

#include "common.h"
#include "config.h"
#include "gu256x64-3900.h"
#include "port.h"


namespace GLCD
{

static const unsigned char dSETSTART0      = 0x02;    // Set display start address (CMD prefix)
static const unsigned char dSETSTART1      = 0x44;    // Set display start address (CMD)
static const unsigned char dSETSTART2      = 0x00;    // Set display start address
static const unsigned char dSETSTART3      = 0x53;    // Set display start address

static const unsigned char nINITDISPLAY0   = 0x1b;    // Initialize display (CMD prefix)
static const unsigned char nINITDISPLAY1   = 0x40;    // Initialize display (CMD)

static const unsigned char nPOWER0         = 0x1f;    // Power on/off
static const unsigned char nPOWER1         = 0x28;    // Power on/off
static const unsigned char nPOWER2         = 0x61;    // Power on/off
static const unsigned char nPOWER3         = 0x40;    // Power on/off

static const unsigned char nSETBRIGHT0     = 0x1f;    // Set brightness (CMD prefix)
static const unsigned char nSETBRIGHT1     = 0x58;    // Set brightness (CMD)

static const unsigned char dSETBRIGHT0     = 0x02;    // Set brightness (CMD prefix)
static const unsigned char dSETBRIGHT1     = 0x44;    // Set brightness (CMD)
static const unsigned char dSETBRIGHT2     = 0x00;    // Set brightness
static const unsigned char dSETBRIGHT3     = 0x58;    // Set brightness

static const unsigned char BRIGHT_000      = 0x10;    // Brightness 0%
static const unsigned char BRIGHT_012      = 0x11;    // Brightness 12.5%
static const unsigned char BRIGHT_025      = 0x12;    // Brightness 25%
static const unsigned char BRIGHT_037      = 0x13;    // Brightness 37.5%
static const unsigned char BRIGHT_050      = 0x14;    // Brightness 50%
static const unsigned char BRIGHT_062      = 0x15;    // Brightness 62.5%
static const unsigned char BRIGHT_075      = 0x16;    // Brightness 75%
static const unsigned char BRIGHT_087      = 0x17;    // Brightness 87.5%
static const unsigned char BRIGHT_100      = 0x18;    // Brightness 100%

static const unsigned char nSETCURSOR0     = 0x1f;    // Set cursor (CMD prefix)
static const unsigned char nSETCURSOR1     = 0x24;    // Set cursor (CMD)

static const unsigned char nDISPLAYCURSOR0 = 0x1f;    // display cursor
static const unsigned char nDISPLAYCURSOR1 = 0x43;    // display cursor

static const unsigned char nBLITIMAGE0     = 0x1f;    // Display image
static const unsigned char nBLITIMAGE1     = 0x28;    // Display image
static const unsigned char nBLITIMAGE2     = 0x66;    // Display image
static const unsigned char nBLITIMAGE3     = 0x11;    // Display image

static const unsigned char dBLITIMAGE0     = 0x02;    // Image write
static const unsigned char dBLITIMAGE1     = 0x44;    // Image write
static const unsigned char dBLITIMAGE2     = 0x00;    // Image write
static const unsigned char dBLITIMAGE3     = 0x46;    // Image write


static const unsigned char WRHI            = 0x0f;    // any control pin to high
static const unsigned char WRLO            = 0x00;
static const unsigned char RDYHI           = 0x40;    // RDY
static const unsigned char RDYHIALT        = 0x80;    // RDY satyr wiring
static const unsigned char RDYLO           = 0x00;

static const std::string kWiringStandard = "Standard";
static const std::string kWiringSatyr    = "Satyr";

static const int kInterfaceParallel = 0; // parallel mode
static const int kInterfaceSerial   = 1; // serial mode


cDriverGU256X64_3900::cDriverGU256X64_3900(cDriverConfig * config)
:   cDriver(config)
{
    portFd = -1;
    m_nRefreshCounter = 0;
}

int cDriverGU256X64_3900::Init()
{
    int x;

    width = config->width;
    if (width <= 0)
        width = 256;
    height = config->height;
    if (height <= 0)
        height = 64;
    m_iSizeYb = ((height + 7) / 8);

    // default values
    readyMask = RDYHI;
    readyHi = RDYHI;
    interface = kInterfaceParallel;
    useDMA = true;

    for (unsigned int i = 0; i < config->options.size(); i++)
    {
        if (config->options[i].name == "Wiring")
        {
            if (config->options[i].value == kWiringStandard)
            {
                readyMask = RDYHI;
                readyHi = RDYHI;
            }
            else if (config->options[i].value == kWiringSatyr)
            {
                readyMask = RDYHIALT;
                readyHi = RDYLO;
            }
            else
                syslog(LOG_ERR, "%s error: wiring %s not supported, using default (Standard)!\n",
                       config->name.c_str(), config->options[i].value.c_str());
        }
        if (config->options[i].name == "Interface")
        {
            if (config->options[i].value == "Parallel")
                interface = kInterfaceParallel;
            else if (config->options[i].value == "Serial")
                interface = kInterfaceSerial;
            else
                syslog(LOG_ERR, "%s error: interface %s not supported, using default (Parallel)!\n",
                       config->name.c_str(), config->options[i].value.c_str());
        }
        else if (config->options[i].name == "DMA")
        {
            if (config->options[i].value == "yes")
                useDMA = true;
            else if (config->options[i].value == "no")
                useDMA = false;
            else
                syslog(LOG_ERR, "%s error: unknown DMA setting %s, using default (%s)!\n",
                       config->name.c_str(), config->options[i].value.c_str(), useDMA ? "yes" : "no");
        }
    }

    if (interface == kInterfaceParallel)
        port = new cParallelPort();
    else
        port = NULL;

    // setup linear lcd array
    m_pDrawMem = new unsigned char *[width];
    if (m_pDrawMem)
    {
        for (x = 0; x < width; x++)
        {
            m_pDrawMem[x] = new unsigned char[m_iSizeYb];
            memset(m_pDrawMem[x], 0, m_iSizeYb);
        }
    }
    Clear();

    // setup the lcd array for the "vertical" mem
    m_pVFDMem = new unsigned char *[width];
    if (m_pVFDMem)
    {
        for (x = 0; x < width; x++)
        {
            m_pVFDMem[x] = new unsigned char[m_iSizeYb];
            memset(m_pVFDMem[x], 0, m_iSizeYb);
        }
    }
    ClearVFDMem();

    if (interface == kInterfaceSerial)
    {
        if (InitSerialPort() < 0)
            return -1;
    }
    else
    {
        if (InitParallelPort() < 0)
            return -1;
    }

    if (useDMA)
        InitDMADisplay();
    else
        InitNormalDisplay();

    if (interface == kInterfaceParallel)
    {
        // claim is in InitParallelPort
        port->Release();
    }

    *oldConfig = *config;

    // Set Display SetBrightness
    SetBrightness(config->brightness);
    // clear display
    Clear();
    ClearVFDMem();

    syslog(LOG_INFO, "%s: gu256x64-3900 initialized.\n", config->name.c_str());
    return 0;
}

int cDriverGU256X64_3900::DeInit()
{
    int x;

    if (m_pVFDMem)
    {
        for (x = 0; x < width; x++)
        {
            delete[] m_pVFDMem[x];
        }
        delete[] m_pVFDMem;
    }
    if (m_pDrawMem)
    {
        for (x = 0; x < width; x++)
        {
            delete[] m_pDrawMem[x];
        }
        delete[] m_pDrawMem;
    }

    if (interface == kInterfaceSerial)
    {
        if (portFd >= 0)
        {
            close(portFd);
            portFd =- 1;
        }
    }
    if (port)
    {
        if (port->Close() != 0)
        {
            return -1;
        }
        delete port;
        port = NULL;
    }
    return 0;
}

int cDriverGU256X64_3900::CheckSetup()
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

    if (config->brightness != oldConfig->brightness)
    {
        oldConfig->brightness = config->brightness;
        SetBrightness(config->brightness);
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

int cDriverGU256X64_3900::InitSerialPort()
{
    if (config->device == "")
    {
        syslog(LOG_ERR, "%s: unable to initialize gu256x64-3900!\n", config->name.c_str());
        return -1;
    }

    portFd = open(config->device.c_str(), O_RDWR | O_NOCTTY);
    if (portFd >= 0)
    {
        struct termios options;
        tcgetattr(portFd, &options);
        cfsetispeed(&options, B38400);
        cfsetospeed(&options, B38400);
        options.c_cflag &= ~CSIZE;
        options.c_cflag &= ~PARENB;
        options.c_cflag &= ~CSTOPB;
        options.c_cflag |= CS8;
        tcsetattr(portFd, TCSANOW, &options);
    }
    else
    {
        syslog(LOG_ERR, "%s: unable to initialize gu256x64-3900!\n", config->name.c_str());
        return -1;
    }
    return 0;
}

int cDriverGU256X64_3900::InitParallelPort()
{
    struct timeval tv1, tv2;

    if (config->device == "")
    {
        // use DirectIO
        if (port->Open(config->port) != 0)
        {
            syslog(LOG_ERR, "%s: unable to initialize gu256x64-3900!\n", config->name.c_str());
            return -1;
        }
        uSleep(10);
    }
    else
    {
        // use ppdev
        if (port->Open(config->device.c_str()) != 0)
        {
            syslog(LOG_ERR, "%s: unable to initialize gu256x64-3900!\n", config->name.c_str());
            return -1;
        }
    }

    if (nSleepInit() != 0)
    {
        syslog(LOG_ERR, "%s: INFO: cannot change wait parameters  Err: %s (cDriver::Init)\n", config->name.c_str(), strerror(errno));
        m_bSleepIsInit = false;
    }
    else
    {
        m_bSleepIsInit = true;
    }

    port->Claim();

    syslog(LOG_DEBUG, "%s: benchmark started.\n", config->name.c_str());
    gettimeofday(&tv1, 0);
    for (int x = 0; x < 1000; x++)
    {
        port->WriteData(x % 0x100);
    }
    gettimeofday(&tv2, 0);
    nSleepDeInit();
    m_nTimingAdjustCmd = (tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec);
    syslog(LOG_INFO, "%s: benchmark stopped. Time for Port Command: %ldns\n", config->name.c_str(), m_nTimingAdjustCmd);

    return 0;
}

void cDriverGU256X64_3900::InitNormalDisplay()
{
    Write(nPOWER0);
    Write(nPOWER1);
    Write(nPOWER2);
    Write(nPOWER3);
    Write(1); // power on

    Write(nINITDISPLAY0);
    Write(nINITDISPLAY1);

    Write(nDISPLAYCURSOR0);
    Write(nDISPLAYCURSOR1);
    Write(0); // off

    Write(nSETCURSOR0);
    Write(nSETCURSOR1);
    Write(0); // low byte x
    Write(0); // high byte x
    Write(0); // low byte y
    Write(0); // high byte y
}

void cDriverGU256X64_3900::InitDMADisplay()
{
    Write(dSETSTART0);
    Write(dSETSTART1);
    Write(dSETSTART2);
    Write(dSETSTART3);
    Write(0);
    Write(0);
}

void cDriverGU256X64_3900::ClearVFDMem()
{
    for (int x = 0; x < width; x++)
        memset(m_pVFDMem[x], 0, m_iSizeYb);
}

void cDriverGU256X64_3900::Clear()
{
    for (int x = 0; x < width; x++)
        memset(m_pDrawMem[x], 0, m_iSizeYb);
}

void cDriverGU256X64_3900::SetBrightness(unsigned int percent)
{
    if (interface == kInterfaceParallel)
        port->Claim();

    if (interface == kInterfaceParallel && useDMA)
    {
        Write(dSETBRIGHT0);
        Write(dSETBRIGHT1);
        Write(dSETBRIGHT2);
        Write(dSETBRIGHT3);
    }
    else
    {
        Write(nSETBRIGHT0);
        Write(nSETBRIGHT1);
    }
    if (percent > 87) {
        Write(BRIGHT_100);
    } else if (percent > 75) {
        Write(BRIGHT_087);
    } else if (percent > 62) {
        Write(BRIGHT_075);
    } else if (percent > 50) {
        Write(BRIGHT_062);
    } else if (percent > 37) {
        Write(BRIGHT_050);
    } else if (percent > 25) {
        Write(BRIGHT_037);
    } else if (percent > 12) {
        Write(BRIGHT_025);
    } else if (percent > 1) {
        Write(BRIGHT_012);
    } else {
        Write(BRIGHT_000);
    }
    if (interface == kInterfaceParallel)
        port->Release();
}

void cDriverGU256X64_3900::WriteParallel(unsigned char data)
{
    if (m_bSleepIsInit)
        nSleepInit();
    if ((port->ReadStatus() & readyMask) != readyHi)
    {
        int i = 0;
        int status = port->ReadStatus();
        for (; ((status&readyMask) != readyHi) && i < 1000; i++)
        {
            // wait until display ack's write but not forever
            status=port->ReadStatus();
        }
    }

    port->WriteControl(WRLO);
    port->WriteData(data);
    nSleep(100 + (100 * config->adjustTiming) - m_nTimingAdjustCmd);
    port->WriteControl(WRHI);
    nSleep(500 + (100 * config->adjustTiming) - m_nTimingAdjustCmd);
}

void cDriverGU256X64_3900::WriteSerial(unsigned char data)
{
    write(portFd, &data, 1);
}

void cDriverGU256X64_3900::Write(unsigned char data)
{
    if (interface == kInterfaceSerial)
        WriteSerial(data);
    else
        WriteParallel(data);
}

void cDriverGU256X64_3900::SetPixel(int x, int y, uint32_t data)
{
    unsigned char c;

    if (!m_pDrawMem)
        return;

    if (x >= width || x < 0)
        return;
    if (y >= height || y < 0)
        return;

    if (config->upsideDown)
    {
        x = width - 1 - x;
        y = height - 1 - y;
    }

    c = 0x80 >> (y % 8);

    if (data == GRAPHLCD_White)
        m_pDrawMem[x][y/8] |= c;
    else
        m_pDrawMem[x][y/8] &= ( 0xFF ^ c );
}

#if 0
void cDriverGU256X64_3900::Set8Pixels(int x, int y, unsigned char data)
{
    int n;

    // x - pos is'nt maybe align to 8
    x &= 0xFFF8;

    for (n = 0; n < 8; ++n)
    {
        if (data & (0x80 >> n)) // if bit is set
            SetPixel(x + n, y, GRAPHLCD_White);
    }
}
#endif

void cDriverGU256X64_3900::Refresh(bool refreshAll)
{
    int xb, yb;

    if (!m_pVFDMem || !m_pDrawMem)
        return;

    bool doRefresh = false;
    int minX = width;
    int maxX = 0;
    int minYb = m_iSizeYb;
    int maxYb = 0;

    if (CheckSetup() > 0)
        refreshAll = true;

    for (xb = 0; xb < width; ++xb)
    {
        for (yb = 0; yb < m_iSizeYb; ++yb)
        {
            if (m_pVFDMem[xb][yb] != m_pDrawMem[xb][yb])
            {
                m_pVFDMem[xb][yb] = m_pDrawMem[xb][yb];
                minX = std::min(minX, xb);
                maxX = std::max(maxX, xb);
                minYb = std::min(minYb, yb);
                maxYb = std::max(maxYb, yb + 1);
                doRefresh = true;
            }
        }
    }

    if (config->refreshDisplay > 0)
    {
        m_nRefreshCounter = (m_nRefreshCounter + 1) % config->refreshDisplay;
        if (!refreshAll && !m_nRefreshCounter)
            refreshAll = true;
    }

    if (refreshAll || doRefresh)
    {
        if (refreshAll)
        {
            minX = 0;
            maxX = width;
            minYb = 0;
            maxYb = m_iSizeYb;
            // and reset RefreshCounter
            m_nRefreshCounter = 0;
        }

        minX = std::max(minX, 0);
        maxX = std::min(maxX, width - 1);
        minYb = std::max(minYb, 0);
        maxYb = std::min(maxYb, m_iSizeYb);

        if (interface == kInterfaceParallel)
            port->Claim();

        if (interface == kInterfaceParallel && useDMA)
        {
            Write(dBLITIMAGE0);
            Write(dBLITIMAGE1);
            Write(dBLITIMAGE2);
            Write(dBLITIMAGE3);
            Write(0); // low byte address
            Write(0); // high byte address
            Write((m_iSizeYb*width)&0xff); // low byte size
            Write((m_iSizeYb*width)>>8); // high byte size
        }
        else
        {
            Write(nSETCURSOR0);
            Write(nSETCURSOR1);
            Write(0); // low byte x
            Write(0); // high byte x
            Write(0); // low byte y
            Write(0); // high byte y

            Write(nBLITIMAGE0);
            Write(nBLITIMAGE1);
            Write(nBLITIMAGE2);
            Write(nBLITIMAGE3);
            Write(width&0xff); // low byte width
            Write(width>>8); // high byte width
            Write(m_iSizeYb); // low byte height
            Write(0); // high byte height
            Write(1); // end header
        }

        for (xb = 0; xb < width; xb++)
        {
            for (yb = 0; yb < m_iSizeYb; yb++)
            {
                Write((m_pVFDMem[xb][yb]) ^ (config->invert ? 0xff : 0x00));
            }
            // parallel port writing is busy waiting - with realtime priority you
            // can lock the system - so don't be so greedy ;)
            if ((xb % 32) == 31)
            {
                uSleep(1000);
            }
        }

        if (interface == kInterfaceParallel)
            port->Release();
    }
}

} // end of namespace
