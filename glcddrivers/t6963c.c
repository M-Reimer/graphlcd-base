/*
 * GraphLCD driver library
 *
 * t6963c.c  -  T6963C driver class
 *
 * low level routines based on lcdproc 0.5 driver, (c) 2001 Manuel Stahl
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2003-2004 Andreas Regel <andreas.regel AT powarman.de>
 * (c) 2011      Wolfgang Astleitner <mrwastl AT users.sourceforge.net>
 */

#include <syslog.h>
#include <cstring>
#include <cstdlib>

#include "common.h"
#include "config.h"
#include "port.h"
#include "t6963c.h"


namespace GLCD
{

// T6963 commands
const unsigned char kSetCursorPointer  = 0x21;
const unsigned char kSetOffsetRegister = 0x22;
const unsigned char kSetAddressPointer = 0x24;

const unsigned char kSetTextHomeAddress    = 0x40;
const unsigned char kSetTextArea           = 0x41;
const unsigned char kSetGraphicHomeAddress = 0x42;
const unsigned char kSetGraphicArea        = 0x43;

const unsigned char kSetMode          = 0x80;
const unsigned char kSetDisplayMode   = 0x90;
const unsigned char kSetCursorPattern = 0xA0;

const unsigned char kDataWriteInc = 0xC0;
const unsigned char kDataReadInc  = 0xC1;
const unsigned char kDataWriteDec = 0xC2;
const unsigned char kDataReadDec  = 0xC3;
const unsigned char kDataWrite    = 0xC4;
const unsigned char kDataRead     = 0xC5;

const unsigned char kAutoWrite = 0xB0;
const unsigned char kAutoRead  = 0xB1;
const unsigned char kAutoReset = 0xB2;


// T6963 Parameters
const unsigned char kModeOr            = 0x00;
const unsigned char kModeXor           = 0x01;
const unsigned char kModeAnd           = 0x03;
const unsigned char kModeTextAttribute = 0x04;
const unsigned char kModeInternalCG    = 0x00;
const unsigned char kModeExternalCG    = 0x08;

const unsigned char kTextAttributeNormal    = 0x00;
const unsigned char kTextAttributeInverse   = 0x05;
const unsigned char kTextAttributeNoDisplay = 0x03;
const unsigned char kTextAttributeBlink     = 0x08;

const unsigned char kDisplayModeBlink   = 0x01;
const unsigned char kDisplayModeCursor  = 0x02;
const unsigned char kDisplayModeText    = 0x04;
const unsigned char kDisplayModeGraphic = 0x08;

const unsigned short kGraphicBase = 0x0000;
const unsigned short kTextBase    = 0x1500;
const unsigned short kCGRAMBase   = 0x1800;


// T6963 Wirings
static const std::string kWiringStandard = "Standard";
static const std::string kWiringWindows  = "Windows";
static const std::string kWiringSerial   = "Serial";

const unsigned char kStandardWRHI = 0x00; // 01 / nSTRB
const unsigned char kStandardWRLO = 0x01; //
const unsigned char kStandardRDHI = 0x00; // 17 / nSELECT
const unsigned char kStandardRDLO = 0x08; //
const unsigned char kStandardCEHI = 0x00; // 14 / nLINEFEED
const unsigned char kStandardCELO = 0x02; //
const unsigned char kStandardCDHI = 0x04; // 16 / INIT
const unsigned char kStandardCDLO = 0x00; //

const unsigned char kWindowsWRHI = 0x04; // 16 / INIT
const unsigned char kWindowsWRLO = 0x00; //
const unsigned char kWindowsRDHI = 0x00; // 14 / nLINEFEED
const unsigned char kWindowsRDLO = 0x02; //
const unsigned char kWindowsCEHI = 0x00; // 01 / nSTRB
const unsigned char kWindowsCELO = 0x01; //
const unsigned char kWindowsCDHI = 0x00; // 17 / nSELECT
const unsigned char kWindowsCDLO = 0x08; //

const unsigned char kSerialWRHI = 0x01; // 01 / nSTRB
const unsigned char kSerialWRLO = 0x00; //
const unsigned char kSerialRDHI = 0x08; // 17 / nSELECT
const unsigned char kSerialRDLO = 0x00; //
const unsigned char kSerialCEHI = 0x02; // 14 / nLINEFEED
const unsigned char kSerialCELO = 0x00; //
const unsigned char kSerialCDHI = 0x00; // 16 / INIT
const unsigned char kSerialCDLO = 0x04; //


cDriverT6963C::cDriverT6963C(cDriverConfig * config)
:   cDriver(config)
{
    port = new cParallelPort();

    //width = config->width;
    //height = config->height;
    refreshCounter = 0;
    displayMode = 0;
    bidirectLPT = 1;
    autoWrite = false;
    serial = 0;
}

cDriverT6963C::~cDriverT6963C()
{
    delete port;
}

int cDriverT6963C::Init()
{
    int x;

    width = config->width;
    if (width <= 0)
        width = 240;
    height = config->height;
    if (height <= 0)
        height = 128;

    // default values
    FS = 6;
    WRHI = kStandardWRHI;
    WRLO = kStandardWRLO;
    RDHI = kStandardRDHI;
    RDLO = kStandardRDLO;
    CEHI = kStandardCEHI;
    CELO = kStandardCELO;
    CDHI = kStandardCDHI;
    CDLO = kStandardCDLO;
    useAutoMode = true;
    useStatusCheck = true;

    for (unsigned int i = 0; i < config->options.size(); i++)
    {
        if (config->options[i].name == "FontSelect")
        {
            int fontSelect = atoi(config->options[i].value.c_str());
            if (fontSelect == 6)
                FS = 6;
            else if (fontSelect == 8)
                FS = 8;
            else
                syslog(LOG_ERR, "%s error: font select %d not supported, using default (%d)!\n",
                       config->name.c_str(), fontSelect, FS);
        }
        else if (config->options[i].name == "Wiring")
        {
            if (config->options[i].value == kWiringStandard)
            {
                WRHI = kStandardWRHI;
                WRLO = kStandardWRLO;
                RDHI = kStandardRDHI;
                RDLO = kStandardRDLO;
                CEHI = kStandardCEHI;
                CELO = kStandardCELO;
                CDHI = kStandardCDHI;
                CDLO = kStandardCDLO;
            }
            else if (config->options[i].value == kWiringWindows)
            {
                WRHI = kWindowsWRHI;
                WRLO = kWindowsWRLO;
                RDHI = kWindowsRDHI;
                RDLO = kWindowsRDLO;
                CEHI = kWindowsCEHI;
                CELO = kWindowsCELO;
                CDHI = kWindowsCDHI;
                CDLO = kWindowsCDLO;
            }
            else if (config->options[i].value == kWiringSerial)
            {
                serial = 1;
                WRHI = kSerialWRHI;
                WRLO = kSerialWRLO;
                RDHI = kSerialRDHI;
                RDLO = kSerialRDLO;
                CEHI = kSerialCEHI;
                CELO = kSerialCELO;
                CDHI = kSerialCDHI;
                CDLO = kSerialCDLO;
            }
            else
                syslog(LOG_ERR, "%s error: wiring %s not supported, using default (Standard)!\n",
                       config->name.c_str(), config->options[i].value.c_str());
        }
        else if (config->options[i].name == "AutoMode")
        {
            if (config->options[i].value == "yes")
                useAutoMode = true;
            else if (config->options[i].value == "no")
                useAutoMode = false;
            else
                syslog(LOG_ERR, "%s error: unknown auto mode setting %s, using default (%s)!\n",
                       config->name.c_str(), config->options[i].value.c_str(), useAutoMode ? "yes" : "no");
        }
        else if (config->options[i].name == "StatusCheck")
        {
            if (config->options[i].value == "yes")
                useStatusCheck = true;
            else if (config->options[i].value == "no")
                useStatusCheck = false;
            else
                syslog(LOG_ERR, "%s error: unknown status check setting %s, using default (%s)!\n",
                       config->name.c_str(), config->options[i].value.c_str(), useStatusCheck ? "yes" : "no");
        }
    }

    // setup lcd array (wanted state)
    newLCD = new unsigned char*[(width + (FS - 1)) / FS];
    if (newLCD)
    {
        for (x = 0; x < (width + (FS - 1)) / FS; x++)
        {
            newLCD[x] = new unsigned char[height];
            memset(newLCD[x], 0, height);
        }
    }
    // setup lcd array (current state)
    oldLCD = new unsigned char*[(width + (FS - 1)) / FS];
    if (oldLCD)
    {
        for (x = 0; x < (width + (FS - 1)) / FS; x++)
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

    // disable chip
    // disable reading from LCD
    // disable writing to LCD
    // command/status mode
    T6963CSetControl(WRHI | CEHI | CDHI | RDHI);
    port->SetDirection(kForward); // make 8-bit parallel port an output port

    // Test ECP mode
    if (bidirectLPT == 1)
    {
        syslog(LOG_DEBUG, "%s: Testing ECP mode...\n", config->name.c_str());
        int i = 0;
        int ecp_input;
        port->SetDirection(kReverse);
        for (int i = 0; i < 100; i++)
        {
            T6963CSetControl(WRHI | CEHI | CDHI | RDHI);    // wr, ce, cd, rd
            T6963CSetControl(WRHI | CELO | CDHI | RDLO);
            T6963CSetControl(WRHI | CELO | CDHI | RDLO);
            T6963CSetControl(WRHI | CELO | CDHI | RDLO);
            ecp_input = port->ReadData();
            T6963CSetControl(WRHI | CEHI | CDHI | RDHI);
            if ((ecp_input & 0x03) == 0x03)
                break;
        }
        port->SetDirection(kForward);
        if (i >= 100)
        {
            syslog(LOG_DEBUG, "%s: ECP mode not working! -> is now disabled\n", config->name.c_str());
            bidirectLPT = 0;
        }
        else
            syslog(LOG_DEBUG, "%s: working!\n", config->name.c_str());
    }

    T6963CCommandWord(kSetGraphicHomeAddress, kGraphicBase);
    if (width % FS == 0)
        T6963CCommandWord(kSetGraphicArea, width / FS);
    else
        T6963CCommandWord(kSetGraphicArea, width / FS + 1);

    T6963CCommand(kSetMode | kModeOr | kModeInternalCG);

    T6963CDisplayMode(kDisplayModeText, false);
    T6963CDisplayMode(kDisplayModeGraphic, true);
    T6963CDisplayMode(kDisplayModeCursor, false);
    T6963CDisplayMode(kDisplayModeBlink, false);

    port->Release();

    *oldConfig = *config;

    // clear display
    Clear();

    syslog(LOG_INFO, "%s: T6963 initialized.\n", config->name.c_str());
    return 0;
}

int cDriverT6963C::DeInit()
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

int cDriverT6963C::CheckSetup()
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

void cDriverT6963C::Clear()
{
    for (int x = 0; x < (width + (FS - 1)) / FS; x++)
        memset(newLCD[x], 0, height);
}


void cDriverT6963C::SetPixel(int x, int y, uint32_t data)
{
    if (x >= width || y >= height)
        return;

    if (FS == 6)
    {
        int pos = x % 6;
        if (config->upsideDown)
        {
            x = width - 1 - x;
            y = height - 1 - y;
        } else {
            pos = 5 - pos; // reverse bit position
        }

        // columns_per_line = (width + FS6-1) / FS6  (FS6 == 6 bits per byte used)
        //int cols = (width + 6 - 1 ) / 6;
        int col = x / 6;
        
        if (data == GRAPHLCD_White)
            newLCD[col][y] |= (1 << pos);
        else
            newLCD[col][y] &= ( 0x3F ^ (1 << pos) );        
    }
    else
    {
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
}


#if 0
void cDriverT6963C::Set8Pixels(int x, int y, unsigned char data)
{
    if (x >= width || y >= height)
        return;

    if (FS == 6)
    {
        unsigned char data1 = 0;
        unsigned char data2 = 0;
        unsigned char data3 = 0;

        if (!config->upsideDown)
        {
            // normal orientation
            x = x - (x % 8);
            data1 = data >> (2 + (x % 6));
            if (x % 6 == 5)
            {
                data2 = data >> 1;
                data3 = data << 5;
            }
            else
                data2 = data << (4 - (x % 6));

            newLCD[x / 6][y] |= data1;
            if (x / 6 + 1 < (width + 5) / 6)
                newLCD[x / 6 + 1][y] |= data2;
            if (x / 6 + 2 < (width + 5) / 6)
                if (x % 6 == 5)
                    newLCD[x / 6 + 2][y] |= data3;
        }
        else
        {
            // upside down orientation
            x = width - 1 - x;
            y = height - 1 - y;
            x = x - (x % 8);
            data = ReverseBits(data);

            data1 = data >> (2 + (x % 6));
            if (x % 6 == 5)
            {
                data2 = data >> 1;
                data3 = data << 5;
            }
            else
                data2 = data << (4 - (x % 6));

            newLCD[x / 6][y] |= data1;
            if (x / 6 + 1 < (width + 5) / 6)
                newLCD[x / 6 + 1][y] |= data2;
            if (x / 6 + 2 < (width + 5) / 6)
                if (x % 6 == 5)
                    newLCD[x / 6 + 2][y] |= data3;
        }
    }
    else
    {
        if (!config->upsideDown)
        {
            newLCD[x / 8][y] |= data;
        }
        else
        {
            x = width - 1 - x;
            y = height - 1 - y;
            newLCD[x / 8][y] |= ReverseBits(data);
        }
    }
}
#endif

void cDriverT6963C::Refresh(bool refreshAll)
{
    int x,y;
    int addr = 0;

    if (CheckSetup() == 1)
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
        T6963CCommandWord(kSetAddressPointer, kGraphicBase);
        if (useAutoMode)
        {
            T6963CCommand(kAutoWrite);
            autoWrite = true;
        }
        for (y = 0; y < height; y++)
        {
            for (x = 0; x < (width + (FS - 1)) / FS; x++)
            {
                if (autoWrite)
                    T6963CData((newLCD[x][y]) ^ (config->invert ? 0xff : 0x00));
                else
                    T6963CCommandByte(kDataWriteInc, (newLCD[x][y]) ^ (config->invert ? 0xff : 0x00));
                oldLCD[x][y] = newLCD[x][y];
            }
        }
        if (autoWrite)
        {
            T6963CCommand(kAutoReset);
            autoWrite = false;
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
                        T6963CCommandWord(kSetAddressPointer, kGraphicBase + addr);
                        if (useAutoMode)
                        {
                            T6963CCommand(kAutoWrite);
                            autoWrite = true;
                        }
                        cs = true;
                    }
                    if (autoWrite)
                        T6963CData((newLCD[x][y]) ^ (config->invert ? 0xff : 0x00));
                    else
                        T6963CCommandByte(kDataWriteInc, (newLCD[x][y]) ^ (config->invert ? 0xff : 0x00));
                    oldLCD[x][y] = newLCD[x][y];
                }
                else
                {
                    if (autoWrite)
                    {
                        T6963CCommand(kAutoReset);
                        autoWrite = false;
                    }
                    cs = false;
                }
            }
        }
        if (autoWrite)
        {
            T6963CCommand(kAutoReset);
            autoWrite = false;
        }
    }
    port->Release();
}

void cDriverT6963C::T6963CSetControl(unsigned char flags)
{
    unsigned char status = port->ReadControl();
    status &= 0xF0; // mask 4 bits
    status |= flags; // add new flags
    port->WriteControl(status);
}

void cDriverT6963C::T6963CDSPReady()
{
    int input = 0;

    port->SetDirection(kReverse);
    if (bidirectLPT == 1)
    {
        for (int i = 0; i < 10; i++)
        {
            T6963CSetControl(WRHI | CEHI | CDHI | RDHI);
            T6963CSetControl(WRHI | CELO | CDHI | RDLO);
            input = port->ReadData();
            T6963CSetControl(WRHI | CEHI | CDHI | RDHI);
            if (!autoWrite && (input & 3) == 3)
                break;
            if (autoWrite && (input & 8) == 8)
                break;
        }
    }
    else
    {
        T6963CSetControl(WRHI | CEHI | CDHI | RDHI);
        T6963CSetControl(WRHI | CELO | CDHI | RDLO);
        T6963CSetControl(WRHI | CEHI | CDHI | RDHI);
    }
    port->SetDirection(kForward);
}

void cDriverT6963C::T6963CData(unsigned char data)
{
    if (serial)
    {
        T6963CSetControl(WRLO | CEHI | CDLO | RDLO);
        for (int i = 128; i; i>>=1)
        {
            if (data & i)
            {
                T6963CSetControl(WRLO | CEHI | CDHI | RDLO);
                T6963CSetControl(WRHI | CEHI | CDHI | RDLO);
            }
            else
            {
                T6963CSetControl(WRLO | CEHI | CDLO | RDLO);
                T6963CSetControl(WRHI | CEHI | CDLO | RDLO);
            }
        }
        T6963CSetControl(WRLO | CEHI | CDLO | RDLO); // CD down (data)
        T6963CSetControl(WRLO | CELO | CDLO | RDLO); // CE down
        T6963CSetControl(WRLO | CEHI | CDLO | RDLO); // CE up
    }
    else
    {
        if (useStatusCheck)
            T6963CDSPReady();
        T6963CSetControl(WRHI | CEHI | CDLO | RDHI); // CD down (data)
        T6963CSetControl(WRLO | CELO | CDLO | RDHI); // CE & WR down
        port->WriteData(data);
        T6963CSetControl(WRHI | CEHI | CDLO | RDHI); // CE & WR up again
        T6963CSetControl(WRHI | CEHI | CDHI | RDHI); // CD up again
    }
}

void cDriverT6963C::T6963CCommand(unsigned char cmd)
{
    if (serial)
    {
        syslog(LOG_DEBUG, "Serial cmd out: ");
        T6963CSetControl(WRLO | CEHI | CDLO | RDLO);
        for (int i = 128; i; i>>=1)
        {
            if (cmd & i)
            {
                T6963CSetControl(WRLO | CEHI | CDHI | RDLO);
                T6963CSetControl(WRHI | CEHI | CDHI | RDLO);
            }
            else
            {
                T6963CSetControl(WRLO | CEHI | CDLO | RDLO);
                T6963CSetControl(WRHI | CEHI | CDLO | RDLO);
            }
        }
        T6963CSetControl(WRLO | CEHI | CDHI | RDLO); // CD up (command)
        T6963CSetControl(WRLO | CELO | CDHI | RDLO); // CE down
        T6963CSetControl(WRLO | CEHI | CDHI | RDLO); // CE up
        T6963CSetControl(WRLO | CEHI | CDLO | RDLO); // CD down
    }
    else
    {
        if (useStatusCheck)
            T6963CDSPReady();
        T6963CSetControl(WRHI | CEHI | CDHI | RDHI); // CD up (command)
        T6963CSetControl(WRLO | CELO | CDHI | RDHI); // CE & WR down
        port->WriteData(cmd);
        T6963CSetControl(WRHI | CEHI | CDHI | RDHI); // CE & WR up again
        T6963CSetControl(WRHI | CEHI | CDLO | RDHI); // CD down again
    }
}

void cDriverT6963C::T6963CCommandByte(unsigned char cmd, unsigned char data)
{
    T6963CData(data);
    T6963CCommand(cmd);
}

void cDriverT6963C::T6963CCommand2Bytes(unsigned char cmd, unsigned char data1, unsigned char data2)
{
    T6963CData(data1);
    T6963CData(data2);
    T6963CCommand(cmd);
}

void cDriverT6963C::T6963CCommandWord(unsigned char cmd, unsigned short data)
{
    T6963CData(data % 256);
    T6963CData(data >> 8);
    T6963CCommand(cmd);
}

void cDriverT6963C::T6963CDisplayMode(unsigned char mode, bool enable)
{
    if (enable)
        displayMode |= mode;
    else
        displayMode &= ~mode;
    T6963CCommand(kSetDisplayMode | displayMode);
}

}
