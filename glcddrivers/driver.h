/*
 * GraphLCD driver library
 *
 * driver.h  -  driver base class
 *
 * parts were taken from graphlcd plugin for the Video Disc Recorder
 * (c) 2001-2004 Carsten Siebholz <c.siebholz AT t-online.de>
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2004 Andreas Regel <andreas.regel AT powarman.de>
 */

#ifndef _GLCDDRIVERS_DRIVER_H_
#define _GLCDDRIVERS_DRIVER_H_

#include <stdint.h>

namespace GLCD
{

class cDriver
{
protected:
    int width;
    int height;
public:
    cDriver();
    virtual ~cDriver() {}

    int Width() const { return width; }
    int Height() const { return height; }

    virtual int Init() { return 0; }
    virtual int DeInit() { return 0; }

    virtual void Clear() {}
    virtual void Set8Pixels(int x, int y, unsigned char data) {}
    virtual void SetScreen(const unsigned char * data, int width, int height, int lineSize);
    virtual void Refresh(bool refreshAll = false) {}

    virtual void SetBrightness(unsigned int percent) {}
};

} // end of namespace

#endif
