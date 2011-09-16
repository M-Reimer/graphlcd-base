/*
 * GraphLCD driver library
 *
 * gu140x32f.c  -  8-bit driver module for Noritake GU140x32-F7806 VFD
 *                 displays. The VFD is operating in its 8 bit-mode
 *                 connected to a single PC parallel port.
 *
 * based on:
 *   HD61830 device
 *     (c) 2001-2003 by Carsten Siebholz <c.siebholz AT t-online.de>
 *   lcdproc 0.4 driver hd44780-ext8bit
 *     (c) 1999, 1995 Benjamin Tse <blt AT Comports com>
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2003      Andreas Brachold <vdr04 AT deltab.de>
 * (c) 2005-2010 Andreas Regel <andreas.regel AT powarman.de>
 * (c) 2011      Wolfgang Astleitner <mrwastl AT users.sourceforge.net>
 */

#include <errno.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/time.h>
#include <cstring>

#include "common.h"
#include "config.h"
#include "gu140x32f.h"
#include "port.h"


namespace GLCD
{

// Defines for hd44780 Displays
#define RS_DAT      0x00
#define RS_CMD      0x01

#define CLEAR       0x01

#define HOMECURSOR  0x02

#define ENTRYMODE   0x04
#define E_MOVERIGHT 0x02
#define E_MOVELEFT  0x00
#define EDGESCROLL  0x01
#define NOSCROLL    0x00

#define ONOFFCTRL   0x08
#define DISPON      0x04
#define DISPOFF     0x00
#define CURSORON    0x02
#define CURSOROFF   0x00
#define CURSORBLINK 0x01
#define CURSORNOBLINK 0x00

#define CURSORSHIFT 0x10
#define SCROLLDISP  0x08
#define MOVECURSOR  0x00
#define MOVERIGHT   0x04
#define MOVELEFT    0x00

#define FUNCSET     0x20
#define IF_8BIT     0x10
#define IF_4BIT     0x00

// Control output lines
// Write to baseaddress+2
#define nSTRB       0x01    // pin 1; negative logic
#define nLF         0x02    // pin 14
#define nINIT       0x04    // pin 16; the only positive logic output line
#define nSEL        0x08    // pin 17
#define ENIRQ       0x10    // Enable IRQ via ACK line (don't enable this withouT
                            // setting up interrupt stuff too)
#define ENBI        0x20    // Enable bi-directional port (is nice to play with!
                            // I first didn't know a SPP could do this)

#define OUTMASK     0x0B    // SEL, LF and STRB are hardware inverted
                            // Use this mask only for the control output lines
                            // XOR with this mask ( ^ OUTMASK )

static const std::string kWiringStandard = "Standard";
static const std::string kWiringWindows  = "Windows";

// standard wiring
// #define RS        nSTRB
// #define RW        nLF
// #define EN1       nINIT
// #define BL        nSEL

// windows wiring
// #define RS        nINIT
// #define RW        nLF
// #define EN1       nSTRB
// #define BL        nSEL


cDriverGU140X32F::cDriverGU140X32F(cDriverConfig * config)
:   cDriver(config),
    m_pDrawMem(0),
    m_pVFDMem(0)
{
    port = new cParallelPort();

    m_nRefreshCounter = 0;
}

cDriverGU140X32F::~cDriverGU140X32F()
{
    delete port;
}

int cDriverGU140X32F::Init()
{
    int x;
    struct timeval tv1, tv2;

    // default values
    width = config->width;
    if (width <= 0)
        width = 140;
    height = config->height;
    if (height <= 0)
        height = 32;
    m_iSizeYb = ((height + 7) / 8); // 4
    m_WiringRS = nSTRB;
    m_WiringEN1 = nINIT;

    for (unsigned int i = 0; i < config->options.size(); i++)
    {
        if (config->options[i].name == "Wiring")
        {
            if (config->options[i].value == kWiringStandard)
            {
                m_WiringRS = nSTRB;
                m_WiringEN1 = nINIT;
            }
            else if (config->options[i].value == kWiringWindows)
            {
                m_WiringRS = nINIT;
                m_WiringEN1 = nSTRB;
            }
            else
                syslog(LOG_ERR, "%s error: wiring %s not supported, using default (Standard)!\n",
                       config->name.c_str(), config->options[i].value.c_str());
        }
    }

    // setup the memory array for the drawing array gu140x32f
    m_pDrawMem = new unsigned char[width * m_iSizeYb];
    Clear();

    // setup the memory array for the display array gu140x32f
    m_pVFDMem = new unsigned char[width * m_iSizeYb];
    ClearVFDMem();

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
        syslog(LOG_ERR, "%s: INFO: cannot change wait parameters  Err: %s (cDriver::Init)\n", config->name.c_str(), strerror(errno));
        m_bSleepIsInit = false;
    }
    else
    {
        m_bSleepIsInit = true;
    }

    syslog(LOG_DEBUG, "%s: benchmark started.\n", config->name.c_str());
    gettimeofday(&tv1, 0);
    for (x = 0; x < 10000; x++)
    {
        port->WriteData(x % 0x100);
    }
    gettimeofday(&tv2, 0);
    nSleepDeInit();
    m_nTimingAdjustCmd = ((tv2.tv_sec - tv1.tv_sec) * 10000 + (tv2.tv_usec - tv1.tv_usec)) / 1000;
    syslog(LOG_DEBUG, "%s: benchmark stopped. Time for Port Command: %ldns\n", config->name.c_str(), m_nTimingAdjustCmd);


    // setup the lcd in 8 bit mode
    Write(RS_CMD, FUNCSET | IF_8BIT, 4100);
    Write(RS_CMD, FUNCSET | IF_8BIT, 100);
    Write(RS_CMD, FUNCSET | IF_8BIT, 40);

    Write(RS_CMD, ONOFFCTRL | DISPON | CURSOROFF | CURSORNOBLINK, 40);
    Write(RS_CMD, CLEAR, 1600);
    Write(RS_CMD, HOMECURSOR, 1600);

    port->Release();

    *oldConfig = *config;

    // Set Display SetBrightness
    SetBrightness(config->brightness);
    // clear display
    ClearVFDMem();
    Clear();

    syslog(LOG_INFO, "%s: gu140x32f initialized.\n", config->name.c_str());
    return 0;
}

int cDriverGU140X32F::DeInit()
{
    if (m_pVFDMem)
        delete[] m_pVFDMem;
    if (m_pDrawMem)
        delete[] m_pDrawMem;

    if (port->Close() != 0)
        return -1;
    return 0;
}

int cDriverGU140X32F::CheckSetup()
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

void cDriverGU140X32F::ClearVFDMem()
{
    for (int n = 0; m_pVFDMem && n < (width * m_iSizeYb); n++)
        m_pVFDMem[n] = 0x00;
}

void cDriverGU140X32F::Clear()
{
    for (int n = 0; m_pDrawMem && n < (width * m_iSizeYb); n++)
        m_pDrawMem[n] = 0x00;
}

void cDriverGU140X32F::SetBrightness(unsigned int percent)
{
    port->Claim();

    unsigned char level;
    if (percent > 100)
        percent = 100;
    level = percent / 25;
    if (level < 1)
        level = 1;
    level = (4 - level) & 0x03;
    // Set Brightness
    // 00 - 100%
    // 01 -  75%
    // 02 -  50%
    // 03 -  25%
    Write(RS_CMD, FUNCSET | IF_8BIT, 40);
    Write(RS_DAT, level, 40);

    port->Release();
}

void cDriverGU140X32F::Write(unsigned char nFlags, unsigned char bData, unsigned int nMicroSecBusyTime)
{
    if (m_bSleepIsInit)
        nSleepInit();

    unsigned char enableLines = 0, portControl;

    // Only one controller is supported
    enableLines = m_WiringEN1;

    if (nFlags == RS_CMD)
        portControl = 0;
    else // if (nFlags == RS_DAT)
        portControl = m_WiringRS;

    // portControl |= m_WiringBL;

    port->WriteControl(portControl ^ OUTMASK);                  //Reset controlbits
    port->WriteData(bData);                                     //Set data
    port->WriteControl((enableLines | portControl) ^ OUTMASK);  //Set controlbits

    // How long hold the data active
    if (m_bSleepIsInit && (25 + (100 * config->adjustTiming) - m_nTimingAdjustCmd > 0))
    {
        // Wait 50ns
        nSleep(std::max(25L, 50 + (100 * config->adjustTiming) - m_nTimingAdjustCmd));
    }

    port->WriteControl(portControl ^ OUTMASK);                  //Reset controlbits

    nSleep((nMicroSecBusyTime * 1000) + (100 * config->adjustTiming) - m_nTimingAdjustCmd);

    if (m_bSleepIsInit)
        nSleepDeInit();
}

void cDriverGU140X32F::SetPixel(int x, int y, uint32_t data)
{
    unsigned char c;
    int n;

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

    n = x + ((y / 8) * width);
    c = 0x80 >> (y % 8);

    if (data == GRAPHLCD_White)
        m_pDrawMem[n] |= c;
    else
        m_pDrawMem[n] &= (0xFF ^ c);
}

#if 0
void cDriverGU140X32F::Set8Pixels(int x, int y, unsigned char data)
{
    int n;

    // x - pos is'nt mayby align to 8
    x &= 0xFFF8;

    for (n = 0; n < 8; ++n)
    {
        if (data & (0x80 >> n))      // if bit is set
            SetPixel(x + n, y, GRAPHLCD_White);
    }
}
#endif

void cDriverGU140X32F::Refresh(bool refreshAll)
{
    int n, x, yb;

    if (!m_pVFDMem || !m_pDrawMem)
        return;

    bool doRefresh = false;
    int minX = width;
    int maxX = 0;
    int minYb = m_iSizeYb;
    int maxYb = 0;

    if (CheckSetup() > 0)
        refreshAll = true;

    for (yb = 0; yb < m_iSizeYb; ++yb)
        for (x = 0; x < width; ++x)
        {
            n = x + (yb * width);
            if (m_pVFDMem[n] != m_pDrawMem[n])
            {
                m_pVFDMem[n] = m_pDrawMem[n];
                minX = std::min(minX, x);
                maxX = std::max(maxX, x);
                minYb = std::min(minYb, yb);
                maxYb = std::max(maxYb, yb + 1);
                doRefresh = true;
            }
        }

    m_nRefreshCounter = (m_nRefreshCounter + 1) % config->refreshDisplay;

    if (!refreshAll && !m_nRefreshCounter)
        refreshAll = true;

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

        port->Claim();
        // send lcd data to display, controller
        Write(RS_CMD, 0xF1, 40);
        Write(RS_DAT, minX, 40);
        Write(RS_DAT, (minYb * 8) & 0xFFF8, 40);
        Write(RS_DAT, maxX, 40);
        Write(RS_DAT, (maxYb * 8), 40);

        Write(RS_DAT, 'v', 500);

        for (yb = minYb; yb <= maxYb; ++yb)
            for (x = minX; x <= maxX; ++x)
            {
                n = x + (yb * width);

                if (n >= (width * m_iSizeYb))
                    break;
                Write(RS_DAT, (m_pVFDMem[n]) ^ (config->invert ? 0xff : 0x00), 40);
            }
        port->Release();
    }
}

} // end of namespace
