/*
 * GraphLCD driver library
 *
 * noritake800.h  -  Noritake 800(A) series VFD graphlcd driver,
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

#ifndef _GLCDDRIVERS_NORITAKE800_H_
#define _GLCDDRIVERS_NORITAKE800_H_

#include "driver.h"

namespace GLCD
{

class cDriverConfig;
class cParallelPort;

class cDriverNoritake800 : public cDriver
{
    cParallelPort * m_pport;

    int m_iSizeYb;
    int m_nRefreshCounter;
    int m_nWiring;

    unsigned char ** m_pDrawMem;             /* the draw "memory" */
    unsigned char ** m_pVFDMem;              /* the double buffed display "memory" */

    long m_nTimingAdjustCmd;
    bool m_bSleepIsInit;

    // internal graphics layers
    bool m_bGraphScreen0_On;
    bool m_bGraphScreen1_On;

    unsigned char * m_pWiringMaskCache;

protected:
    void ClearVFDMem();
    void N800Cmd(unsigned char data);
    void N800Data(unsigned char data);
    int CheckSetup();
    unsigned char N800LptWiringMask(unsigned char ctrl_bits);
    void N800WriteByte(unsigned char data, int nCol, int nRow, int layer);

public:
    cDriverNoritake800(cDriverConfig * config);
    virtual ~cDriverNoritake800();

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
