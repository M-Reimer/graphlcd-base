/*
 * GraphLCD driver library
 *
 * hd61830.h  -  HD61830 driver class
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2001-2004 Carsten Siebholz <c.siebholz AT t-online.de>
 * (c) 2005-2010 Andreas Regel <andreas.regel AT powarman.de>
 * (c) 2011      Wolfgang Astleitner <mrwastl AT users.sourceforge.net>
 */

#ifndef _GLCDDRIVERS_HD61830_H_
#define _GLCDDRIVERS_HD61830_H_

#include "driver.h"

namespace GLCD
{

class cDriverConfig;
class cParallelPort;

class cDriverHD61830 : public cDriver
{
private:
    cParallelPort * port;

    unsigned char ** newLCD; // wanted state
    unsigned char ** oldLCD; // current state
    int refreshCounter;
    long timeForPortCmdInNs;
    bool useSleepInit;

    int CheckSetup();
    int InitGraphic();
    void Write(unsigned char cmd, unsigned char data);

public:
    cDriverHD61830(cDriverConfig * config);
    virtual ~cDriverHD61830();

    virtual int Init();
    virtual int DeInit();

    virtual void Clear();
    virtual void SetPixel(int x, int y, uint32_t data);
    //virtual void Set8Pixels(int x, int y, unsigned char data);
    virtual void Refresh(bool refreshAll = false);
};

} // end of namespace

#endif
