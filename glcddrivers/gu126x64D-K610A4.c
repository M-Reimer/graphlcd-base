/*
 * GraphLCD driver library
 *
 * gu126x64D-K610A4.c -  8-bit driver module for Noritake GU126x64D-K610A4 VFD
 *                       displays. The VFD is operating in its 8 bit-mode
 *                       connected to a single PC parallel port.
 *
 * based on: 
 *   gu256x64-372 driver module for graphlcd
 *     (c) 2004 Andreas 'randy' Weinberger (randy AT smue.org)
 *   gu256x64-3900 driver module for graphlcd
 *     (c) 2004 Ralf Mueller (ralf AT bj-ig.de)
 *   gu140x32f driver module for graphlcd
 *     (c) 2003 Andreas Brachold <vdr04 AT deltab.de>
 *   ks0108 driver module for graphlcd
 *     (c) 2004 Andreas 'randy' Weinberger (randy AT smue.org)
 *  
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2007      Alexander Rieger (Alexander.Rieger AT inka.de)
 * (c) 2011      Wolfgang Astleitner <mrwastl AT users.sourceforge.net>
 */

#include <errno.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/time.h>
#include <cstring>
#include <cstdlib>

#include "common.h"
#include "config.h"
#include "gu126x64D-K610A4.h"
#include "port.h"

namespace GLCD
{
//----- commands to the display -----------------------------------------------
static const unsigned char CMD_RUN_MACRO_01  = 0x01; // run macro 1
static const unsigned char CMD_RUN_MACRO_02  = 0x02; // run macro 2
static const unsigned char CMD_RUN_MACRO_03  = 0x03; // run macro 3
static const unsigned char CMD_RUN_MACRO_04  = 0x04; // run macro 4
static const unsigned char CMD_RUN_MACRO_05  = 0x05; // run macro 5
static const unsigned char CMD_RUN_MACRO_06  = 0x06; // run macro 6
static const unsigned char CMD_RUN_MACRO_07  = 0x07; // run macro 7

static const unsigned char CMD_CURSOR_POS    = 0x10; // Set cursor position
static const unsigned char CMD_BOX_SET       = 0x11; // Set area
static const unsigned char CMD_BOX_CLEAR     = 0x12; // Clear area
static const unsigned char CMD_BOX_INVERT    = 0x13; // Invert area
static const unsigned char CMD_RECT_SET      = 0x14; // Set outline
static const unsigned char CMD_RECT_CLEAR    = 0x15; // Clear outline

static const unsigned char CMD_PIXEL_SET     = 0x16; // Set pixel at current pos
static const unsigned char CMD_PIXEL_CLEAR   = 0x17; // Clear pixel at current pos

static const unsigned char CMD_GRAPHIC_WRITE = 0x18; // Write graphics data (args: len, data)
static const unsigned char CMD_RESET         = 0x19; // Reset display
static const unsigned char CMD_WRITE_MODE    = 0x1A; // Write mode 
static const unsigned char CMD_INTRO         = 0x1B; // Intro for other commands (see CMA_*)
static const unsigned char CMD_FONT_PROP_SML = 0x1C; // Select font: proportional mini
static const unsigned char CMD_FONT_FIX_MED  = 0x1D; // Select font: fixed spaced 5x7
static const unsigned char CMD_FONT_FIX_BIG  = 0x1E; // Select font: fixed spaced 10x14

static const unsigned char CMA_MACROS_ERASE  = 0x4D; // Erase Macros  (usage: CMD_INTRO + this)
static const unsigned char CMA_EPROM_LOCK    = 0x4C; // Lock EEPROM   (usage: CMD_INTRO + this)
static const unsigned char CMA_EPROM_UNLOCK  = 0x55; // Unlock EEPROM (usage: CMD_INTRO + this)

static const unsigned char CMA_POWER_OFF     = 0x46; // Power off     (usage: CMD_INTRO + this)
static const unsigned char CMA_POWER_ON      = 0x50; // Power on      (usage: CMD_INTRO + this)

//----- signal lines ----------------------------------------------------------
static const unsigned char OUT_EN_HI         = kAutoLow ;
static const unsigned char OUT_EN_LO         = kAutoHigh;
static const unsigned char OUT_EN_MASK       = OUT_EN_HI;

static const unsigned char IN_MB_HI          = 0x40;
static const unsigned char IN_MB_LO          = 0x00;
static const unsigned char IN_MB_MASK        = IN_MB_HI;

//----- log flags -------------------------------------------------------------
static const unsigned int  LL_REFRESH_START  = 0x0001;  //  1
static const unsigned int  LL_REFRESH_END    = 0x0002;  //  2
static const unsigned int  LL_REFRESH_MED    = 0x0004;  //  4
static const unsigned int  LL_VFD_CMD        = 0x0008;  //  8
static const unsigned int  LL_MAX_WAIT       = 0x0010;  // 16

//----- mixed consts ----------------------------------------------------------
static const long          ADJUST_FACTOR     =  100;    // used to adjust timing

//-----------------------------------------------------------------------------
cDriverGU126X64D_K610A4::cDriverGU126X64D_K610A4(cDriverConfig * config)
                       : cDriver              (config)
                       , port                (0)
                       , myNumRows           (0)
                       , myDrawMem           (0)
                       , myVFDMem            (0)
                       , myUseSleepInit      (false)
                       , myPortDelayNS       (0)
                       , myDelay125NS        (0)
                       , myRefreshCounter    (0)
                       , myClaimCounter      (0)
                       , myDataPendingCounter(0)
                       , myLogFlags          (0)
{
} // cDriverGU126X64D_K610A4::cDriverGU126X64D_K610A4()


//-----------------------------------------------------------------------------
int cDriverGU126X64D_K610A4::Init()
{
    width = config->width;
    if (width <= 0 || width > 256)   // don't allow unreasonable big sizes from config
    {
        width = 126;
    } // if

    height = config->height;
    if (height <= 0 || height > 256) // don't allow unreasonable big sizes from config
    {
        height = 64;
    } // if

    //----- parse config -----
    for (unsigned int i = 0; i < config->options.size(); i++)
    {
        if (config->options[i].name == "Debug")
        {
            myLogFlags = atoi(config->options[i].value.c_str());
        } // if
    } // for

    myNumRows = ((height + 7) / 8);
    port      = new cParallelPort();

    // setup drawing memory
    myDrawMem = new unsigned char *[width];
    for (int x = 0; x < width; x++)
    {
        myDrawMem[x] = new unsigned char[myNumRows];
        memset(myDrawMem[x], 0, myNumRows);
    } // for

    // setup vfd memory
    myVFDMem = new unsigned char *[width];
    for (int x = 0; x < width; x++)
    {
        myVFDMem[x] = new unsigned char[myNumRows];
        memset(myVFDMem[x], 0, myNumRows);
    } // for

    if (initParallelPort() < 0)
    {
        return -1;
    } // if

    initDisplay();

    *oldConfig = *config;

    // Set Display SetBrightness
    SetBrightness(config->brightness);

    // clear display
    Clear();
    clearVFDMem();

    syslog( LOG_INFO, "%s: initialized (width: %d  height: %d)"
          , config->name.c_str(), width, height
          );

    return 0;
} // cDriverGU126X64D_K610A4::Init()

//-----------------------------------------------------------------------------
int cDriverGU126X64D_K610A4::DeInit()
{
    if (myVFDMem)
    {
        for (int x = 0; x < width; x++)
        {
            delete[] myVFDMem[x];
        } // for
        delete[] myVFDMem;
        myVFDMem = 0;
    } // if

    if (myDrawMem)
    {
        for(int x = 0; x < width; x++)
        {
            delete[] myDrawMem[x];
        } // for
        delete[] myDrawMem;
        myDrawMem = 0;
    } // if

    if (port)
    {
        // claim port to avoid msg when closing the port
        port->Claim();
        if (port->Close() != 0)
        {
            return -1;
        } // if
        delete port;
        port = 0;
    } // if

    return 0;
} // cDriverGU126X64D_K610A4::DeInit()

//-----------------------------------------------------------------------------
int cDriverGU126X64D_K610A4::checkSetup()
{
    if ( config->device != oldConfig->device
      || config->port   != oldConfig->port
      || config->width  != oldConfig->width
      || config->height != oldConfig->height
       )
    {
        DeInit();
        Init();
        return 0;
    } // if

    if (config->brightness != oldConfig->brightness)
    {
        oldConfig->brightness = config->brightness;
        SetBrightness(config->brightness);
    } // if

    if ( config->upsideDown != oldConfig->upsideDown 
      || config->invert     != oldConfig->invert
       )
    {
        oldConfig->upsideDown = config->upsideDown;
        oldConfig->invert     = config->invert;

        return 1;
    } // if

    return 0;
} // cDriverGU126X64D_K610A4::checkSetup()

//-----------------------------------------------------------------------------
int cDriverGU126X64D_K610A4::initParallelPort()
{
    struct timeval tv1, tv2;

    if (config->device == "")
    {
        // use DirectIO
        if (port->Open(config->port) != 0)
        {
            syslog(LOG_ERR, "%s: unable to initialize gu256x64-3900!", config->name.c_str());
            return -1;
        } // if
        syslog(LOG_INFO, "%s: using direct IO!", config->name.c_str());
        uSleep(10);
    }
    else
    {
        // use ppdev
        if (port->Open(config->device.c_str()) != 0)
        {
            syslog(LOG_ERR, "%s: unable to initialize gu256x64-3900!", config->name.c_str());
            return -1;
        } // if
        syslog(LOG_INFO, "%s: using ppdev!", config->name.c_str());
    } // if

    if (nSleepInit() != 0)
    {
        syslog(LOG_ERR, "%s: INFO: cannot change wait parameters  Err: %s (cDriver::Init)", config->name.c_str(), strerror(errno));
        myUseSleepInit = false;
    }
    else
    {
        myUseSleepInit = true;
    } // if

    //----- measure the time to write to the port -----
    syslog(LOG_DEBUG, "%s: benchmark started.", config->name.c_str());
    gettimeofday(&tv1, 0);

    const int aBenchCount = 1000; // don't change this!
    for (int x = 0; x < aBenchCount; x++)
    {
        port->WriteData(x % 0x100);
    } // for

    gettimeofday(&tv2, 0);

    // release the port, which was implicitely claimed by open
    port->Release();

    if (myUseSleepInit) nSleepDeInit();

    myPortDelayNS  = (tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec);

    myDelay125NS = std::max(125 + (ADJUST_FACTOR * config->adjustTiming) - myPortDelayNS, 0L);

    syslog( LOG_DEBUG, "%s: benchmark stopped. Time for Port Command: %ldns, delay: %ldns"
          , config->name.c_str(), myPortDelayNS, myDelay125NS
          );

    return 0;
} // cDriverGU126X64D_K610A4::initParallelPort()

//-----------------------------------------------------------------------------
void cDriverGU126X64D_K610A4::initDisplay()
{
    claimPort();
    cmdReset();
    releasePort();
} // cDriverGU126X64D_K610A4::initDisplay()

//-----------------------------------------------------------------------------
void cDriverGU126X64D_K610A4::clearVFDMem()
{
    for (int x = 0; x < width; x++)
    {
        memset(myVFDMem[x], 0, myNumRows);
    } // for
} // cDriverGU126X64D_K610A4::clearVFDMem()

//-----------------------------------------------------------------------------
void cDriverGU126X64D_K610A4::Clear()
{
    for (int x = 0; x < width; x++)
    {
        memset(myDrawMem[x], 0, myNumRows);
    } // for
} // cDriverGU126X64D_K610A4::Clear()

//-----------------------------------------------------------------------------
void cDriverGU126X64D_K610A4::SetBrightness(unsigned int percent)
{
    claimPort();
    cmdSetBrightness(percent);
    releasePort();
} // cDriverGU126X64D_K610A4::SetBrightness()

//-----------------------------------------------------------------------------
bool cDriverGU126X64D_K610A4::waitForStatus(unsigned char theMask, unsigned char theValue, int theMaxWait)
{
    theValue = theValue & theMask;

    int status = port->ReadStatus();

    if ((status & theMask) != theValue)
    {
        // wait some time for MB go HI/LO but not forever
        int i = 0;
        for(i = 0; ((status & theMask) != theValue) && i < theMaxWait; i++)
        {
            status = port->ReadStatus();
        } // for

        if (isLogEnabled(LL_MAX_WAIT) && i >= theMaxWait)
        {
            syslog( LOG_INFO, "%s: slept for %5d times while waiting for MB = %d"
                  , config->name.c_str(), i, ((theMask & theValue) == 0 ? 0 : 1)
                  );
        } // 
    } // if

    return ((status & theMask) == theValue);
} // cDriverGU126X64D_K610A4::waitForStatus()

//-----------------------------------------------------------------------------
void cDriverGU126X64D_K610A4::writeParallel(unsigned char data)
{
    if (myUseSleepInit) nSleepInit();

    waitForStatus(IN_MB_MASK, IN_MB_LO, 500);                  // wait for MB == LO

    port->WriteData(data);                                     // write data
    nSleep(myDelay125NS);                                      // - sleep

    port->WriteControl(OUT_EN_LO & OUT_EN_MASK);               // set ENABLE to LO
    nSleep(myDelay125NS);                                      // - sleep

    port->WriteControl(OUT_EN_HI & OUT_EN_MASK);               // set ENABLE to HI

    waitForStatus(IN_MB_MASK, IN_MB_HI, 50);                   // wait for MB == HI

//  the other drivers don't do this neither
//  if (myUseSleepInit) nSleepDeInit();
} // cDriverGU126X64D_K610A4::writeParallel()

//-----------------------------------------------------------------------------
int cDriverGU126X64D_K610A4::write(unsigned char data)
{
    int b = 0;

    writeParallel(data);
    ++b; 

    // if data == 0x60 -> send 0x60 twice 
    // (0x60 switches to hex-mode)
    if (data == 0x60)
    {
        writeParallel(data);
        ++b;
    } // if

    return b;
} // cDriverGU126X64D_K610A4::write()

//-----------------------------------------------------------------------------
void cDriverGU126X64D_K610A4::SetPixel(int x, int y, uint32_t data)
{
    if (!myDrawMem          ) return;
    if (x >= width  || x < 0) return;
    if (y >= height || y < 0) return;

    if (config->upsideDown)
    {
        x = width  - 1 - x;
        y = height - 1 - y;
    } // if

    unsigned char c = 0x80 >> (y % 8);

    if (data == GRAPHLCD_White)
        myDrawMem[x][y/8] |= c;
    else
        myDrawMem[x][y/8] &= ( 0xFF ^ c );
} // cDriverGU126X64D_K610A4::SetPixel()

#if 0
//-----------------------------------------------------------------------------
void cDriverGU126X64D_K610A4::Set8Pixels(int x, int y, unsigned char data)
{
    // x - pos isn't maybe align to 8
    x &= 0xFFF8;

    for (int n = 0; n < 8; ++n)
    {
        if ((data & (0x80 >> n)) != 0) // if bit is set
        {
            setPixel(x + n, y, GRAPHLCD_White);
        } // if
    } // for
} // cDriverGU126X64D_K610A4::Set8Pixels()
#endif

//-----------------------------------------------------------------------------
void cDriverGU126X64D_K610A4::Refresh(bool refreshAll)
{
    // no mem present -> return
    if (!myVFDMem || !myDrawMem)
    {
        return;
    } // if

    // create log
    if (isLogEnabled(LL_REFRESH_START))
    {
        syslog( LOG_INFO, "%s: > Refresh()  all = %d  RefreshDisplay = %d  RefreshCtr  = %d  Delay = %ld"
              , config->name.c_str()
              , refreshAll
              , config->refreshDisplay
              , myRefreshCounter
              , myDelay125NS
              );
    } // if

    // setup changed -> refresh all
    if (checkSetup() > 0)
    {
        syslog(LOG_DEBUG, "%s:   Refresh() checkSetup() returned != 0 -> refreshAll = true", config->name.c_str());
        refreshAll = true;
    } // if

    // refresh-counter exceeded -> refresh all
    if (!refreshAll && config->refreshDisplay != 0)
    {
        myRefreshCounter = (myRefreshCounter + 1) % config->refreshDisplay;
        refreshAll = myRefreshCounter == 0;

        if (refreshAll && isLogEnabled(LL_REFRESH_START))
        {
          syslog(LOG_DEBUG, "%s:   Refresh() refresh-count reached -> refreshAll = true", config->name.c_str());
        } // if
    } // if

    if (isLogEnabled(LL_REFRESH_START))
    {
        syslog( LOG_INFO, "%s:   Refresh()  all = %d  RefreshDisplay = %d  RefreshCtr  = %d  Delay = %ld"
              , config->name.c_str()
              , refreshAll
              , config->refreshDisplay
              , myRefreshCounter
              , myDelay125NS
              );
    } // if

    // time for logs
    struct timeval tv1, tv2;
    gettimeofday(&tv1, 0);

    claimPort();

    int  chunk = 128; // displays with more than 128 pixels width are written in chunks
                      // note: this driver isn't really prepared to handle displays
                      //       with other dimensions than 126x64
    int  xb    = 0;
    int  yb    = 0;
    long bc    = 0;

    for (yb = 0; yb < myNumRows; ++yb)
    {
        int  minX  = width;
        int  maxX  = 0;

        //----- if !refreshAll -> check modified bytes
        if (!refreshAll)
        {
            for (xb = 0; xb < width; ++xb)
            {
                if (myVFDMem[xb][yb] != myDrawMem[xb][yb])
                {
                    minX = std::min(minX, xb);
                    maxX = std::max(maxX, xb);
                } // if
            } // for
        }
        else
        {
            minX = 0;
            maxX = width - 1;
        } // if

        // create log
        if (isLogEnabled(LL_REFRESH_MED))
        {
            if (minX <= maxX)
            {
                syslog( LOG_INFO, "%s: Row[%d] %3d - %3d : %3d"
                      , config->name.c_str(), yb
                      , minX, maxX
                      , maxX - minX + 1
                      );
            }
            else
            {
                syslog( LOG_INFO, "%s: Row[%d] --- - --- : ---"
                      , config->name.c_str(), yb
                      );
            } // if
        } // if

        // perform refresh
        if (minX <= maxX)
        {
            bc += cmdSetCursorPos(minX, yb * 8);

            for (xb = minX; xb <= maxX; ++xb)
            {
                if ((xb - minX) % chunk == 0)
                {
                    bc += cmdGraphicWrite(std::min((maxX - xb + 1), chunk));
                } // if

                bc += cmdGraphicData(myDrawMem[xb][yb]);
                myVFDMem[xb][yb] = myDrawMem[xb][yb];
            } // for
        } // if
    } // for

    releasePort();

    // create log
    if (isLogEnabled(LL_REFRESH_END))
    {
        gettimeofday(&tv2, 0);

        long duration_ms = ((tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec)) 
                        / 1000 /* us -> ms */
                        ;

        syslog( LOG_INFO, "%s: < Refresh()  all = %d  took %3ld ms  %5ld bytes = %5ld bytes/sec = %5ld ns/byte"
              , config->name.c_str()
              , refreshAll
              , duration_ms
              , bc
              , duration_ms == 0 ? -1 : bc * 1000 / duration_ms
              , bc          == 0 ? -1 : duration_ms * 1000000 / bc
              );
    } // if
} // cDriverGU126X64D_K610A4::Refresh()

//-----------------------------------------------------------------------------
int cDriverGU126X64D_K610A4::cmdReset()
{
    ensureNotInGraphics();
    int b = 0;

    if (isLogEnabled(LL_VFD_CMD))
    {
        syslog(LOG_INFO, "- 1B: CMD_RESET        : 0x%02X ", int(CMD_RESET));
    } // if

    b += write(CMD_RESET);

    unsigned char aMode = 1 << 7  // data orientation : 0: horizontal, 1: vertical , default: 0
                        | 0 << 6  // cursor movement  : 0: horizontal, 1: vertical , default: 0
                        | 0 << 5  // cursor direction : 0: forwards  , 1: backwards, default: 0
                        | 0 << 4  // underscore cursor: 0: off       , 1: on       , default: 0
                        | 0 << 3  // underscore cursor: 0: static    , 1: flash    , default: 0
                        | 0 << 2  // not in documentation
                        | 0 << 0  // pen type: 0: overwrite, 1: AND, 2: OR, 3: XOR , default: 0
                        ;

    if (isLogEnabled(LL_VFD_CMD))
    {
        syslog(LOG_INFO, "- 2B: CMD_WRITE_MODE   : 0x%02X 0x%02X", int(CMD_RESET), int(aMode));
    } // if

    b += write(CMD_WRITE_MODE);
    b += write(aMode);

    return b;
} // cDriverGU126X64D_K610A4::cmdReset()

//-----------------------------------------------------------------------------
int cDriverGU126X64D_K610A4::cmdPower(bool fOn)
{
    ensureNotInGraphics();
    int b = 0;

    if (isLogEnabled(LL_VFD_CMD))
    {
        syslog( LOG_INFO, "- 2B: CMD_POWER        : 0x%02X 0x%02X"
              , int(CMD_INTRO), int(fOn ? CMA_POWER_ON : CMA_POWER_OFF)
              );
    } // if

    b += write(CMD_INTRO);
    b += write(fOn ? CMA_POWER_ON : CMA_POWER_OFF);

    return b;
} // cDriverGU126X64D_K610A4::cmdPower()

//-----------------------------------------------------------------------------
int cDriverGU126X64D_K610A4::cmdLock(bool fLock)
{
    ensureNotInGraphics();
    int b = 0;

    if (isLogEnabled(LL_VFD_CMD))
    {
        syslog( LOG_INFO, "- 2B: CMD_LOCK         : 0x%02X 0x%02X"
              , int(CMD_INTRO), int(fLock ? CMA_EPROM_LOCK : CMA_EPROM_UNLOCK)
              );
    } // if

    b += write(CMD_INTRO);
    b += write(fLock ? CMA_EPROM_LOCK : CMA_EPROM_UNLOCK);

    return b;
} // cDriverGU126X64D_K610A4::cmdPower()

//-----------------------------------------------------------------------------
int cDriverGU126X64D_K610A4::cmdSetCursorPos(unsigned char x, unsigned char y)
{
    ensureNotInGraphics();
    int b = 0;

    if (isLogEnabled(LL_VFD_CMD))
    {
        syslog( LOG_INFO, "- 3B: CMD_CURSOR_POS   : 0x%02X 0x%02X 0x%02X  (x = %3d, y = %3d)"
              , int(CMD_CURSOR_POS), int(x), int(y), int(x), int(y)
              );
    } // if

    b += write(CMD_CURSOR_POS); // cmd
    b += write(x             ); // xpos
    b += write(y             ); // ypos

    return b;
} // cDriverGU126X64D_K610A4::cmdSetCursorPos();

//-----------------------------------------------------------------------------
int cDriverGU126X64D_K610A4::cmdGraphicWrite(unsigned char count)
{
    ensureNotInGraphics();
    int b = 0;

    if (isLogEnabled(LL_VFD_CMD))
    {
        syslog( LOG_INFO, "- 2B: CMD_GRAPHIC_WRITE: 0x%02X 0x%02X (%d bytes)"
              , int(CMD_GRAPHIC_WRITE), int(count), int(count)
              );
    } // if

    b += write(CMD_GRAPHIC_WRITE); // cmd
    b += write(count            ); // len

    myDataPendingCounter = count;

    return b;
} // cDriverGU126X64D_K610A4::cmdGraphicWrite()

//-----------------------------------------------------------------------------
int cDriverGU126X64D_K610A4::cmdGraphicData(unsigned char data)
{
    int b = 0;

    myDataPendingCounter--;
    if (myDataPendingCounter < 0)
    {
        syslog( LOG_WARNING, "%s error: more graphic data written than announced -> ignored"
              , config->name.c_str()
              );
    }
    else
    {
        if (isLogEnabled(LL_VFD_CMD))
        {
            syslog( LOG_INFO, "- 1B: CMD_GRAPHIC_DATA : 0x%02X  (expecting another %d bytes)"
                  , int(data), myDataPendingCounter
                  );
        } // if

        b += write(data ^ (config->invert ? 0xFF : 0x00));
    } // if

    return b;
} // cDriverGU126X64D_K610A4::cmdGraphicData()

//-----------------------------------------------------------------------------
int cDriverGU126X64D_K610A4::cmdSetBrightness(unsigned int percent)
{
    ensureNotInGraphics();
    int b = 0;

    unsigned char bright = 0;
    if      (percent >= 85) bright = 0xFF;
    else if (percent >= 71) bright = 0xFE;
    else if (percent >= 57) bright = 0xFD;
    else if (percent >= 43) bright = 0xFC;
    else if (percent >= 29) bright = 0xFB;
    else if (percent >= 15) bright = 0xFA;
    else if (percent >=  1) bright = 0xF9;
    else                    bright = 0xF8;

    if (isLogEnabled(LL_VFD_CMD))
    {
        syslog( LOG_INFO, "- 2B: CMD_INTRO        : 0x%02X 0x%02X = set brightness"
              , int(CMD_INTRO), int(bright)
              );
    } // if

    b += write(CMD_INTRO);
    b += write(bright);

    return b;
} // cDriverGU126X64D_K610A4::cmdSetBrightness()

//-----------------------------------------------------------------------------
int cDriverGU126X64D_K610A4::cmdSetFont(FontType theFont)
{
    ensureNotInGraphics();
    int b = 0;

    unsigned char aCmd = 0;
    switch (theFont)
    {
        case FONT_PROP_SML: aCmd = CMD_FONT_PROP_SML; break;
        case FONT_FIX_BIG : aCmd = CMD_FONT_FIX_BIG ; break;
        case FONT_FIX_MED : 
        default           : aCmd = CMD_FONT_FIX_MED ; break;
    } // switch

    if (isLogEnabled(LL_VFD_CMD))
    {
        syslog(LOG_INFO, "- 1B: CMD_SET_FONT     : 0x%02X", int(aCmd));
    } // if

    b += write(aCmd);

    return b;
} // cDriverGU126X64D_K610A4::cmdSetFont()

//-----------------------------------------------------------------------------
int cDriverGU126X64D_K610A4::cmdWriteText(const char *theText)
{
    ensureNotInGraphics();
    int b = 0;

    if (isLogEnabled(LL_VFD_CMD))
    {
        syslog(LOG_INFO, "-%2dB: WRITE_TEXT       : '%s'", (int)strlen(theText), theText);
    } // if

    for (const char *p = theText; *p != '\0'; ++p)
    {
        b += write(*p);
    } // for

    return b;
} // cDriverGU126X64D_K610A4::cmdWriteText()

//-----------------------------------------------------------------------------
int cDriverGU126X64D_K610A4::cmdDrawRect(unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2)
{
    ensureNotInGraphics();
    int b = 0;

    if (isLogEnabled(LL_VFD_CMD))
    {
        syslog( LOG_INFO, "- 5B: CMD_SET_OUTLINE  : 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X  (x1 = %3d, y1 = %3d, x2 = %3d, y2 = %3d)"
              , int(CMD_CURSOR_POS)
              , int(x1), int(y1), int(x2), int(y2)
              , int(x1), int(y1), int(x2), int(y2)
              );
    } // if

    b += write(CMD_RECT_SET  );
    b += write(x1            );
    b += write(y1            );
    b += write(x2            );
    b += write(y2            );

    return b;
} // cDriverGU126X64D_K610A4::cmdDrawRect()

//-----------------------------------------------------------------------------
int cDriverGU126X64D_K610A4::cmdSetMacro(unsigned char theMacroNum, unsigned char theCountBytes)
{
    if (theMacroNum > 7)
    {
        return 0;
    } // if

    ensureNotInGraphics();
    int b = 0;

    if (isLogEnabled(LL_VFD_CMD))
    {
        syslog( LOG_INFO, "- 3B: CMD_INTRO        : 0x%02X 0x%02X 0x%02X (define macro %d with length %d)"
              , int(CMD_INTRO)
              , int(theMacroNum), int(theCountBytes)
              , int(theMacroNum), int(theCountBytes)
              );
    } // if

    b += write(CMD_INTRO     );
    b += write(theMacroNum   );
    b += write(theCountBytes );

    return b;
} // cDriverGU126X64D_K610A4::cmdSetMacro()

//-----------------------------------------------------------------------------
int cDriverGU126X64D_K610A4::cmdSetPixel(bool fSet)
{
    int b = 0;

    if (fSet)
    {
        ensureNotInGraphics();

        if (isLogEnabled(LL_VFD_CMD))
        {
            syslog(LOG_INFO, "- 1B: SET_PIXEL        : 0x%02X", 0x16);
        } // if

        b += write(CMD_PIXEL_SET);
    }
    else
    {
        b = cmdClrPixel();
    } // if

    return b;
} // cDriverGU126X64D_K610A4::cmdSetPixel()

//-----------------------------------------------------------------------------
int cDriverGU126X64D_K610A4::cmdClrPixel()
{
    ensureNotInGraphics();
    int b = 0;

    if (isLogEnabled(LL_VFD_CMD))
    {
        syslog(LOG_INFO, "- 1B: CLR_PIXEL        : 0x%02X", 0x17);
    } // if

    b += write(CMD_PIXEL_CLEAR);

    return b;
} // cDriverGU126X64D_K610A4::cmdClrPixel()

//-----------------------------------------------------------------------------
void cDriverGU126X64D_K610A4::ensureNotInGraphics()
{
    if (myClaimCounter <= 0)
    {
        syslog(LOG_ERR, "%s: ERROR: port not claimed (%d)", config->name.c_str(), myClaimCounter);
    } // if

    if (myDataPendingCounter > 0)
    {
        syslog( LOG_WARNING, "%s error: expected another %d bytes graphic data, filling with 0x00"
              , config->name.c_str(), myDataPendingCounter
              );
    } // if
    while (myDataPendingCounter > 0)
    {
        cmdGraphicData(0);
    } // while
} // cDriverGU126X64D_K610A4::ensureNotInGraphics()


//-----------------------------------------------------------------------------
void cDriverGU126X64D_K610A4::claimPort()
{
    if (myClaimCounter == 0) 
    {
        port->Claim();
    } // if

    myClaimCounter++;

    if (myClaimCounter > 1)
    {
        syslog( LOG_WARNING, "%s: port claimed more than once (%d)"
              , config->name.c_str(), myClaimCounter
              );
    } // if

} // cDriverGU126X64D_K610A4::claimPort()

//-----------------------------------------------------------------------------
void cDriverGU126X64D_K610A4::releasePort()
{
    if (myClaimCounter == 1) 
    {
        port->Release();
    } // if

    myClaimCounter--;

    if (myClaimCounter < 0)
    {
        syslog( LOG_WARNING, "%s: port released more often than claimed"
              , config->name.c_str()
              );
        myClaimCounter = 0;
    } // if

} // cDriverGU126X64D_K610A4::releasePort()

//-----------------------------------------------------------------------------
bool cDriverGU126X64D_K610A4::isLogEnabled(int theLevel) const
{
    return (theLevel & myLogFlags) != 0;
} // cDriverGU126X64D_K610A4::isLogEnabled()

//-----------------------------------------------------------------------------
} // end of namespace

