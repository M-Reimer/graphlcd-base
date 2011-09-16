/*
 * GraphLCD driver library
 *
 * gu256x64-372.h  -  8-bit driver module for Noritake GU256x64-372
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

#ifndef _GLCDDRIVERS_GU256X64_372_H_
#define _GLCDDRIVERS_GU256X64_372_H_

#include "driver.h"

namespace GLCD
{

class cDriverConfig;
class cParallelPort;

class cDriverGU256X64_372 : public cDriver
{
    cParallelPort * port;

    int m_iSizeYb;
    int m_nRefreshCounter;

    unsigned char ** m_pDrawMem; // the draw "memory"
    unsigned char ** m_pVFDMem; // the double buffed display "memory"

    long m_nTimingAdjustCmd;
    bool m_bSleepIsInit;

    int CheckSetup();

    int WRHI;
    int WRLO;
    int RDHI;
    int RDLO;
    int CDHI;
    int CDLO;

protected:
    void ClearVFDMem();
    void GU256X64Cmd(unsigned char data);
    void GU256X64Data(unsigned char data);

public:
    cDriverGU256X64_372(cDriverConfig * config);
    virtual ~cDriverGU256X64_372();

    virtual int Init();
    virtual int DeInit();

    virtual void Clear();
    virtual void SetPixel(int x, int y, uint32_t data);
    //virtual void Set8Pixels(int x, int y, unsigned char data);
    virtual void Refresh(bool refreshAll = false);

    virtual void SetBrightness(unsigned int percent);
};

} // end of namespace

#endif
