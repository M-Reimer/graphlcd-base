/*
 * GraphLCD driver library
 *
 * noritake800.c  -  Noritake 800(A) series VFD graphlcd driver,
 *                   different "Medium 0.6 dot" sizes should work,
 *                   see http://www.noritake-itron.com:
 *                    - GU128X64-800A,
 *                    - GU256X32-800A,
 *                    - GU128X32-800A,
 *                    - GU160X16-800A,
 *                    - GU160X32-800A,
 *                    - GU192X16-800A.
 *
 * based on:
 *   ideas and HW-command related stuff from the open source project
 *   "lcdplugin for Winamp":
 *     (c) 1999 - 2003 Markus Zehnder <lcdplugin AT markuszehnder.ch>
 *   GU256x64-372 driver module for graphlcd
 *     (c) 20040410 Andreas 'Randy' Weinberger <randy AT smue.org>
 *   gu140x32f driver module for graphlcd
 *     (c) 2003 Andreas Brachold <vdr04 AT deltab de>
 *   HD61830 device
 *     (c) 2001-2003 by Carsten Siebholz <c.siebholz AT t-online.de>
 *   lcdproc 0.4 driver hd44780-ext8bit
 *     (c) 1999, 1995 Benjamin Tse <blt AT Comports.com>
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2004-2011 Lucian Muresan <lucianm AT users.sourceforge.net>
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
#include "noritake800.h"
#include "port.h"

namespace GLCD
{

/* LPT Control Port lines */
#define LPT_CTL_HI_DIR      0x20
#define LPT_CTL_HI_IRQEN    0x10
#define LPT_CTL_LO_STROBE   0x01
#define LPT_CTL_LO_LFEED    0x02
#define LPT_CTL_LO_INIT     0x04
#define LPT_CTL_LO_SELECT   0x08

/* Noritake 800(A) VFD control signals bit masks*/
#define VFDSGN_CD   0x01
#define VFDSGN_WR   0x02
#define VFDSGN_RD   0x04
#define VFDSGN_CSS  0x08

//wirings
#define WIRING_LIQUIDMP3 0
static const std::string kWiringLiquidmp3 = "LiquidMp3";
#define WIRING_MZ  1
static const std::string kWiringMZ = "MZ";
// ... other wirings may follow

/* Command set for this display */
#define ANDCNTL         0x03
#define ORCNTL          0x01
#define XORCNTL         0x02
#define Init800A        0x5F /* initialization code sequence 5f */
#define Init800B        0x62
#define Init800C        0x00+n
#define Init800D        0xFF
#define CLEARSCREENS    0x5e /* clear all screens (layers) */
#define LAYER0ON        0x24 /* screen0 both on */
#define LAYER1ON        0x28 /* screen1 both on */
#define LAYERSON        0x2c /* both screens both on */
#define LAYERSOFF       0x20 /* screens both off */
#define ORON            0x40 /* OR screens */
#define ANDON           0x48 /* AND screens */
#define XORON           0x44 /* XOR screens */
#define SETX            0x64 /* set X position */
#define SETY            0x60 /* set Y position */
#define HSHIFT          0x70 /* set horizontal shift */
#define VSHIFT          0xB0
#define AUTOINCOFF      0x80 /* address auto increment off */
#define SETPOSITION     0xff


cDriverNoritake800::cDriverNoritake800(cDriverConfig * config) : cDriver(config)
{
    int x = 0;
    m_bGraphScreen0_On = true;
    m_bGraphScreen1_On = false;
    // default initilaization for the wiring
    m_nWiring = WIRING_LIQUIDMP3;

    m_pport = new cParallelPort();

    m_nTimingAdjustCmd = 0;
    m_nRefreshCounter = 0;

    width = config->width;      // 128
    if (width <= 0)
        width = 128;
    height = config->height;    //  64
    if (height <= 0)
        height = 64;
    m_iSizeYb = (height + 7)/8;   //   8

    //
    // initialize wiring
    //
    for (unsigned int i = 0; i < config->options.size(); i++)
    {
        if (config->options[i].name == "Wiring")
        {
            if (config->options[i].value == kWiringLiquidmp3)
            {
                m_nWiring = WIRING_LIQUIDMP3;
            }
            else if (config->options[i].value == kWiringMZ)
            {
                m_nWiring = WIRING_MZ;
            }
            else
                syslog(LOG_ERR, "%s error: wiring %s not supported, using default wiring(%s)!\n",
                       config->name.c_str(), config->options[i].value.c_str(), kWiringLiquidmp3.c_str());
        }
    }
    // fill the wiring mask cache for all the 16 possibilities
    m_pWiringMaskCache = new unsigned char[16];
    for (unsigned int i = 0; i < 16; i++)
    {
        m_pWiringMaskCache[i] = N800LptWiringMask(i);
    }

    // setup linear lcd array
    m_pDrawMem = new unsigned char*[width];
    if (m_pDrawMem)
    {
        for (x = 0; x < width; x++)
        {
            m_pDrawMem[x] = new unsigned char[m_iSizeYb];
            memset(m_pDrawMem[x], 0, m_iSizeYb);
        }
    }
    Clear();

    // setup the lcd array for the "vertikal" mem
    m_pVFDMem = new unsigned char*[width];
    if (m_pVFDMem)
    {
        for (x = 0; x < width; x++)
        {
            m_pVFDMem[x] = new unsigned char[m_iSizeYb];
            memset(m_pVFDMem[x], 0, m_iSizeYb);
        }
    }
    ClearVFDMem();
}

cDriverNoritake800::~cDriverNoritake800()
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
    delete[] m_pWiringMaskCache;
    delete m_pport;
}

void cDriverNoritake800::Clear()
{
    for (int x = 0; x < width; x++)
    {
        memset(m_pDrawMem[x], 0, m_iSizeYb);
    }
}

void cDriverNoritake800::ClearVFDMem()
{
    for (int x = 0; x < width; x++)
    {
        memset(m_pVFDMem[x], 0, m_iSizeYb);
    }
}

int cDriverNoritake800::DeInit()
{
    if (m_pport->Close() != 0)
        return -1;
    return 0;
}

int cDriverNoritake800::CheckSetup()
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

int cDriverNoritake800::Init()
{
    int x;
    struct timeval tv1, tv2;

    if (config->device == "")
    {
        // use DirectIO
        if (m_pport->Open(config->port) != 0)
            return -1;
        uSleep(10);
    }
    else
    {
        // use ppdev
        if (m_pport->Open(config->device.c_str()) != 0)
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

    // benchmark port access
    m_pport->Claim();
    syslog(LOG_DEBUG, "%s: benchmark started.\n", config->name.c_str());
    gettimeofday(&tv1, 0);
    int nBenchIterations = 10000;
    for (x = 0; x < nBenchIterations; x++)
    {
        m_pport->WriteData(x % 0x100);
    }
    gettimeofday(&tv2, 0);
    nSleepDeInit();
    // calculate port command duration in nanoseconds
    m_nTimingAdjustCmd = long(double((tv2.tv_sec - tv1.tv_sec) * 1000000000 + (tv2.tv_usec - tv1.tv_usec) * 1000) / double(nBenchIterations));
    syslog(LOG_DEBUG, "%s: benchmark stopped. Time for Port Command: %ldns\n", config->name.c_str(), m_nTimingAdjustCmd);
    m_pport->Release();

    // initialize display
    N800Cmd(Init800A);

    int n;
    for (n=0; n < 15; n++)
    {
        N800Cmd(0x62);
        nSleep(100 + (100 * config->adjustTiming) - m_nTimingAdjustCmd);
        N800Cmd(n);
        nSleep(100 + (100 * config->adjustTiming) - m_nTimingAdjustCmd);
        N800Data(0xff);
        nSleep(100 + (100 * config->adjustTiming) - m_nTimingAdjustCmd);
    }
    nSleep(100 + (100 * config->adjustTiming) - m_nTimingAdjustCmd);


    N800Cmd(LAYERSOFF | LAYER0ON); // layer 0 of the graphic RAM on
    N800Cmd(ORON);     // OR the layers
    N800Cmd(HSHIFT);    // set horizontal shift
    N800Cmd(0x00);     // no shift
    N800Cmd(VSHIFT);    // Vertical shift =0
    N800Cmd(AUTOINCOFF);   // auto increment off
    N800Cmd(SETX);     // set x coord
    N800Cmd(0x40);     // to 0
    N800Cmd(SETY);     // set y coord
    N800Cmd(0);      // to 0

    m_pport->Release();

    //*oldConfig = *config;

    // Set Display SetBrightness
    SetBrightness(config->brightness);
    // clear display
    ClearVFDMem();
    Refresh(true);

    syslog(LOG_INFO, "%s: initialization done.\n", config->name.c_str());
    return 0;
}

void cDriverNoritake800::Refresh(bool refreshAll)
{
    int xb, yb;

    if (CheckSetup() > 0)
        refreshAll = true;

    if (!m_pVFDMem || !m_pDrawMem)
        return;

    if (config->refreshDisplay > 0)
    {
        m_nRefreshCounter = (m_nRefreshCounter + 1) % config->refreshDisplay;
        if (m_nRefreshCounter == 0)
            refreshAll = true;
    }

    m_pport->Claim();
    for (xb = 0; xb < width; ++xb)
    {
        for (yb = 0; yb < m_iSizeYb; ++yb)
        {
            // if differenet or explicitly refresh all
            if (    m_pVFDMem[xb][yb] != m_pDrawMem[xb][yb] ||
                    refreshAll )
            {
                m_pVFDMem[xb][yb] = m_pDrawMem[xb][yb];
                // reset RefreshCounter if doing a full refresh
                if (refreshAll)
                    m_nRefreshCounter = 0;
                // actually write to display
                N800WriteByte(
                    (m_pVFDMem[xb][yb]) ^ ((config->invert != 0) ? 0xff : 0x00),
                    xb,
                    yb,
                    0);
            }
        }
    }
    m_pport->Release();
}

void cDriverNoritake800::N800Cmd(unsigned char data)
{
    if (m_bSleepIsInit)
        nSleepInit();

    // set direction to "port_output" & C/D to C
    m_pport->WriteControl(m_pWiringMaskCache[0x00]);
    // write to data port
    m_pport->WriteData(data);
    nSleep(100 + (100 * config->adjustTiming) - m_nTimingAdjustCmd);
    // set /WR on the control port
    m_pport->WriteControl(m_pWiringMaskCache[VFDSGN_WR]);
    nSleep(100 + (100 * config->adjustTiming) - m_nTimingAdjustCmd);
    // reset /WR on the control port
    m_pport->WriteControl(m_pWiringMaskCache[0x00]);
    nSleep(100 + (100 * config->adjustTiming) - m_nTimingAdjustCmd);
    // set direction to "port_input"
    m_pport->WriteControl(LPT_CTL_HI_DIR | m_pWiringMaskCache[0x00]);
}

void cDriverNoritake800::N800Data(unsigned char data)
{
    if (m_bSleepIsInit)
        nSleepInit();

    // set direction to "port_output" & C/D to C
    m_pport->WriteControl(m_pWiringMaskCache[VFDSGN_CD]);
    // write to data port
    m_pport->WriteData(data);
    nSleep(100 + (100 * config->adjustTiming) - m_nTimingAdjustCmd);
    // set /WR on the control port
    m_pport->WriteControl(m_pWiringMaskCache[VFDSGN_CD | VFDSGN_WR]);
    nSleep(100 + (100 * config->adjustTiming) - m_nTimingAdjustCmd);
    // reset /WR on the control port
    m_pport->WriteControl(m_pWiringMaskCache[VFDSGN_CD]);
    nSleep(100 + (100 * config->adjustTiming) - m_nTimingAdjustCmd);
    // set direction to "port_input"
    m_pport->WriteControl(LPT_CTL_HI_DIR | m_pWiringMaskCache[0x00]);
}

void cDriverNoritake800::SetPixel(int x, int y, uint32_t data)
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
        m_pDrawMem[x][y/8] &= ( 0xFF ^ c);
}

#if 0
void cDriverNoritake800::Set8Pixels(int x, int y, unsigned char data)
{
    int n;

    // x - pos is'nt mayby align to 8
    x &= 0xFFF8;

    for (n = 0; n < 8; ++n)
    {
        if (data & (0x80 >> n))      // if bit is set
            SetPixel(x + n, y);
    }
}
#endif

void cDriverNoritake800::SetBrightness(unsigned int percent)
{
    // display can do 16 brightness levels,
    //  0 = light
    // 15 = dark

    // convert from "light percentage" into darkness values from 0 to 15
    if (percent > 100)
    {
        percent = 100;
    }
    unsigned int darkness = 16 - (unsigned int)((double)percent * 16.0 / 100.0);

    m_pport->Claim();
    N800Cmd(0x40 + (darkness & 0xf));
    m_pport->Release();
}

unsigned char cDriverNoritake800::N800LptWiringMask(unsigned char ctrl_bits)
{
    unsigned char newstatus = 0x0;

    if (m_nWiring == WIRING_LIQUIDMP3)
    {
        if (ctrl_bits & VFDSGN_CSS)
            newstatus |= LPT_CTL_LO_STROBE;
        else
            newstatus &= ~LPT_CTL_LO_STROBE;

        if (ctrl_bits & VFDSGN_RD)
            newstatus |= LPT_CTL_LO_LFEED;
        else
            newstatus &= ~LPT_CTL_LO_LFEED;

        if (ctrl_bits & VFDSGN_WR)
            newstatus |= LPT_CTL_LO_INIT;
        else
            newstatus &= ~LPT_CTL_LO_INIT;

        if (ctrl_bits & VFDSGN_CD)
            newstatus |= LPT_CTL_LO_SELECT;
        else
            newstatus &= ~LPT_CTL_LO_SELECT;

        // control commands are XOR-ed with 0x5
        // to account for active lows and highs
        newstatus ^= 0x5;
    }
    else if (m_nWiring == WIRING_MZ)
    {
        if (ctrl_bits & VFDSGN_CSS)
            newstatus |= LPT_CTL_LO_INIT;
        else
            newstatus &= ~LPT_CTL_LO_INIT;

        if (ctrl_bits & VFDSGN_RD)
            newstatus |= LPT_CTL_LO_LFEED;
        else
            newstatus &= ~LPT_CTL_LO_LFEED;

        if (ctrl_bits & VFDSGN_WR)
            newstatus |= LPT_CTL_LO_STROBE;
        else
            newstatus &= ~LPT_CTL_LO_STROBE;

        if (ctrl_bits & VFDSGN_CD)
            newstatus |= LPT_CTL_LO_SELECT;
        else
            newstatus &= ~LPT_CTL_LO_SELECT;
    }
    return newstatus;
}

void cDriverNoritake800::N800WriteByte(unsigned char data, int nCol, int nRow, int layer)
{
    /* set cursor to desired address */
    N800Cmd(SETX);           /* set upper cursor address */
    N800Cmd(nCol);

    if (layer==0)
    {
        N800Cmd(SETY);     /* set lower cursor address */
        N800Cmd(nRow);     /*layer0 */
    }
    else if (layer==1)
    {
        N800Cmd(SETY);      /* set lower cursor address */
        N800Cmd(nRow+8);    /* layer 1 */
    }

    N800Data(ReverseBits(data));
}

} // end of namespace

