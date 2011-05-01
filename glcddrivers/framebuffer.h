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

#ifndef _GLCDDRIVERS_FRAMEBUFFER_H_
#define _GLCDDRIVERS_FRAMEBUFFER_H_

#include "driver.h"
#include <linux/fb.h>


namespace GLCD
{

class cDriverConfig;

class cDriverFramebuffer : public cDriver
{
private:
    unsigned char ** LCD;
    cDriverConfig * config;
    cDriverConfig * oldConfig;
    char *offbuff;
    int fbfd;
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;
    long int screensize;
    void *fbp;
    int zoom;

    int CheckSetup();
    void SetPixel(int x, int y, uint32_t data);

public:
    cDriverFramebuffer(cDriverConfig * config);
    virtual ~cDriverFramebuffer();

    virtual int Init();
    virtual int DeInit();

    virtual void Clear();
    virtual void Set8Pixels(int x, int y, unsigned char data);
    virtual void Refresh(bool refreshAll = false);
};

} // end of namespace

#endif
