/*
 * GraphLCD driver library
 *
 * simlcd.h  -  SimLCD driver class
 *              Output goes to a file instead of lcd.
 *              Use SimLCD tool to view this file.
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2001-2004 Carsten Siebholz <c.siebholz AT t-online.de>
 */

#ifndef _GLCDDRIVERS_SIMLCD_H_
#define _GLCDDRIVERS_SIMLCD_H_

#include "driver.h"


namespace GLCD
{

class cDriverConfig;

class cDriverSimLCD : public cDriver
{
private:
    unsigned char ** LCD;
    cDriverConfig * config;
    cDriverConfig * oldConfig;

    int CheckSetup();

public:
    cDriverSimLCD(cDriverConfig * config);
    virtual ~cDriverSimLCD();

    virtual int Init();
    virtual int DeInit();

    virtual void Clear();
    virtual void Set8Pixels(int x, int y, unsigned char data);
    virtual void Refresh(bool refreshAll = false);
};

} // end of namespace

#endif
