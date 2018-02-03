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

#ifndef _GLCDDRIVERS_USBSERLED_H_
#define _GLCDDRIVERS_USBSERLED_H_

#include "driver.h"

namespace GLCD
{

class cDriverConfig;
class cSerialPort;

// Possible serial package types
enum glcd_pkgtype: char {
  PKGTYPE_DATA,
  PKGTYPE_BRIGHTNESS
};

class cDriverUSBserLCD : public cDriver
{
private:
    cSerialPort * port;
    unsigned char ** newLCD; // wanted state
    unsigned char ** oldLCD; // current state
    int refreshCounter;
    int displayMode;

    int FS;
    char brightness;

    int CheckSetup();

public:
    cDriverUSBserLCD(cDriverConfig * config);
    virtual ~cDriverUSBserLCD();

    virtual int Init();
    virtual int DeInit();

    virtual void Clear();
    virtual void SetPixel(int x, int y, uint32_t data);
    virtual void Refresh(bool refreshAll = false);
    virtual void SetBrightness(unsigned int percent);
};

} // end of namespace

#endif
