/*
 * GraphLCD graphics library
 *
 * image.h  -  image and animation handling
 *
 * based on graphlcd plugin 0.1.1 for the Video Disc Recorder
 *  (c) 2001-2004 Carsten Siebholz <c.siebholz AT t-online.de>
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2004 Andreas Regel <andreas.regel AT powarman.de>
 */

#ifndef _GLCDGRAPHICS_IMAGE_H_
#define _GLCDGRAPHICS_IMAGE_H_

#include <stdint.h>

#include <vector>

namespace GLCD
{

class cBitmap;

class cImage
{
private:
    unsigned int width;
    unsigned int height;
    unsigned int delay;
    unsigned int curBitmap;
    uint64_t lastChange;
    std::vector <cBitmap *> bitmaps;
public:
    cImage();
    ~cImage();

    unsigned int Width() const { return width; }
    unsigned int Height() const { return height; }
    unsigned int Count() const { return bitmaps.size(); }
    unsigned int Delay() const { return delay; }
    uint64_t LastChange() const { return lastChange; }
    void First(uint64_t t) { lastChange = t; curBitmap = 0; }
    bool Next(uint64_t t) { lastChange = t; curBitmap++; return curBitmap < bitmaps.size(); }
    void SetWidth(unsigned int Width) { width = Width; }
    void SetHeight(unsigned int Height) { height = Height; }
    void SetDelay(unsigned int d) { delay = d; }
    cBitmap * GetBitmap(unsigned int nr) const;
    cBitmap * GetBitmap() const;
    void AddBitmap(cBitmap * Bitmap) { bitmaps.push_back(Bitmap); }
    void Clear();
};

} // end of namespace

#endif
