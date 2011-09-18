/*
 * GraphLCD graphics library
 *
 * bitmap.h  -  cBitmap class
 *
 * based on graphlcd plugin 0.1.1 for the Video Disc Recorder
 *  (c) 2001-2004 Carsten Siebholz <c.siebholz AT t-online.de>
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2004-2010 Andreas Regel <andreas.regel AT powarman.de>
 * (c) 2010-2011 Wolfgang Astleitner <mrwastl AT users sourceforge net>
 *               Andreas 'randy' Weinberger 
 */

#ifndef _GLCDGRAPHICS_BITMAP_H_
#define _GLCDGRAPHICS_BITMAP_H_

#include <string>
#include <inttypes.h>

// graphlcd-base uses ARGB bitmaps instead of 1bit ones
// this flag will be checked in current versions of vdr-graphlcd-plugin
#define GRAPHLCD_CBITMAP_ARGB

// colour definitions for glcddrivers so that libglcddrivers.so isn't link-dependent on libglcdgraphics.so
#define GRAPHLCD_Black       0xFF000000
#define GRAPHLCD_White       0xFFFFFFFF
#define GRAPHLCD_Transparent 0x00FFFFFF
#define GRAPHLCD_ERRCOL      0x00000000


namespace GLCD
{

#if 0
enum eColor
{
   clrTransparent,
   clrGray50,
   clrBlack,
   clrRed,
   clrGreen,
   clrYellow,
   clrMagenta,
   clrBlue,
   clrCyan,
   clrWhite                  
};
#endif


class cColor
{
protected:
  uint32_t color;

public:
   cColor(uint32_t col) { color = col; }
   cColor(const cColor & col) { color = col.color; }

   static const uint32_t Black;
   static const uint32_t White;
   static const uint32_t Red;
   static const uint32_t Green;
   static const uint32_t Blue;
   static const uint32_t Magenta;
   static const uint32_t Cyan;
   static const uint32_t Yellow;
   static const uint32_t Transparent;
   static const uint32_t ERRCOL;

   uint32_t GetColor (void)         { return color; }
   void     SetColor (uint32_t col) { color = col; }

   cColor Invert   (void);

   operator uint32_t(void) { return GetColor(); }

   static cColor ParseColor (std::string col);
   static uint32_t AlignAlpha  (uint32_t col);
};


class cFont;

class cBitmap
{
protected:
    int width;
    int height;
    int lineSize;
    uint32_t * bitmap;
    bool ismonochrome;
    bool processAlpha;

    uint32_t backgroundColor;

public:
    cBitmap(int width, int height, uint32_t * data = NULL);
    cBitmap(int width, int height, uint32_t initcol);
    cBitmap(const cBitmap & b);
    ~cBitmap();

    int Width() const { return width; }
    int Height() const { return height; }
    int LineSize() const { return lineSize; }
    const uint32_t * Data() const { return bitmap; }

    void Clear(uint32_t color = cColor::Transparent);
    void Invert();
    void DrawPixel(int x, int y, uint32_t color);
    void DrawLine(int x1, int y1, int x2, int y2, uint32_t color);
    void DrawHLine(int x1, int y, int x2, uint32_t color);
    void DrawVLine(int x, int y1, int y2, uint32_t color);
    void DrawRectangle(int x1, int y1, int x2, int y2, uint32_t color, bool filled);
    void DrawRoundRectangle(int x1, int y1, int x2, int y2, uint32_t color, bool filled, int size);
    void DrawEllipse(int x1, int y1, int x2, int y2, uint32_t color, bool filled, int quadrants);
    void DrawSlope(int x1, int y1, int x2, int y2, uint32_t color, int type);
    void DrawBitmap(int x, int y, const cBitmap & bitmap, uint32_t color = cColor::White, uint32_t bgcolor = cColor::Black, int opacity = 255);
    int DrawText(int x, int y, int xmax, const std::string & text, const cFont * font,
                 uint32_t color = cColor::White, uint32_t bgcolor = cColor::Black, bool proportional = true, int skipPixels = 0);
    int DrawCharacter(int x, int y, int xmax, uint32_t c, const cFont * font,
                      uint32_t color = cColor::White, uint32_t bgcolor = cColor::Black, int skipPixels = 0);

    cBitmap * SubBitmap(int x1, int y1, int x2, int y2) const;
    uint32_t GetPixel(int x, int y) const;

    void SetMonochrome(bool mono) { ismonochrome = mono; }
    bool IsMonochrome(void) const { return ismonochrome; }

    void SetProcessAlpha(bool procAlpha) { processAlpha = procAlpha; }
    bool IsProcessAlpha(void) const { return processAlpha; }
    
    static const unsigned char* ConvertTo1BPP(const cBitmap & bitmap, int threshold = 127);
    static const cBitmap* ConvertFrom1BPP(const unsigned char* monobmp, int w, int h, uint32_t fg = cColor::White, uint32_t bg = cColor::Black);

#if 0
    bool LoadPBM(const std::string & fileName);
    void SavePBM(const std::string & fileName);
#endif
};

} // end of namespace

#endif
