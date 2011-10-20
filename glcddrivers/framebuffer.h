/*
 * GraphLCD driver library
 *
 * framebuffer.h  -  framebuffer device
 *                   Output goes to a framebuffer device
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2004      Stephan Skrodzki
 * (c) 2011      Wolfgang Astleitner <mrwastl AT users.sourceforge.net>
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
    char *offbuff;
    int fbfd;
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;
    long int screensize;
    void *fbp;
    int zoom;
    int damage;
    int bbox[4];
    int depth;
    uint32_t roff, boff, goff, aoff;
    uint32_t rlen, blen, glen, alen;

    int CheckSetup();
    void processDamage (void);
protected:
    virtual bool GetDriverFeature  (const std::string & Feature, int & value);  
public:
    cDriverFramebuffer(cDriverConfig * config);

    virtual int Init();
    virtual int DeInit();

    virtual void Clear();
    virtual void SetPixel(int x, int y, uint32_t data);
    //virtual void Set8Pixels(int x, int y, unsigned char data);
    virtual void Refresh(bool refreshAll = false);
};

} // end of namespace

#endif
