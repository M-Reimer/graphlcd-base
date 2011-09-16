/*
 * GraphLCD driver library
 *
 * gu140x32f.h  -  8-bit driver module for Noritake GU140x32-F7806 VFD
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

#ifndef _GLCDDRIVERS_GU140X32F_H_
#define _GLCDDRIVERS_GU140X32F_H_

#include "driver.h"


namespace GLCD
{

class cDriverConfig;
class cParallelPort;


class cDriverGU140X32F : public cDriver
{
    unsigned char m_WiringRS;
    unsigned char m_WiringEN1;

    cParallelPort * port;

    int m_iSizeYb;
    int m_nRefreshCounter;
    unsigned char *m_pDrawMem; // the draw "memory"
    unsigned char *m_pVFDMem;  // the double buffed display "memory"
    long m_nTimingAdjustCmd;
    bool m_bSleepIsInit;

    int CheckSetup();

protected:
    void ClearVFDMem();
    void Write(unsigned char nFlags, unsigned char bData, unsigned int nMicroSecBusyTime);

public:
    cDriverGU140X32F(cDriverConfig * config);
    virtual ~cDriverGU140X32F();

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
