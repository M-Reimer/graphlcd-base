/*
 * GraphLCD driver library
 *
 * gu256x64-372.c  -  8-bit driver module for Noritake GU256x64-372
 *                    VFD displays. The VFD is operating in its 8-bit
 *                    mode connected to a single PC parallel port.
 *
 * based on:
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
 * (c) 2004-2011 Andreas 'randy' Weinberger (randy AT smue.org)
 * (c) 2011      Wolfgang Astleitner <mrwastl AT users.sourceforge.net>
 */

#include <errno.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/time.h>
#include <cstring>

#include "common.h"
#include "config.h"
#include "gu256x64-372.h"
#include "port.h"


namespace GLCD
{

#define SCREENSOFF      0x00 // both screens are off
#define SCREEN1ON       0x01 // only screen #1 is on (graphic screen)
#define SCREEN2ON       0x02 // only screen #2 is on (graphic/character screen)
#define SCREENSON       0x03 // both screens are on

#define CURS_AUTOINC    0x04 // cursor increments automatically
#define CURS_HOLD       0x05 // cursor holds

#define SCREEN2CHAR     0x06 // screen #2 sets to "character" display
#define SCREEN2GRAPH    0x07 // screen #2 sets to "graphic" display

#define DATA_WRITE      0x08 // data write mode
#define DATA_READ       0x09 // data read mode

#define DISP_LOSTA1     0x0A // lower addr. of display start of screen #1
#define DISP_HISTA1     0x0B // upper addr. of display start of screen #1
#define DISP_LOSTA2     0x0C // lower addr. of display start of screen #2
#define DISP_HISTA2     0x0D // upper addr. of display start of screen #2
#define CURS_LOADDR     0x0E // lower addr. of cursor of screen #1 & #2
#define CURS_HIADDR     0x0F // upper addr. of cursor start of screen #1 & #2

#define DISP_OR         0x10 // or display of screen #1 & #2
#define DISP_EXOR       0x11 // ex-or display of screen #1 & #2
#define DISP_AND        0x12 // and display of screen #1 & #2

#define BRIGHT_1        0x18 // luminance level 1 100.0%
#define BRIGHT_2        0x19 // luminance level 2  87.5%
#define BRIGHT_3        0x1A // luminance level 3  75.0%
#define BRIGHT_4        0x1B // luminance level 4  62.5%

static const std::string kWiringStandard = "Standard";
static const std::string kWiringWindows  = "Windows";

const unsigned char kStandardWRHI = 0x04; // 16 / INIT
const unsigned char kStandardWRLO = 0x00; //
const unsigned char kStandardRDHI = 0x00; // 
const unsigned char kStandardRDLO = 0x00; //
const unsigned char kStandardCDHI = 0x00; // 17 / nSELECT
const unsigned char kStandardCDLO = 0x08; //

const unsigned char kWindowsWRHI = 0x00; // 01 / nSTRB
const unsigned char kWindowsWRLO = 0x01; //
const unsigned char kWindowsRDHI = 0x00; // 14 / nLINEFEED
const unsigned char kWindowsRDLO = 0x02; //
const unsigned char kWindowsCDHI = 0x00; // 17 / nSELECT
const unsigned char kWindowsCDLO = 0x08; //


cDriverGU256X64_372::cDriverGU256X64_372(cDriverConfig * config)
:   cDriver(config)
{
    port = new cParallelPort();

    m_nRefreshCounter = 0;
}

cDriverGU256X64_372::~cDriverGU256X64_372()
{
    delete port;
}

int cDriverGU256X64_372::Init()
{
    int x;
    struct timeval tv1, tv2;

    width = config->width;
    if (width <= 0)
        width = 256;
    height = config->height;
    if (height <= 0)
        height = 64;
    m_iSizeYb = (height + 7) / 8;

    // default values
    WRHI = kStandardWRHI;
    WRLO = kStandardWRLO;
    RDHI = kStandardRDHI;
    RDLO = kStandardRDLO;
    CDHI = kStandardCDHI;
    CDLO = kStandardCDLO;

    for (unsigned int i = 0; i < config->options.size(); i++)
    {
        if (config->options[i].name == "Wiring")
        {
            if (config->options[i].value == kWiringStandard)
            {
                WRHI = kStandardWRHI;
                WRLO = kStandardWRLO;
                RDHI = kStandardRDHI;
                RDLO = kStandardRDLO;
                CDHI = kStandardCDHI;
                CDLO = kStandardCDLO;
                syslog(LOG_DEBUG, "%s: using standard wiring\n", config->name.c_str());
            }
            else if (config->options[i].value == kWiringWindows)
            {
                WRHI = kWindowsWRHI;
                WRLO = kWindowsWRLO;
                RDHI = kWindowsRDHI;
                RDLO = kWindowsRDLO;
                CDHI = kWindowsCDHI;
                CDLO = kWindowsCDLO;
                syslog(LOG_DEBUG, "%s: using windows wiring\n", config->name.c_str());
            }
            else
            {
                syslog(LOG_ERR, "%s error: wiring %s not supported, using default (Standard)!\n",
                       config->name.c_str(), config->options[i].value.c_str());
            }
        }	
    }

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

    GU256X64Cmd(SCREEN1ON);
    GU256X64Cmd(CURS_AUTOINC);
    GU256X64Cmd(SCREEN2CHAR);

    GU256X64Cmd(DISP_LOSTA1); GU256X64Data(0x00);
    GU256X64Cmd(DISP_HISTA1); GU256X64Data(0x00);
    GU256X64Cmd(DISP_LOSTA2); GU256X64Data(0x00);
    GU256X64Cmd(DISP_HISTA2); GU256X64Data(0x10);
    GU256X64Cmd(CURS_LOADDR); GU256X64Data(0x00);
    GU256X64Cmd(CURS_HIADDR); GU256X64Data(0x00);

    GU256X64Cmd(DISP_OR);

    port->Release();

    *oldConfig = *config;

    // Set Display SetBrightness
    SetBrightness(config->brightness);
    // clear display
    Clear();
    ClearVFDMem();

    syslog(LOG_INFO, "%s: gu256x64-372 initialized.\n", config->name.c_str());
    return 0;
}

int cDriverGU256X64_372::DeInit()
{
    int x;

    if (m_pVFDMem)
        for (x = 0; x < width; x++)
        {
            delete[] m_pVFDMem[x];
        }
    delete[] m_pVFDMem;

    if (m_pDrawMem)
        for (x = 0; x < width; x++)
        {
            delete[] m_pDrawMem[x];
        }
    delete[] m_pDrawMem;

    if (port->Close() != 0)
        return -1;
    return 0;
}

int cDriverGU256X64_372::CheckSetup()
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

void cDriverGU256X64_372::ClearVFDMem()
{
    for (int x = 0; x < width; x++)
        memset(m_pVFDMem[x], 0, m_iSizeYb);
}

void cDriverGU256X64_372::Clear()
{
    for (int x = 0; x < width; x++)
        memset(m_pDrawMem[x], 0, m_iSizeYb);
}

void cDriverGU256X64_372::SetBrightness(unsigned int percent)
{
    port->Claim();

    if (percent > 88) {
        GU256X64Cmd(BRIGHT_1);
    } else if (percent > 75) {
        GU256X64Cmd(BRIGHT_2);
    } else if (percent > 66) {
        GU256X64Cmd(BRIGHT_3);
    } else if (percent > 0 ) {
        GU256X64Cmd(BRIGHT_4);
    } else {
        GU256X64Cmd(SCREENSOFF);
    }
    port->Release();
}

void cDriverGU256X64_372::GU256X64Cmd(unsigned char data)
{
    if (m_bSleepIsInit)
        nSleepInit();

    port->WriteControl(CDHI | WRHI | RDLO);
    port->WriteData(data);
    nSleep(100 + (100 * config->adjustTiming) - m_nTimingAdjustCmd);
    port->WriteControl(CDHI | WRLO | RDLO);
    nSleep(100 + (100 * config->adjustTiming) - m_nTimingAdjustCmd);
    port->WriteControl(CDHI | WRHI | RDLO);
    nSleep(100 + (100 * config->adjustTiming) - m_nTimingAdjustCmd);
}

void cDriverGU256X64_372::GU256X64Data(unsigned char data)
{
    if (m_bSleepIsInit)
        nSleepInit();

    port->WriteControl(CDLO | WRHI | RDLO);
    port->WriteData(data);
    nSleep(100 + (100 * config->adjustTiming) - m_nTimingAdjustCmd);
    port->WriteControl(CDLO | WRLO | RDLO);
    nSleep(100 + (100 * config->adjustTiming) - m_nTimingAdjustCmd);
    port->WriteControl(CDLO | WRHI | RDLO);
    nSleep(100 + (100 * config->adjustTiming) - m_nTimingAdjustCmd);
}

void cDriverGU256X64_372::SetPixel(int x, int y, uint32_t data)
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
void cDriverGU256X64_372::Set8Pixels(int x, int y, unsigned char data)
{
    int n;

    // x - pos is'nt mayby align to 8
    x &= 0xFFF8;

    for (n = 0; n < 8; ++n)
    {
        if (data & (0x80 >> n)) // if bit is set
            SetPixel(x + n, y, GRAPHLCD_White);
    }
}
#endif

void cDriverGU256X64_372::Refresh(bool refreshAll)
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

    m_nRefreshCounter = (m_nRefreshCounter + 1) % config->refreshDisplay;
    if (!refreshAll && !m_nRefreshCounter)
        refreshAll = true;

    if (refreshAll || doRefresh)
    {
        if (refreshAll) {
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

        GU256X64Cmd(CURS_LOADDR); GU256X64Data(0x00);
        GU256X64Cmd(CURS_HIADDR); GU256X64Data(0x00);
        GU256X64Cmd(DATA_WRITE);

        for (xb = 0; xb < width; xb++)
        {
            for (yb = 0; yb < m_iSizeYb; yb++)
            {
                GU256X64Data((m_pVFDMem[xb][yb]) ^ (config->invert ? 0xff : 0x00));
            }
        }
        port->Release();
    }
}

} // end of namespace
