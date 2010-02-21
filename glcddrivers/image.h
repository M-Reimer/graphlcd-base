/*
 * GraphLCD driver library
 *
 * image.h  -  Image output device
 *             Output goes to a image file instead of LCD.
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2004 Andreas Regel <andreas.regel AT powarman.de>
 */

#ifndef _GLCDDRIVERS_IMAGE_H_
#define _GLCDDRIVERS_IMAGE_H_

#include "driver.h"


namespace GLCD
{

class cDriverConfig;

class cDriverImage : public cDriver
{
private:
    unsigned char * newLCD;
    unsigned char * oldLCD;
    cDriverConfig * config;
    cDriverConfig * oldConfig;
    int lineSize;
    int counter;

    int CheckSetup();

public:
    cDriverImage(cDriverConfig * config);
    virtual ~cDriverImage();

    virtual int Init();
    virtual int DeInit();

    virtual void Clear();
    virtual void Set8Pixels(int x, int y, unsigned char data);
    virtual void Refresh(bool refreshAll = false);
};

} // end of namespace

#endif
