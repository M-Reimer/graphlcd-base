/*
 * GraphLCD driver library
 *
 * sed1330.c  -  SED1330 driver class
 *
 * based on: hd61830.c
 *  (c) 2001-2004 Carsten Siebholz <c.siebholz AT t-online.de>
 *
 * changes for Seiko-Epson displays: Mar 2004
 *  (c) 2004 Heinz Gressenberger <heinz.gressenberger AT stmk.gv.at>
 *
 * init sequence taken from Thomas Baumann's LCD-Test program
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2003      Roland Praml <praml.roland AT t-online.de>
 * (c) 2005-2010 Andreas Regel <andreas.regel AT powarman.de>
 * (c) 2011      Wolfgang Astleitner <mrwastl AT users.sourceforge.net>
 */

#include <syslog.h>
#include <sys/time.h>
#include <cstring>
#include <cstdlib>

#include "common.h"
#include "config.h"
#include "port.h"
#include "sed1330.h"


namespace GLCD
{

// SED1330 Commands

// Command Codes, Bytes for Param

#define C_SYSTEMSET  0x40
#define C_SLEEPIN    0x53
#define C_DISPON     0x59
#define C_DISPOFF    0x58
#define C_SCROLL     0x44
#define C_CSRFORM    0x5D
#define C_CGRAMADR   0x5C
#define C_CSRDIR_R   0x4C
#define C_CSRDIR_L   0x4D
#define C_CSRDIR_U   0x4E
#define C_CSRDIR_D   0x4F
#define C_HDOTSCR    0x5A
#define C_OVLAY      0x5B
#define C_CSRW       0x46
#define C_CSRR       0x47
#define C_MWRITE     0x42
#define C_MREAD      0x43

#define M0 0
// 0-internal CG ROM, 1-external CG-ROM/RAM

#define M1 0
// 0 - 32 char CG-RAM, 1 - 64 char

#define M2 0
// 0 - 8x8, 1 - 8x16 matrix in CG-RAM/ROM

#define FX 8
// character-with
#define FY 8
// character-height
#define BPC 1
// byte per character, 1 - FX<=8, 2 - FX=9..16

#define SAD1 0x0000
// startadress first screen


const int kInterface6800 = 0;
const int kInterface8080 = 1;

const std::string kWiringOriginal = "Original";
const std::string kWiringPowerLCD = "PowerLCD";
const std::string kWiringLCDProc  = "LCDProc";
const std::string kWiringTweakers = "Tweakers";
const std::string kWiringYASEDW   = "YASEDW";

const unsigned char kOriginalA0HI = kInitHigh;
const unsigned char kOriginalA0LO = kInitLow;
const unsigned char kOriginalRDHI = kStrobeHigh;
const unsigned char kOriginalRDLO = kStrobeLow;
const unsigned char kOriginalWRHI = kAutoHigh;
const unsigned char kOriginalWRLO = kAutoLow;
const unsigned char kOriginalCSHI = kSelectHigh;
const unsigned char kOriginalCSLO = kSelectLow;

const unsigned char kPowerLCDA0HI = kInitHigh;
const unsigned char kPowerLCDA0LO = kInitLow;
const unsigned char kPowerLCDRDHI = kSelectHigh;
const unsigned char kPowerLCDRDLO = kSelectLow;
const unsigned char kPowerLCDWRHI = kStrobeHigh;
const unsigned char kPowerLCDWRLO = kStrobeLow;
const unsigned char kPowerLCDCSHI = kAutoHigh;
const unsigned char kPowerLCDCSLO = kAutoLow;

const unsigned char kLCDProcA0HI = kSelectHigh;
const unsigned char kLCDProcA0LO = kSelectLow;
const unsigned char kLCDProcRDHI = kInitHigh;
const unsigned char kLCDProcRDLO = kInitLow;
const unsigned char kLCDProcWRHI = kAutoHigh;
const unsigned char kLCDProcWRLO = kAutoLow;
const unsigned char kLCDProcCSHI = kStrobeHigh;
const unsigned char kLCDProcCSLO = kStrobeLow;

const unsigned char kTweakersA0HI = kSelectHigh;
const unsigned char kTweakersA0LO = kSelectLow;
const unsigned char kTweakersRDHI = kAutoHigh;
const unsigned char kTweakersRDLO = kAutoLow;
const unsigned char kTweakersWRHI = kInitHigh;
const unsigned char kTweakersWRLO = kInitLow;
const unsigned char kTweakersCSHI = kStrobeHigh;
const unsigned char kTweakersCSLO = kStrobeLow;

const unsigned char kYASEDWA0HI = kAutoHigh;
const unsigned char kYASEDWA0LO = kAutoLow;
const unsigned char kYASEDWRDHI = kInitHigh;
const unsigned char kYASEDWRDLO = kInitLow;
const unsigned char kYASEDWWRHI = kStrobeHigh;
const unsigned char kYASEDWWRLO = kStrobeLow;
const unsigned char kYASEDWCSHI = kSelectHigh;
const unsigned char kYASEDWCSLO = kSelectLow;


cDriverSED1330::cDriverSED1330(cDriverConfig * config)
:   cDriver(config)
{
    port = new cParallelPort();

    refreshCounter = 0;
}

cDriverSED1330::~cDriverSED1330()
{
    delete port;
}

int cDriverSED1330::Init()
{
    int x;
    struct timeval tv1, tv2;

    width = config->width;
    if (width <= 0)
        width = 320;
    height = config->height;
    if (height <= 0)
        height = 240;

    // default values
    oscillatorFrequency = 9600;
    interface = kInterface6800;
    A0HI = kOriginalA0HI;
    A0LO = kOriginalA0LO;
    RDHI = kOriginalRDHI;
    RDLO = kOriginalRDLO;
    WRHI = kOriginalWRHI;
    WRLO = kOriginalWRLO;
    CSHI = kOriginalCSHI;
    CSLO = kOriginalCSLO;
    ENHI = RDHI;
    ENLO = RDLO;
    RWHI = WRHI;
    RWLO = WRLO;

    for (unsigned int i = 0; i < config->options.size(); i++)
    {
        if (config->options[i].name == "Wiring")
        {
            if (config->options[i].value == kWiringOriginal)
            {
                A0HI = kOriginalA0HI;
                A0LO = kOriginalA0LO;
                RDHI = kOriginalRDHI;
                RDLO = kOriginalRDLO;
                WRHI = kOriginalWRHI;
                WRLO = kOriginalWRLO;
                CSHI = kOriginalCSHI;
                CSLO = kOriginalCSLO;
            }
            else if (config->options[i].value == kWiringPowerLCD)
            {
                A0HI = kPowerLCDA0HI;
                A0LO = kPowerLCDA0LO;
                RDHI = kPowerLCDRDHI;
                RDLO = kPowerLCDRDLO;
                WRHI = kPowerLCDWRHI;
                WRLO = kPowerLCDWRLO;
                CSHI = kPowerLCDCSHI;
                CSLO = kPowerLCDCSLO;
            }
            else if (config->options[i].value == kWiringLCDProc)
            {
                A0HI = kLCDProcA0HI;
                A0LO = kLCDProcA0LO;
                RDHI = kLCDProcRDHI;
                RDLO = kLCDProcRDLO;
                WRHI = kLCDProcWRHI;
                WRLO = kLCDProcWRLO;
                CSHI = kLCDProcCSHI;
                CSLO = kLCDProcCSLO;
            }
            else if (config->options[i].value == kWiringTweakers)
            {
                A0HI = kTweakersA0HI;
                A0LO = kTweakersA0LO;
                RDHI = kTweakersRDHI;
                RDLO = kTweakersRDLO;
                WRHI = kTweakersWRHI;
                WRLO = kTweakersWRLO;
                CSHI = kTweakersCSHI;
                CSLO = kTweakersCSLO;
            }
            else if (config->options[i].value == kWiringYASEDW)
            {
                A0HI = kYASEDWA0HI;
                A0LO = kYASEDWA0LO;
                RDHI = kYASEDWRDHI;
                RDLO = kYASEDWRDLO;
                WRHI = kYASEDWWRHI;
                WRLO = kYASEDWWRLO;
                CSHI = kYASEDWCSHI;
                CSLO = kYASEDWCSLO;
            }
            else
            {
                syslog(LOG_ERR, "%s error: wiring %s not supported, using default (Original)!\n",
                       config->name.c_str(), config->options[i].value.c_str());
            }
            ENHI = RDHI;
            ENLO = RDLO;
            RWHI = WRHI;
            RWLO = WRLO;
        }
        else if (config->options[i].name == "OscillatorFrequency")
        {
            int freq = atoi(config->options[i].value.c_str());
            if (freq > 1000 && freq < 15000)
                oscillatorFrequency = freq;
            else
                syslog(LOG_ERR, "%s error: oscillator frequency %d out of range, using default (%d)!\n",
                       config->name.c_str(), freq, oscillatorFrequency);
        }
        if (config->options[i].name == "Interface")
        {
            if (config->options[i].value == "6800")
                interface = kInterface6800;
            else if (config->options[i].value == "8080")
                interface = kInterface8080;
            else
                syslog(LOG_ERR, "%s error: interface %s not supported, using default (6800)!\n",
                       config->name.c_str(), config->options[i].value.c_str());
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
    oldLCD = new unsigned char *[(width + 7) / 8];
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
    for (x = 0; x < 1000; x++)
    {
        port->WriteData(x % 0x100);
    }
    gettimeofday(&tv2, 0);
    if (useSleepInit)
        nSleepDeInit();
    timeForPortCmdInNs = (tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec);
    syslog(LOG_DEBUG, "%s: benchmark stopped. Time for Command: %ldns\n", config->name.c_str(), timeForPortCmdInNs);

    // initialize graphic mode
    InitGraphic();

    *oldConfig = *config;

    // clear display
    Clear();
    // The SED1330 can have up to 64k memory. If there is less memory
    // it will be overwritten twice or more times.
    WriteCmd(C_MWRITE);
    for (x = 0; x < 65536; x++)
        WriteData(0x00);

    WriteCmd(C_CSRW);
    WriteData(0x00); // initializing cursor adress, low byte
    WriteData(0x00); // high byte

    port->Release();

    syslog(LOG_INFO, "%s: SED1330 initialized.\n", config->name.c_str());
    return 0;
}

int cDriverSED1330::DeInit()
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

int cDriverSED1330::CheckSetup()
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

int cDriverSED1330::InitGraphic()
{
    // initialize setup with two graphic screens
    // most parts taken from Thomas Baumann's LCD-Test program
    int cr;
    int memGraph;
    int sad1l, sad1h, sad2l, sad2h;

    cr = (width / FX - 1) * BPC;
    memGraph = ((cr + 1) * height);  // required memory for a graphic layer
    sad1l = SAD1 & 0xFF;
    sad1h = SAD1 >> 8 & 0xFF;
    sad2l = ((SAD1 + memGraph) & 0xFF);
    sad2h = ((SAD1 + memGraph) >> 8 & 0xFF);

    WriteCmd(C_SYSTEMSET);
    WriteData(0x30 + M0 + (M1 << 1) + (M2 << 2));
    WriteData(0x80 + (FX - 1));
    WriteData(0x00 + (FY - 1));
    WriteData(cr);   // C/R .. display adresses per line
    WriteData((oscillatorFrequency * 1000 / (70 * height) - 1) / 9);  // TC/R .. , fFR=70Hz
    WriteData(height - 1); // L/F .. display lines per screen
    WriteData(cr + 1);   // adresses per virtual display line, low byte
    WriteData(0x00);   // adresses per virtual display line, high byte
    // currently we don't use virtual screens greater then the display,
    // therefore the high byte should always be zero

    WriteCmd(C_SCROLL);
    WriteData(sad1l);  // low-byte startadress first layer
    WriteData(sad1h);  // high-byte startadress first layer
    WriteData(height); // lines per screen
    WriteData(sad2l);  // low-byte startadress second layer
    WriteData(sad2h);  // high-byte startadress second layer
    WriteData(height); // lines per screen
    WriteData(0x00); // low-byte startadress third layer, not used
    WriteData(0x00); // high-byte startadress third layer, not used
    WriteData(0x00); // low-byte startadress fourth layer, not used
    WriteData(0x00); // high-byte startadress fourth layer, not used

    WriteCmd(C_CSRFORM);
    WriteData(0x00); // cursor with: 1 pixel
    WriteData(0x86); // cursor height: 7 lines, block mode

    WriteCmd(C_CSRDIR_R);  // automatic cursor increment to the right

    WriteCmd(C_OVLAY);
    WriteData(0x0C); // two layer composition with Priority-OR

    WriteCmd(C_HDOTSCR);
    WriteData(0x00);

    WriteCmd(C_DISPON);  // display ON with
    WriteData(0x04); // cursor OFF and first layer ON without flashing

    WriteCmd(C_CSRW);
    WriteData(0x00); // initializing cursor adress, low byte
    WriteData(0x00); // high byte

    return 0;
}

void cDriverSED1330::WriteCmd(unsigned char cmd)
{
    //if (useSleepInit)
    //    nSleepInit();

    if (interface == kInterface6800)
    {
        // set A0 high (instruction), RW low (write) and E low
        port->WriteControl(A0HI | CSLO | RWLO | ENLO);
        //nSleep(140 - timeForPortCmdInNs + 100 * config->adjustTiming);
        // Output the actual command
        port->WriteData(cmd);
        //nSleep(140 - timeForPortCmdInNs + 100 * config->adjustTiming);
        // set E high
        port->WriteControl(A0HI | CSLO | RWLO | ENHI);
        //nSleep(450 - timeForPortCmdInNs + 100 * config->adjustTiming);
        // set E low
        port->WriteControl(A0HI | CSLO | RWLO | ENLO);
        //nSleep(450 - timeForPortCmdInNs + 100 * config->adjustTiming);
    }
    else
    {
        // set A0 high (instruction), CS low, RD and WR high
        port->WriteControl(A0HI | CSLO | RDHI | WRHI);
        //nSleep(140 - timeForPortCmdInNs + 100 * config->adjustTiming);
        // Output the actual command
        port->WriteData(cmd);
        //nSleep(140 - timeForPortCmdInNs + 100 * config->adjustTiming);
        // set WR low
        port->WriteControl(A0HI | CSLO | RDHI | WRLO);
        //nSleep(450 - timeForPortCmdInNs + 100 * config->adjustTiming);
        // set WR high
        port->WriteControl(A0HI | CSLO | RDHI | WRHI);
        //nSleep(450 - timeForPortCmdInNs + 100 * config->adjustTiming);
    }

    //if (useSleepInit)
    //    nSleepDeInit();
}

void cDriverSED1330::WriteData(unsigned char data)
{
    //if (useSleepInit)
    //    nSleepInit();

    if (interface == kInterface6800)
    {
        // set A0 low (data), RW low (write) and E low
        port->WriteControl(A0LO | CSLO | RWLO | ENLO);
        //nSleep(140 - timeForPortCmdInNs + 100 * config->adjustTiming);
        // Output the actual data
        port->WriteData(data);
        //nSleep(140 - timeForPortCmdInNs + 100 * config->adjustTiming);
        // set E high
        port->WriteControl(A0LO | CSLO | RWLO | ENHI);
        //nSleep(450 - timeForPortCmdInNs + 100 * config->adjustTiming);
        // set E low
        port->WriteControl(A0LO | CSLO | RWLO | ENLO);
        //nSleep(450 - timeForPortCmdInNs + 100 * config->adjustTiming);
    }
    else
    {
        // set A0 low (data), CS low, RD and WR high
        port->WriteControl(A0LO | CSLO | RDHI | WRHI);
        //nSleep(140 - timeForPortCmdInNs + 100 * config->adjustTiming);
        // Output the actual data
        port->WriteData(data);
        //nSleep(140 - timeForPortCmdInNs + 100 * config->adjustTiming);
        // set WR low
        port->WriteControl(A0LO | CSLO | RDHI | WRLO);
        //nSleep(450 - timeForPortCmdInNs + 100 * config->adjustTiming);
        // set WR high
        port->WriteControl(A0LO | CSLO | RDHI | WRHI);
        //nSleep(450 - timeForPortCmdInNs + 100 * config->adjustTiming);
    }

    //if (useSleepInit)
    //    nSleepDeInit();
}

void cDriverSED1330::Clear()
{
    for (int x = 0; x < (width + 7) / 8; x++)
        memset(newLCD[x], 0, height);
}


void cDriverSED1330::SetPixel(int x, int y, uint32_t data)
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


#if 0
void cDriverSED1330::Set8Pixels(int x, int y, unsigned char data)
{
    if (x >= width || y >= height)
        return;

    if (!config->upsideDown)
    {
        // normal orientation
        newLCD[x / 8][y] = newLCD[x / 8][y] | data;
    }
    else
    {
        // upside down orientation
        x = width - 1 - x;
        y = height - 1 - y;
        newLCD[x / 8][y] = newLCD[x / 8][y] | ReverseBits(data);
    }
}
#endif

void cDriverSED1330::Refresh(bool refreshAll)
{
    int x;
    int y;
    int pos = SAD1;

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
        // set cursor to startadress
        WriteCmd(C_CSRW);
        WriteData(pos & 0xFF);
        WriteData(pos >> 8);

        for (y = 0; y < height; y++)
        {
            for (x = 0; x < (width + 7) / 8; x++)
            {
                WriteCmd(C_MWRITE); // cursor increments automatically
                WriteData(newLCD[x][y] ^ (config->invert ? 0xff : 0x00));
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
                        WriteCmd(C_CSRW);
                        WriteData(pos & 0xFF);
                        WriteData(pos >> 8);
                        WriteCmd(C_MWRITE);
                        cs = true;
                    }
                    WriteData(newLCD[x][y] ^ (config->invert ? 0xff : 0x00));
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
