/*
 * GraphLCD driver library
 *
 * t6963c.h  -  T6963C driver class
 *
 * low level routines based on lcdproc 0.5 driver, (c) 2001 Manuel Stahl
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2003-2004 Andreas Regel <andreas.regel AT powarman.de>
 * (c) 2011      Wolfgang Astleitner <mrwastl AT users.sourceforge.net>
 */

#ifndef _GLCDDRIVERS_T6963C_H_
#define _GLCDDRIVERS_T6963C_H_

#include "driver.h"

namespace GLCD
{

class cDriverConfig;
class cParallelPort;

class cDriverT6963C : public cDriver
{
private:
    cParallelPort * port;
    unsigned char ** newLCD; // wanted state
    unsigned char ** oldLCD; // current state
    int refreshCounter;
    int bidirectLPT;
    int displayMode;
    bool useAutoMode;
    bool useStatusCheck;

    int serial;
    int FS;
    int WRHI;
    int WRLO;
    int RDHI;
    int RDLO;
    int CEHI;
    int CELO;
    int CDHI;
    int CDLO;
    bool autoWrite;

    void T6963CSetControl(unsigned char flags);
    void T6963CDSPReady();
    void T6963CData(unsigned char data);
    void T6963CCommand(unsigned char cmd);
    void T6963CCommandByte(unsigned char cmd, unsigned char data);
    void T6963CCommand2Bytes(unsigned char cmd, unsigned char data1, unsigned char data2);
    void T6963CCommandWord(unsigned char cmd, unsigned short data);
    void T6963CDisplayMode(unsigned char mode, bool enable);

    int CheckSetup();

public:
    cDriverT6963C(cDriverConfig * config);
    virtual ~cDriverT6963C();

    virtual int Init();
    virtual int DeInit();

    virtual void Clear();
    virtual void SetPixel(int x, int y, uint32_t data);
    //virtual void Set8Pixels(int x, int y, unsigned char data);
    virtual void Refresh(bool refreshAll = false);
};

} // end of namespace

#endif
