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

#include <fcntl.h>
#include <stdio.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <cstdlib>
#include <cstring>

#include "common.h"
#include "config.h"
#include "framebuffer.h"


namespace GLCD
{

cDriverFramebuffer::cDriverFramebuffer(cDriverConfig * config)
:   cDriver(config),
    offbuff(0),
    fbfd(-1)
{
}

int cDriverFramebuffer::Init()
{
#if 0  
    // default values
    width = config->width;
    if (width <= 0)
        width = 320;
    height = config->height;
    if (height <= 0)
        height = 240;
#endif
    zoom = 1;
    damage = 0;
    depth = 1;

    for (unsigned int i = 0; i < config->options.size(); i++)
    {
        if (config->options[i].name == "Zoom")
        {
            int z = atoi(config->options[i].value.c_str());
            if (z == 0 || z == 1)
                zoom = z;
            else
                syslog(LOG_ERR, "%s error: zoom %d not supported, using default (%d)!\n",
                       config->name.c_str(), z, zoom);
        }
        else if (config->options[i].name == "ReportDamage" || config->options[i].name == "Damage" )
        {
            if (config->options[i].value == "none") {
                damage = 0;
            } else if (config->options[i].value == "ugly") {
                damage = 1;
            } else if (config->options[i].value == "udlfb") {
                damage = 2;
            } else if (config->options[i].value == "auto") {
                damage = -1;
            }
            else
                syslog(LOG_ERR, "%s error: ReportDamage='%s' not supported, continuing w/o damage reporting!\n",
                       config->name.c_str(), config->options[i].value.c_str());
        }
    }

    if (config->device == "")
    {
        fbfd = open("/dev/fb0", O_RDWR);
    }
    else
    {
        fbfd = open(config->device.c_str(), O_RDWR);
    }
    
    //fbfd = open("/dev/fb0", O_RDWR);
    if (fbfd < 0)
    {
        syslog(LOG_ERR, "%s: cannot open framebuffer device.\n", config->name.c_str());
        return -1;
    }
    syslog(LOG_INFO, "%s: The framebuffer device was opened successfully.\n", config->name.c_str());

    // Get fixed screen information
    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo))
    {
        syslog(LOG_ERR, "%s: Error reading fixed information.\n", config->name.c_str());
        return -1;
    }

    // Get variable screen information
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo))
    {
        syslog(LOG_ERR, "%s: Error reading variable information.\n", config->name.c_str());
        return -1;
    }

    if ( ! ( vinfo.bits_per_pixel == 8 || vinfo.bits_per_pixel == 16 || vinfo.bits_per_pixel == 24 || vinfo.bits_per_pixel == 32 ) )
    {
        syslog(LOG_ERR, "%s: bpp %d not supported.\n", config->name.c_str(), vinfo.bits_per_pixel);
        return -1;
    }

    // get colour info
    if (vinfo.bits_per_pixel > 8) {
        rlen = vinfo.red.length;
        glen = vinfo.green.length;
        blen = vinfo.blue.length;
        alen = vinfo.transp.length;

        roff = vinfo.red.offset;
        goff = vinfo.green.offset;
        boff = vinfo.blue.offset;
        aoff = vinfo.transp.offset;
    } else {
      // init colour map
      struct fb_cmap cmap;
      uint16_t r[256], g[256], b[256];
      int i;

      // RGB332 code from fbsplash-project taken as guideline
      for ( i = 0; i < 256; i ++ ) {
        r[i] = (( i & 0xe0) <<  8) + ((i & 0x20) ? 0x1fff : 0);
        g[i] = (( i & 0x1c) << 11) + ((i & 0x04) ? 0x1fff : 0);
        b[i] = (( i & 0x03) << 14) + ((i & 0x01) ? 0x3fff : 0);
      }
      cmap.start  = 0;
      cmap.len    = 256;
      cmap.red    = r;
      cmap.green  = g;
      cmap.blue   = b;
      cmap.transp = 0;
      if (ioctl(fbfd, FBIOPUTCMAP, &cmap)) {
        syslog(LOG_ERR, "%s: Error setting colour map for bpp=8.\n", config->name.c_str());
        return -1;
      }
    }

    // Figure out the size of the screen in bytes
    screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;

    syslog(LOG_INFO, "%s: V01: xres: %d, yres %d, vyres: %d, bpp: %d, linelenght: %d\n", config->name.c_str(),vinfo.xres,vinfo.yres,vinfo.yres_virtual,vinfo.bits_per_pixel,finfo.line_length);

    // auto calc w/h depending on zoom
    if (zoom == 1) {
        width = vinfo.xres >> 1;
        height = vinfo.yres >> 1;
    } else {
        width = vinfo.xres;
        height = vinfo.yres;
    }
    depth = vinfo.bits_per_pixel;

    // init bounding box
    bbox[0] = width - 1;  // x top
    bbox[1] = height - 1; // y top
    bbox[2] = 0;          // x bottom
    bbox[3] = 0;          // y bottom
   

    // damage reporting == auto: detect framebuffer driver
    if (damage == -1) {
        if (strncasecmp(finfo.id, "udlfb", 16) == 0) {
           damage = 2; // udlfb
        } else {  /* not supported / not detected */
           damage = 0; // not detected -> no damage reporting
        }
    }

    // reserve another memory to draw into
    offbuff = new char[screensize];
    if (!offbuff)
    {
        syslog(LOG_ERR, "%s: failed to alloc memory for framebuffer device.\n", config->name.c_str());
        return -1;
    }

    // Map the device to memory
    fbp = mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if (fbp == MAP_FAILED)
    {
        syslog(LOG_ERR, "%s: failed to map framebuffer device to memory.\n", config->name.c_str());
        return -1;
    }
    syslog(LOG_INFO, "%s: The framebuffer device was mapped to memory successfully.\n", config->name.c_str());

    *oldConfig = *config;

    // clear display
    Refresh(true);

    syslog(LOG_INFO, "%s: Framebuffer initialized.\n", config->name.c_str());
    return 0;
}

int cDriverFramebuffer::DeInit()
{
    if (offbuff)
        delete[] offbuff;
    munmap(fbp, screensize);
    if (-1 != fbfd)
        close(fbfd);
    return 0;
}

int cDriverFramebuffer::CheckSetup()
{
    if (config->device != oldConfig->device ||
        config->port != oldConfig->port ||
        config->width != oldConfig->width ||
        config->height != oldConfig->height)
    {
        DeInit();
        Init();
        return 0;
    }

    if (config->upsideDown != oldConfig->upsideDown ||
        config->invert != oldConfig->invert)
    {
        oldConfig->upsideDown = config->upsideDown;
        oldConfig->invert = config->invert;
        return 1;
    }
    return 0;
}

void cDriverFramebuffer::SetPixel(int x, int y, uint32_t data)
{
    int location;
    unsigned char col1, col2, col3, alpha = 0;
    uint32_t colraw;
    int changed = 0;

    if (x >= width || y >= height)
        return;

    if (config->upsideDown)
    {
        x = width - 1 - x;
        y = height - 1 - y;
    }


    // Figure out where in memory to put the pixel
    location = ( (x << zoom) + vinfo.xoffset) * (vinfo.bits_per_pixel >> 3) +
               ( (y << zoom) + vinfo.yoffset) * finfo.line_length;

    if (vinfo.bits_per_pixel <= 8)
    {
        col1 = ((data & 0x00FF0000) >> (16 + 5) << 5) |   // RRRg ggbb
               ((data & 0x0000FF00) >> ( 8 + 5) << 2) |   // rrrG GGbb
               ((data & 0x000000FF) >> (     6)     );    // rrrg ggBB
        if ( *(offbuff + location) != col1) {
            changed = 1;
            *(offbuff + location) = col1;
            if (zoom == 1)
            {
                *(offbuff + location + 1) = col1;
                *(offbuff + location + finfo.line_length) = col1;
                *(offbuff + location + finfo.line_length + 1) = col1;
            }
        }
    }
    else if (vinfo.bits_per_pixel <= 16)
    {
        

        colraw = ((data & 0x00FF0000) >> (16 + 8 - rlen) << roff) |   // red
                 ((data & 0x0000FF00) >> ( 8 + 8 - glen) << goff) |   // green
                 ((data & 0x000000FF) >> ( 0 + 8 - blen) << boff);    // blue
        col1 = colraw & 0x0000FF;
        col2 = (colraw & 0x00FF00) >> 8;
        if ( *(offbuff + location) != col1 || *(offbuff + location + 1) != col2 ) {
            changed = 1;
            *(offbuff + location) = col1;
            *(offbuff + location + 1) = col2;
            if (zoom == 1)
            {
                *(offbuff + location + 2) = col1;
                *(offbuff + location + 3) = col2;
                *(offbuff + location + finfo.line_length) = col1;
                *(offbuff + location + finfo.line_length + 1) = col2;
                *(offbuff + location + finfo.line_length + 2) = col1;
                *(offbuff + location + finfo.line_length + 3) = col2;
            }
        }
    }
    else
    {
        // remap graphlcd colour representation to framebuffer rep.
        colraw = ((data & 0x00FF0000) >> (16 + 8 - rlen) << roff) |   // red
                 ((data & 0x0000FF00) >> ( 8 + 8 - glen) << goff) |   // green
                 ((data & 0x000000FF) >> ( 0 + 8 - blen) << boff);    // blue
        col1 =  (colraw & 0x000000FF);
        col2 =  (colraw & 0x0000FF00) >>  8;
        col3 =  (colraw & 0x00FF0000) >> 16;

        if (vinfo.bits_per_pixel == 32) {
          colraw |= ((data & 0xFF000000) >> (24 + 8 - alen) << aoff);    // transp.
          alpha = (colraw & 0xFF000000) >> 24;
        }

        if ( *(offbuff + location) != col1 || *(offbuff + location + 1) != col2 || *(offbuff + location + 2) != col3 ) {
            int pos = 0;
            changed = 1;
            *(offbuff + location + pos++) = col1;
            *(offbuff + location + pos++) = col2;
            *(offbuff + location + pos++) = col3;
            if (vinfo.bits_per_pixel > 24)
                *(offbuff + location + pos++) = alpha;     /* should be transparency */
            if (zoom == 1)
            {
                *(offbuff + location + pos++) = col1;
                *(offbuff + location + pos++) = col2;
                *(offbuff + location + pos++) = col3;
                if (vinfo.bits_per_pixel > 24)
                    *(offbuff + location + pos++) = alpha;
                pos = 0;
                *(offbuff + location + finfo.line_length + pos++) = col1;
                *(offbuff + location + finfo.line_length + pos++) = col2;
                *(offbuff + location + finfo.line_length + pos++) = col3;
                if (vinfo.bits_per_pixel > 24)
                    *(offbuff + location + finfo.line_length + pos++) = alpha;
                *(offbuff + location + finfo.line_length + pos++) = col1;
                *(offbuff + location + finfo.line_length + pos++) = col2;
                *(offbuff + location + finfo.line_length + pos++) = col3;
                if (vinfo.bits_per_pixel > 24)
                    *(offbuff + location + finfo.line_length + pos++) = alpha;
            }
        }
    }

    if (changed) {
        // bounding box changed?
        if (x < bbox[0]) bbox[0] = x;
        if (y < bbox[1]) bbox[1] = y;
        if (x > bbox[2]) bbox[2] = x;
        if (y > bbox[3]) bbox[3] = y;
    }
}

void cDriverFramebuffer::Clear()
{
    memset(offbuff, 0, screensize);
    processDamage();
}

#if 0
void cDriverFramebuffer::Set8Pixels(int x, int y, unsigned char data)
{
    int n;

    x &= 0xFFF8;

    for (n = 0; n < 8; ++n)
    {
        if (data & (0x80 >> n))      // if bit is set
            SetPixel(x + n, y, GRAPHLCD_White);
    }
}
#endif

void cDriverFramebuffer::Refresh(bool refreshAll)
{
    memcpy(fbp, offbuff, screensize);
    processDamage();
}

bool cDriverFramebuffer::GetDriverFeature  (const std::string & Feature, int & value) {
    if (offbuff) {
        if (strcasecmp(Feature.c_str(), "depth") == 0) {
            value = depth;
            return true;
        } else if (strcasecmp(Feature.c_str(), "ismonochrome") == 0) {
            value = 0;
            return true;
        } else if (strcasecmp(Feature.c_str(), "isgreyscale") == 0 || strcasecmp(Feature.c_str(), "isgrayscale") == 0) {
            value = 0;
            return true;
        } else if (strcasecmp(Feature.c_str(), "iscolour") == 0 || strcasecmp(Feature.c_str(), "iscolor") == 0) {
            value = 1;
            return true;
#if 0
        } else if (strcasecmp(Feature.c_str(), "touch") == 0 || strcasecmp(Feature.c_str(), "touchscreen") == 0) {
            if (...) {
                value = (...) ? 1 : 0;
            }
            return true;
#endif
        }
    }
    value = 0;
    return false;
}


/* defines for different damage processing calls needed by _update() */
#define DLFB_IOCTL_REPORT_DAMAGE 0xAA
void cDriverFramebuffer::processDamage (void) {
    switch (damage) {
        case 1: // ugly
            {
                unsigned char buf[3] = "\n";
                write(fbfd,buf,2);
            }
            break;
        case 2: // udlfb
            {
                struct fb_rect {
                    int x; int y; int w; int h;
                } damage = {0, 0, 0, 0};
                damage.x = bbox[0] << zoom;
                damage.y = bbox[1] << zoom;
                damage.w = (bbox[2] - bbox[0] + 1) << zoom;
                damage.h = (bbox[3] - bbox[1] + 1) << zoom;

                ioctl(fbfd, DLFB_IOCTL_REPORT_DAMAGE, &damage);
            }
            break;
        default: // no damage reporting
            break;
    }

    /* reset bounding box */
    bbox[0] = width - 1;
    bbox[1] = height - 1;
    bbox[2] = 0;
    bbox[3] = 0;
}

} // end of namespace
