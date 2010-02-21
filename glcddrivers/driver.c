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
 * (c) 2004 Andreas Regel <andreas.regel AT powarman.de>
 */

#include "common.h"
#include "driver.h"


namespace GLCD
{

cDriver::cDriver()
:   width(0),
    height(0)
{
}

void cDriver::SetScreen(const unsigned char * data, int wid, int hgt, int lineSize)
{
    int x, y;

    if (wid > width)
        wid = width;
    if (hgt > height)
        hgt = height;

    Clear();
    if (data)
    {
        for (y = 0; y < hgt; y++)
        {
            for (x = 0; x < (wid / 8); x++)
            {
                Set8Pixels(x * 8, y, data[y * lineSize + x]);
            }
            if (width % 8)
            {
                Set8Pixels((wid / 8) * 8, y, data[y * lineSize + wid / 8] & bitmaskl[wid % 8 - 1]);
            }
        }
    }
}

} // end of namespace
