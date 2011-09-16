/*
 * GraphLCD driver library
 *
 * driver.c  -  driver base class
 *
 * parts were taken from graphlcd plugin for the Video Disc Recorder
 * (c) 2001-2004 Carsten Siebholz <c.siebholz AT t-online.de>
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2004-2010 Andreas Regel <andreas.regel AT powarman.de>
 * (c) 2010-2011 Wolfgang Astleitner <mrwastl AT users sourceforge net>
 */

#include "common.h"
#include "driver.h"
#include "config.h"

namespace GLCD
{

cSimpleTouchEvent::cSimpleTouchEvent() : x(0), y(0), touch(0)
{
}

cDriver::cDriver(cDriverConfig * config)
:   width(0),
    height(0),
    config(config)
{
    fgcol = GetDefaultForegroundColor();
    bgcol = GetDefaultBackgroundColor();
    oldConfig = new cDriverConfig(*config);
}

cDriver::~cDriver(void)
{
    delete oldConfig;
}

const std::string cDriver::ConfigName() {
  return (config) ? config->name : "";
}

const std::string cDriver::DriverName() {
  return (config) ? config->driver : "";
}


//void cDriver::SetScreen(const unsigned char * data, int wid, int hgt, int lineSize)
void cDriver::SetScreen(const uint32_t * data, int wid, int hgt)
{
    int x, y;

    if (wid > width)
        wid = width;
    if (hgt > height)
        hgt = height;

    //Clear();
    if (data)
    {
        for (y = 0; y < hgt; y++)
        {
          for (x = 0; x < wid; x++)
          {
//            printf("%s:%s(%d) - %03d * %03d (linesize %02d), %08x\n", __FILE__, __FUNCTION__, __LINE__, x, y, lineSize, data[y * lineSize + x]);
            SetPixel(x, y, data[y * wid + x]);
          }
        }
    }
/*
            for (x = 0; x < (wid / 8); x++)
            {
                Set8Pixels(x * 8, y, data[y * lineSize + x]);
            }
            if (width % 8)
            {
                Set8Pixels((wid / 8) * 8, y, data[y * lineSize + wid / 8] & bitmaskl[wid % 8 - 1]);
            }
        }
*/
}

void cDriver::Set8Pixels(int x, int y, unsigned char data)
{
    int n;
    // calling GetForegroundColor() and GetBackgroundColor() is slow in some situations.
    // will be replaced through setting object-wide (incl. derived objs) class members
    uint32_t fg = GetForegroundColor();
    uint32_t bg = GetBackgroundColor();
    
    // guarante that x starts at a position divisible by 8
    x &= 0xFFF8;

    for (n = 0; n < 8; ++n) {
        SetPixel(x + n, y, (data & (0x80 >> n)) ? fg : bg);
    }
}


} // end of namespace
