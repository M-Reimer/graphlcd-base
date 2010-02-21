/*
 * GraphLCD graphics library
 *
 * image.c  -  image and animation handling
 *
 * based on graphlcd plugin 0.1.1 for the Video Disc Recorder
 *  (c) 2001-2004 Carsten Siebholz <c.siebholz AT t-online.de>
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2004 Andreas Regel <andreas.regel AT powarman.de>
 */

#include "bitmap.h"
#include "image.h"


namespace GLCD
{

using namespace std;

cImage::cImage()
:   width(0),
    height(0),
    delay(0),
    curBitmap(0),
    lastChange(0)
{
}

cImage::~cImage()
{
    Clear();
}

cBitmap * cImage::GetBitmap() const
{
    if (curBitmap < bitmaps.size())
        return bitmaps[curBitmap];
    return NULL;
}

cBitmap * cImage::GetBitmap(unsigned int nr) const
{
    if (nr < bitmaps.size())
        return bitmaps[nr];
    return NULL;
}

void cImage::Clear()
{
    vector <cBitmap *>::iterator it;
    for (it = bitmaps.begin(); it != bitmaps.end(); it++)
    {
        delete *it;
    }
    bitmaps.clear();
    width = 0;
    height = 0;
    delay = 0;
    curBitmap = 0;
    lastChange = 0;
}

} // end of namespace
