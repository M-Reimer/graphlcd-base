/*
 * GraphLCD graphics library
 *
 * font.h  -  font handling
 *
 * based on graphlcd plugin 0.1.1 for the Video Disc Recorder
 *  (c) 2001-2004 Carsten Siebholz <c.siebholz AT t-online.de>
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2004 Andreas Regel <andreas.regel AT powarman.de>
 */

#ifndef _GLCDGRAPHICS_FONT_H_
#define _GLCDGRAPHICS_FONT_H_

#include <string>
#include <vector>

#include "bitmap.h"

namespace GLCD
{

class cFont
{
private:
    int totalWidth;
    int totalHeight;
    int totalAscent;
    int spaceBetween;
    int lineHeight;

    cBitmap * characters[256];
protected:
    void Init();
    void Unload();
public:
    cFont();
    ~cFont();

    bool LoadFNT(const std::string & fileName);
    bool SaveFNT(const std::string & fileName) const;
    bool LoadFT2(const std::string & fileName, const std::string & encoding,
                 int size, bool dingBats = false);
    int TotalWidth() const { return totalWidth; };
    int TotalHeight() const { return totalHeight; };
    int TotalAscent() const { return totalAscent; };
    int SpaceBetween() const { return spaceBetween; };
    int LineHeight() const { return lineHeight; };

    void SetTotalWidth(int width) { totalWidth = width; };
    void SetTotalHeight(int height) { totalHeight = height; };
    void SetTotalAscent(int ascent) { totalAscent = ascent; };
    void SetSpaceBetween(int width) { spaceBetween = width; };
    void SetLineHeight(int height) { lineHeight = height; };

    int Width(char ch) const;
    int Width(const std::string & str) const;
    int Width(const std::string & str, unsigned int len) const;
    int Height(char ch) const;
    int Height(const std::string & str) const;
    int Height(const std::string & str, unsigned int len) const;

    const cBitmap * GetCharacter(char ch) const;
    void SetCharacter(char ch, cBitmap * bitmapChar);

    void WrapText(int Width, int Height, std::string & Text,
                  std::vector <std::string> & Lines, int * TextWidth = NULL) const;
};

} // end of namespace

#endif
