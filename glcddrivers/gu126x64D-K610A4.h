/*
 * GraphLCD driver library
 *
 * gu126x64D-K610A4.h -  8-bit driver module for Noritake GU126x64D-K610A4 VFD
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

#ifndef _GLCDDRIVERS_GU126X64D_K610A4_H_
#define _GLCDDRIVERS_GU126X64D_K610A4_H_

#include "driver.h"

//===============================================================================
//     namespace GLCD
//===============================================================================
namespace GLCD
{

class cDriverConfig;
class cParallelPort;

//===============================================================================
//     class cDriverGU126X64D_K610A4
//===============================================================================
class cDriverGU126X64D_K610A4 : public cDriver
{
public:
    //---------------------------------------------------------------------------
    //     constructor/destructor
    //---------------------------------------------------------------------------
    cDriverGU126X64D_K610A4(cDriverConfig * config);

    //---------------------------------------------------------------------------
    //     from cDriver
    //---------------------------------------------------------------------------
    virtual int Init();
    virtual int DeInit();

    virtual void Clear();
    virtual void SetPixel (int x, int y, uint32_t data);
    //virtual void Set8Pixels(int x, int y, unsigned char data);
    virtual void Refresh(bool refreshAll = false);
    virtual void SetBrightness(unsigned int percent);

    //---------------------------------------------------------------------------
    //     display-specific enums/methods/etc.
    //---------------------------------------------------------------------------
    enum FontType
    {
        FONT_PROP_SML
    ,   FONT_FIX_MED
    ,   FONT_FIX_BIG
    };


    int  cmdReset        ();
    int  cmdPower        (bool fOn);
    int  cmdLock         (bool fLock);
    int  cmdSetCursorPos (unsigned char x, unsigned char y);
    int  cmdGraphicWrite (unsigned char count);
    int  cmdGraphicData  (unsigned char data);
    int  cmdSetBrightness(unsigned int  percent);
    int  cmdSetFont      (FontType);
    int  cmdWriteText    (const char *theText);
    int  cmdDrawRect     (unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2);
    int  cmdSetMacro     (unsigned char theMacroNum, unsigned char theCountBytes);
    int  cmdSetPixel     (bool fSet = true);
    int  cmdClrPixel     ();

    void claimPort();
    void releasePort();

private:
    //---------------------------------------------------------------------------
    //     helper methods
    //---------------------------------------------------------------------------
    void clearVFDMem();
    int  checkSetup();
    int  initParallelPort();
    void initDisplay();

    bool waitForStatus(unsigned char theMask, unsigned char theValue, int theMaxWait);
    void writeParallel(unsigned char data);
    int  write(unsigned char data);

    void ensureNotInGraphics();
    bool isLogEnabled(int theLevel) const;

    //---------------------------------------------------------------------------
    //     attributes
    //---------------------------------------------------------------------------
    cParallelPort  *port;

    int             myNumRows;
    unsigned char **myDrawMem;
    unsigned char **myVFDMem;

    bool            myUseSleepInit;
    long            myPortDelayNS;
    long            myDelay125NS;
    int             myRefreshCounter;
    int             myClaimCounter;
    int             myDataPendingCounter;
    unsigned int    myLogFlags;

}; // class cDriverGU126X64D_K610A4

} // namespace GLCD

#endif
