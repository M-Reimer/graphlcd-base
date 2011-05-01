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
:   config(config),
    offbuff(0),
    fbfd(-1)
{
    oldConfig = new cDriverConfig(*config);
}

cDriverFramebuffer::~cDriverFramebuffer()
{
    delete oldConfig;
}

int cDriverFramebuffer::Init()
{
    // default values
    width = config->width;
    if (width <= 0)
        width = 320;
    height = config->height;
    if (height <= 0)
        height = 240;
    zoom = 1;

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
    }

    // Open the file for reading and writing
    fbfd = open("/dev/fb0", O_RDWR);
    if (1 == fbfd)
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

    // Figure out the size of the screen in bytes
    screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;

    syslog(LOG_INFO, "%s: V01: xres: %d, yres %d, vyres: %d, bpp: %d, linelenght: %d\n", config->name.c_str(),vinfo.xres,vinfo.yres,vinfo.yres_virtual,vinfo.bits_per_pixel,finfo.line_length);

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
    int outcol;

    if (x >= width || y >= height)
        return;

    if (config->upsideDown)
    {
        x = width - 1 - x;
        y = height - 1 - y;
    }

    // Figure out where in memory to put the pixel
    location = (x*(1+zoom)+vinfo.xoffset) * (vinfo.bits_per_pixel/8) +
               (y*(1+zoom)+vinfo.yoffset) * finfo.line_length;

    if (vinfo.bits_per_pixel <= 8)
    {
        outcol = 15;
    }
    else
    {
        outcol = 255;
    }

    if (vinfo.bits_per_pixel <= 8)
    {
        *(offbuff + location) = outcol;
        if (zoom == 1)
        {
            *(offbuff + location + 1) = outcol;
            *(offbuff + location + finfo.line_length) = outcol;
            *(offbuff + location + finfo.line_length + 1) = outcol;
        }
    }
    else if (vinfo.bits_per_pixel <= 16)
    {
        *(offbuff + location) = outcol;
        *(offbuff + location + 1) = outcol;
        if (zoom == 1)
        {
            *(offbuff + location + 2) = outcol;
            *(offbuff + location + 3) = outcol;
            *(offbuff + location + finfo.line_length) = outcol;
            *(offbuff + location + finfo.line_length + 1) = outcol;
            *(offbuff + location + finfo.line_length + 2) = outcol;
            *(offbuff + location + finfo.line_length + 3) = outcol;
        }
    }
    else
    {
        *(offbuff + location) = outcol;
        *(offbuff + location + 1) = outcol;
        *(offbuff + location + 2) = outcol;
        *(offbuff + location + 3) = 0;     /* should be transparency */
        if (zoom == 1)
        {
            *(offbuff + location + 4) = outcol;
            *(offbuff + location + 5) = outcol;
            *(offbuff + location + 6) = outcol;
            *(offbuff + location + 7) = 0;
            *(offbuff + location + finfo.line_length) = outcol;
            *(offbuff + location + finfo.line_length + 1) = outcol;
            *(offbuff + location + finfo.line_length + 2) = outcol;
            *(offbuff + location + finfo.line_length + 3) = 0;
            *(offbuff + location + finfo.line_length + 4) = outcol;
            *(offbuff + location + finfo.line_length + 5) = outcol;
            *(offbuff + location + finfo.line_length + 6) = outcol;
            *(offbuff + location + finfo.line_length + 7) = 0;
        }
    }
}

void cDriverFramebuffer::Clear()
{
    memset(offbuff, 0, screensize);
}

void cDriverFramebuffer::Set8Pixels(int x, int y, unsigned char data)
{
    int n;

    x &= 0xFFF8;

    for (n = 0; n < 8; ++n)
    {
        if (data & (0x80 >> n))      // if bit is set
            SetPixel(x + n, y, GLCD::cColor::White);
    }
}

void cDriverFramebuffer::Refresh(bool refreshAll)
{
    memcpy(fbp, offbuff, screensize);
}

} // end of namespace
