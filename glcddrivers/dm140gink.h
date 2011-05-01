/*
 * GraphLCD driver library
 *
 * framebuffer.h  -  framebuffer device
 *                   Output goes to a framebuffer device
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2004 Stephan Skrodzki
 */

#ifndef _GLCDDRIVERS_DM140GINK_H_
#define _GLCDDRIVERS_DM140GINK_H_

#include "driver.h"


namespace GLCD
{

class cDriverConfig;

class cDriverDM140GINK : public cDriver
{
private:
    cDriverConfig * config;
    cDriverConfig * oldConfig;

    int fd;

    int vendor;
    int product;

    char *framebuff;

    long int screensize;

    int SendReport(const char *buf, size_t size);
    int CheckSetup();
    void SetPixel(int x, int y, uint32_t data);

public:
    cDriverDM140GINK(cDriverConfig * config);
    virtual ~cDriverDM140GINK();

    virtual int Init();
    virtual int DeInit();

    virtual void Clear();
    virtual void Set8Pixels(int x, int y, unsigned char data);
    virtual void Refresh(bool refreshAll = false);
};

} // end of namespace

#endif
