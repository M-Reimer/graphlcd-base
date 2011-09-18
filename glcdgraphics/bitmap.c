/*
 * GraphLCD graphics library
 *
 * bitmap.c  -  cBitmap class
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "bitmap.h"
#include "common.h"
#include "font.h"


namespace GLCD
{

const uint32_t cColor::Black       = GRAPHLCD_Black;
const uint32_t cColor::White       = GRAPHLCD_White;
const uint32_t cColor::Red         = 0xFFFF0000;
const uint32_t cColor::Green       = 0xFF00FF00;
const uint32_t cColor::Blue        = 0xFF0000FF;
const uint32_t cColor::Magenta     = 0xFFFF00FF;
const uint32_t cColor::Cyan        = 0xFF00FFFF;
const uint32_t cColor::Yellow      = 0xFFFFFF00;
const uint32_t cColor::Transparent = GRAPHLCD_Transparent;
const uint32_t cColor::ERRCOL      = GRAPHLCD_ERRCOL;


cColor cColor::ParseColor(std::string col) {
    if (col == "black")            return cColor(cColor::Black);
    else if (col == "white")       return cColor(cColor::White);
    else if (col == "red")         return cColor(cColor::Red);
    else if (col == "green")       return cColor(cColor::Green);
    else if (col == "blue")        return cColor(cColor::Blue);
    else if (col == "magenta")     return cColor(cColor::Magenta);
    else if (col == "cyan")        return cColor(cColor::Cyan);
    else if (col == "yellow")      return cColor(cColor::Yellow);
    else if (col == "transparent") return cColor(cColor::Transparent);
    else if (col.substr(0, 2) == "0x" || col.substr(0, 2) == "0X") {
        if (col.length() <= 2 || col.length() > 10)
            return cColor(cColor::ERRCOL);

        char* tempptr;
        const char* str = col.c_str();
        uint32_t rv = (uint32_t) strtoul(str, &tempptr, 16);

        if ((str == tempptr) || (*tempptr != '\0'))
            return cColor(cColor::ERRCOL);

        if (col.length() <= 8) // eg. 0xRRGGBB -> 0xFFRRGGBB
            rv |= 0xFF000000;
        return cColor(rv);
    }
    return cColor(cColor::ERRCOL);
}

cColor cColor::Invert(void)
{
    return cColor( (uint32_t)(color ^ 0x00FFFFFF) ) ;
}

uint32_t cColor::AlignAlpha  (uint32_t col) { 
    switch (col) {
        case Transparent:
        case ERRCOL:
            return col;
        default:
            return (col & 0xFF000000) ? col : (col | 0xFF000000);
    }
}

const unsigned char bitmask[8]  = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
const unsigned char bitmaskl[8] = {0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff};
const unsigned char bitmaskr[8] = {0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01};

cBitmap::cBitmap(int width, int height, uint32_t * data)
:   width(width),
    height(height),
    bitmap(NULL),
    ismonochrome(false),
    processAlpha(true)
{
#ifdef DEBUG
    printf("%s:%s(%d) cBitmap Size %03d * %03d\n", __FILE__, __FUNCTION__, __LINE__, width, height);
#endif
    if (width > 0 && height > 0) {
        bitmap = new uint32_t[width * height];
        if (data && bitmap) {
            memcpy(bitmap, data, width * height * sizeof(uint32_t));
        }
    }
    backgroundColor = cColor::White;
}


cBitmap::cBitmap(int width, int height, uint32_t initcol)
:   width(width),
    height(height),
    bitmap(NULL),
    ismonochrome(false),
    processAlpha(true)
{
#ifdef DEBUG
    printf("%s:%s(%d) cBitmap Size %03d * %03d\n", __FILE__, __FUNCTION__, __LINE__, width, height);
#endif

    if (width > 0 && height > 0) {
        bitmap = new uint32_t[width * height];
        Clear(initcol);
    }
}

cBitmap::cBitmap(const cBitmap & b)
{
    width = b.width;
    height = b.height;
    lineSize = b.lineSize;
    backgroundColor = b.backgroundColor;
    ismonochrome = b.ismonochrome;
    processAlpha = b.processAlpha;
    bitmap = new uint32_t[b.width * b.height];
    if (b.bitmap && bitmap) {
        memcpy(bitmap, b.bitmap, b.width * b.height * sizeof(uint32_t));
    }
}

cBitmap::~cBitmap()
{
    if (bitmap)
        delete[] bitmap;
    bitmap = NULL;
}

void cBitmap::Clear(uint32_t color)
{
#ifdef DEBUG
    printf("%s:%s(%d) %03d * %03d (color %08x)\n", __FILE__, __FUNCTION__, __LINE__, width, height, color);
#endif
    //uint32_t col = initcol; //(initcol == cColor::Transparent) ? backgroundColor : initcol;

    // force clearing (== background) colour to contain alpha level = 0xFF
    if ( color != cColor::Transparent )
        color = (color & 0x00FFFFFF) | 0xFF000000;

    for (int i = 0; i < width * height; i++)
        bitmap[i] = color;
    backgroundColor = color;
}

void cBitmap::Invert()
{
    int i;

    for (i = 0; i < width * height; i++)
    {
        bitmap[i] ^= 0xFFFFFF;
    }
}

void cBitmap::DrawPixel(int x, int y, uint32_t color)
{
    if (x < 0 || x > width - 1)
        return;
    if (y < 0 || y > height - 1)
        return;

    if (color != GLCD::cColor::Transparent) {
        uint32_t col = cColor::AlignAlpha(color);
        if (processAlpha) {
            uint32_t bg = bitmap[x + (width * y)];
            uint32_t afg = (col & 0xFF000000) >> 24;
            uint32_t rfg = (col & 0x00FF0000) >> 16;
            uint32_t gfg = (col & 0x0000FF00) >> 8;
            uint32_t bfg = (col & 0x000000FF);

            uint32_t rbg = (bg & 0x00FF0000) >> 16;
            uint32_t gbg = (bg & 0x0000FF00) >> 8;
            uint32_t bbg = (bg & 0x000000FF);
            
            // calculate colour channels of new colour depending on alpha channel and background colour
            rfg = (rfg * afg + rbg * (255 - afg)) / 255;
            gfg = (gfg * afg + gbg * (255 - afg)) / 255;
            bfg = (bfg * afg + bbg * (255 - afg)) / 255;
            
            // as we draw from bottom to top, the new colour will always have alpha level == 0xFF
            // (it will serve as background colour for future objects that will be drawn onto the current object)
            col = 0xFF000000 | (rfg << 16) | (gfg << 8) | bfg;
        }
        bitmap[x + (width * y)] = col;
    }
    //else
    //    bitmap[x + (width * y)] = cColor::AlignAlpha(backgroundColor);
}


void cBitmap::DrawLine(int x1, int y1, int x2, int y2, uint32_t color)
{
    int d, sx, sy, dx, dy;
    unsigned int ax, ay;

    color = cColor::AlignAlpha(color);

    dx = x2 - x1;
    ax = abs(dx) << 1;
    if (dx < 0)
        sx = -1;
    else
        sx = 1;

    dy = y2 - y1;
    ay = abs(dy) << 1;
    if (dy < 0)
        sy = -1;
    else
        sy = 1;

    DrawPixel(x1, y1, color);
    if (ax > ay)
    {
        d = ay - (ax >> 1);
        while (x1 != x2)
        {
            if (d >= 0)
            {
                y1 += sy;
                d -= ax;
            }
            x1 += sx;
            d += ay;
            DrawPixel(x1, y1, color);
        }
    }
    else
    {
        d = ax - (ay >> 1);
        while (y1 != y2)
        {
            if (d >= 0)
            {
                x1 += sx;
                d -= ay;
            }
            y1 += sy;
            d += ax;
            DrawPixel(x1, y1, color);
        }
    }
}

void cBitmap::DrawHLine(int x1, int y, int x2, uint32_t color)
{
#ifdef DEBUG
    printf("%s:%s(%d) %03d -> %03d, %03d (color %08x)\n", __FILE__, __FUNCTION__, __LINE__, x1, x2, y, color);
#endif
    color = cColor::AlignAlpha(color);

    sort(x1,x2);
    while (x1 <= x2) {
      DrawPixel(x1, y, color);
      x1++;
    };
}

void cBitmap::DrawVLine(int x, int y1, int y2, uint32_t color)
{
#ifdef DEBUG
    printf("%s:%s(%d) %03d, %03d -> %03d (color %08x)\n", __FILE__, __FUNCTION__, __LINE__, x, y1, y2, color);
#endif
    color = cColor::AlignAlpha(color);

    sort(y1,y2);
    while (y1 <= y2) {
      DrawPixel(x, y1, color);
      y1++;
    }
}

void cBitmap::DrawRectangle(int x1, int y1, int x2, int y2, uint32_t color, bool filled)
{
#ifdef DEBUG
    printf("%s:%s(%d) %03d * %03d -> %03d * %03d (color %08x)\n", __FILE__, __FUNCTION__, __LINE__, x1, y1, x2, y2, color);
#endif
    int y;

    color = cColor::AlignAlpha(color);

    sort(x1,x2);
    sort(y1,y2);

    if (!filled)
    {
        DrawHLine(x1, y1, x2, color);
        DrawVLine(x1, y1, y2, color);
        DrawHLine(x1, y2, x2, color);
        DrawVLine(x2, y1, y2, color);
    }
    else
    {
        for (y = y1; y <= y2; y++)
        {
            DrawHLine(x1, y, x2, color);
        }
    }
}

void cBitmap::DrawRoundRectangle(int x1, int y1, int x2, int y2, uint32_t color, bool filled, int type)
{
#ifdef DEBUG
    printf("%s:%s(%d) %03d * %03d -> %03d * %03d (color %08x)\n", __FILE__, __FUNCTION__, __LINE__, x1, y1, x2, y2, color);
#endif
    color = cColor::AlignAlpha(color);

    sort(x1,x2);
    sort(y1,y2);

    if (type > (x2 - x1) / 2)
        type = (x2 - x1) / 2;
    if (type > (y2 - y1) / 2)
        type = (y2 - y1) / 2;

    if (filled)
    {
        DrawHLine(x1 + type, y1, x2 - type, color);
        for (int y = y1 + 1; y < y1 + type; y++)
            DrawHLine(x1 + 1, y, x2 - 1, color);
        for (int y = y1 + type; y <= y2 - type; y++)
            DrawHLine(x1, y, x2, color);
        for (int y = y2 - type + 1; y < y2; y++)
            DrawHLine(x1 + 1, y, x2 - 1, color);
        DrawHLine(x1 + type, y2, x2 - type, color);
        if (type == 4)
        {
            // round the ugly fat box...
//            DrawPixel(x1 + 1, y1 + 1, color == clrWhite ? clrBlack : clrWhite);
//            DrawPixel(x1 + 1, y2 - 1, color == clrWhite ? clrBlack : clrWhite);
//            DrawPixel(x2 - 1, y1 + 1, color == clrWhite ? clrBlack : clrWhite);
//            DrawPixel(x2 - 1, y2 - 1, color == clrWhite ? clrBlack : clrWhite);
            DrawPixel(x1 + 1, y1 + 1, backgroundColor);
            DrawPixel(x1 + 1, y2 - 1, backgroundColor);
            DrawPixel(x2 - 1, y1 + 1, backgroundColor);
            DrawPixel(x2 - 1, y2 - 1, backgroundColor);
        }
    }
    else
    {
        DrawHLine(x1 + type, y1, x2 - type, color);
        DrawVLine(x1, y1 + type, y2 - type, color);
        DrawVLine(x2, y1 + type, y2 - type, color);
        DrawHLine(x1 + type, y2, x2 - type, color);
        if (type > 1)
        {
            DrawHLine(x1 + 1, y1 + 1, x1 + type - 1, color);
            DrawHLine(x2 - type + 1, y1 + 1, x2 - 1, color);
            DrawHLine(x1 + 1, y2 - 1, x1 + type - 1, color);
            DrawHLine(x2 - type + 1, y2 - 1, x2 - 1, color);
            DrawVLine(x1 + 1, y1 + 1, y1 + type - 1, color);
            DrawVLine(x1 + 1, y2 - 1, y2 - type + 1, color);
            DrawVLine(x2 - 1, y1 + 1, y1 + type - 1, color);
            DrawVLine(x2 - 1, y2 - 1, y2 - type + 1, color);
        }
    }
}

void cBitmap::DrawEllipse(int x1, int y1, int x2, int y2, uint32_t color, bool filled, int quadrants)
{
#ifdef DEBUG
    printf("%s:%s(%d) %03d * %03d -> %03d * %03d (color %08x)\n", __FILE__, __FUNCTION__, __LINE__, x1, y1, x2, y2, color);
#endif
    color = cColor::AlignAlpha(color);

    // Algorithm based on http://homepage.smc.edu/kennedy_john/BELIPSE.PDF
    int rx = x2 - x1;
    int ry = y2 - y1;
    int cx = (x1 + x2) / 2;
    int cy = (y1 + y2) / 2;
    switch (abs(quadrants))
    {
        case 0: rx /= 2; ry /= 2; break;
        case 1: cx = x1; cy = y2; break;
        case 2: cx = x2; cy = y2; break;
        case 3: cx = x2; cy = y1; break;
        case 4: cx = x1; cy = y1; break;
        case 5: cx = x1;          ry /= 2; break;
        case 6: cy = y2; rx /= 2; break;
        case 7: cx = x2;          ry /= 2; break;
        case 8: cy = y1; rx /= 2; break;
    }
    int TwoASquare = 2 * rx * rx;
    int TwoBSquare = 2 * ry * ry;
    int x = rx;
    int y = 0;
    int XChange = ry * ry * (1 - 2 * rx);
    int YChange = rx * rx;
    int EllipseError = 0;
    int StoppingX = TwoBSquare * rx;
    int StoppingY = 0;
    while (StoppingX >= StoppingY)
    {
        if (filled)
        {
            switch (quadrants)
            {
                case  5: DrawRectangle(cx,     cy + y, cx + x, cy + y, color, filled); // no break
                case  1: DrawRectangle(cx,     cy - y, cx + x, cy - y, color, filled); break;
                case  7: DrawRectangle(cx - x, cy + y, cx,     cy + y, color, filled); // no break
                case  2: DrawRectangle(cx - x, cy - y, cx,     cy - y, color, filled); break;
                case  3: DrawRectangle(cx - x, cy + y, cx,     cy + y, color, filled); break;
                case  4: DrawRectangle(cx,     cy + y, cx + x, cy + y, color, filled); break;
                case  0:
                case  6: DrawRectangle(cx - x, cy - y, cx + x, cy - y, color, filled); if (quadrants == 6) break;
                case  8: DrawRectangle(cx - x, cy + y, cx + x, cy + y, color, filled); break;
                case -1: DrawRectangle(cx + x, cy - y, x2,     cy - y, color, filled); break;
                case -2: DrawRectangle(x1,     cy - y, cx - x, cy - y, color, filled); break;
                case -3: DrawRectangle(x1,     cy + y, cx - x, cy + y, color, filled); break;
                case -4: DrawRectangle(cx + x, cy + y, x2,     cy + y, color, filled); break;
            }
        }
        else
        {
            switch (quadrants)
            {
                case  5: DrawPixel(cx + x, cy + y, color); // no break
                case -1:
                case  1: DrawPixel(cx + x, cy - y, color); break;
                case  7: DrawPixel(cx - x, cy + y, color); // no break
                case -2:
                case  2: DrawPixel(cx - x, cy - y, color); break;
                case -3:
                case  3: DrawPixel(cx - x, cy + y, color); break;
                case -4:
                case  4: DrawPixel(cx + x, cy + y, color); break;
                case  0:
                case  6: DrawPixel(cx - x, cy - y, color); DrawPixel(cx + x, cy - y, color); if (quadrants == 6) break;
                case  8: DrawPixel(cx - x, cy + y, color); DrawPixel(cx + x, cy + y, color); break;
            }
        }
        y++;
        StoppingY += TwoASquare;
        EllipseError += YChange;
        YChange += TwoASquare;
        if (2 * EllipseError + XChange > 0)
        {
            x--;
            StoppingX -= TwoBSquare;
            EllipseError += XChange;
            XChange += TwoBSquare;
        }
    }
    x = 0;
    y = ry;
    XChange = ry * ry;
    YChange = rx * rx * (1 - 2 * ry);
    EllipseError = 0;
    StoppingX = 0;
    StoppingY = TwoASquare * ry;
    while (StoppingX <= StoppingY)
    {
        if (filled)
        {
            switch (quadrants)
            {
                case  5: DrawRectangle(cx,     cy + y, cx + x, cy + y, color, filled); // no break
                case  1: DrawRectangle(cx,     cy - y, cx + x, cy - y, color, filled); break;
                case  7: DrawRectangle(cx - x, cy + y, cx,     cy + y, color, filled); // no break
                case  2: DrawRectangle(cx - x, cy - y, cx,     cy - y, color, filled); break;
                case  3: DrawRectangle(cx - x, cy + y, cx,     cy + y, color, filled); break;
                case  4: DrawRectangle(cx,     cy + y, cx + x, cy + y, color, filled); break;
                case  0:
                case  6: DrawRectangle(cx - x, cy - y, cx + x, cy - y, color, filled); if (quadrants == 6) break;
                case  8: DrawRectangle(cx - x, cy + y, cx + x, cy + y, color, filled); break;
                case -1: DrawRectangle(cx + x, cy - y, x2,     cy - y, color, filled); break;
                case -2: DrawRectangle(x1,     cy - y, cx - x, cy - y, color, filled); break;
                case -3: DrawRectangle(x1,     cy + y, cx - x, cy + y, color, filled); break;
                case -4: DrawRectangle(cx + x, cy + y, x2,     cy + y, color, filled); break;
            }
        }
        else
        {
            switch (quadrants)
            {
                case  5: DrawPixel(cx + x, cy + y, color); // no break
                case -1:
                case  1: DrawPixel(cx + x, cy - y, color); break;
                case  7: DrawPixel(cx - x, cy + y, color); // no break
                case -2:
                case  2: DrawPixel(cx - x, cy - y, color); break;
                case -3:
                case  3: DrawPixel(cx - x, cy + y, color); break;
                case -4:
                case  4: DrawPixel(cx + x, cy + y, color); break;
                case  0:
                case  6: DrawPixel(cx - x, cy - y, color); DrawPixel(cx + x, cy - y, color); if (quadrants == 6) break;
                case  8: DrawPixel(cx - x, cy + y, color); DrawPixel(cx + x, cy + y, color); break;
            }
        }
        x++;
        StoppingX += TwoBSquare;
        EllipseError += XChange;
        XChange += TwoBSquare;
        if (2 * EllipseError + YChange > 0)
        {
            y--;
            StoppingY -= TwoASquare;
            EllipseError += YChange;
            YChange += TwoASquare;
        }
    }
}

void cBitmap::DrawSlope(int x1, int y1, int x2, int y2, uint32_t color, int type)
{
#ifdef DEBUG
    printf("%s:%s(%d) %03d * %03d -> %03d * %03d\n", __FILE__, __FUNCTION__, __LINE__, x1, y1, x2, y2);
#endif
    color = cColor::AlignAlpha(color);

    bool upper    = type & 0x01;
    bool falling  = type & 0x02;
    bool vertical = type & 0x04;
    if (vertical)
    {
        for (int y = y1; y <= y2; y++)
        {
            double c = cos((y - y1) * M_PI / (y2 - y1 + 1));
            if (falling)
                c = -c;
            int x = int((x2 - x1 + 1) * c / 2);
            if ((upper && !falling) || (!upper && falling))
                DrawRectangle(x1, y, (x1 + x2) / 2 + x, y, color, true);
            else
                DrawRectangle((x1 + x2) / 2 + x, y, x2, y, color, true);
        }
    }
    else
    {
        for (int x = x1; x <= x2; x++)
        {
            double c = cos((x - x1) * M_PI / (x2 - x1 + 1));
            if (falling)
                c = -c;
            int y = int((y2 - y1 + 1) * c / 2);
            if (upper)
                DrawRectangle(x, y1, x, (y1 + y2) / 2 + y, color, true);
            else
                DrawRectangle(x, (y1 + y2) / 2 + y, x, y2, color, true);
        }
    }
}

void cBitmap::DrawBitmap(int x, int y, const cBitmap & bitmap, uint32_t color, uint32_t bgcolor, int opacity)
{
#ifdef DEBUG
    printf("%s:%s(%d) '%03d' x '%03d' \n", __FILE__, __FUNCTION__, __LINE__, x, y);
#endif
    color = cColor::AlignAlpha(color);
    bgcolor = cColor::AlignAlpha(bgcolor);

    uint32_t cl = 0;
    const uint32_t * data = bitmap.Data();
    bool ismonochrome = bitmap.IsMonochrome();

    int xt, yt;
    uint32_t alpha;

    if (data)
    {
      for (yt = 0; yt < bitmap.Height(); yt++)
        {
          for (xt = 0; xt < bitmap.Width(); xt++)
          {
            cl = data[(yt * bitmap.Width())+xt];
            if (cl != cColor::Transparent) {
              if (ismonochrome) {
                cl = (cl == cColor::Black) ? color : bgcolor;
              }
              if (opacity != 255) {
                  alpha = (cl & 0xFF000000) >> 24;
                  alpha = (alpha * opacity) / 255;
                  cl = (cl & 0x00FFFFFF) | (alpha << 24);
              }
              DrawPixel(xt+x, yt+y, cl);
           }
         }
       }
    }
}

int cBitmap::DrawText(int x, int y, int xmax, const std::string & text, const cFont * font,
                      uint32_t color, uint32_t bgcolor, bool proportional, int skipPixels)
{
#ifdef DEBUG
    printf("%s:%s(%d) text '%s', color '%08x'/'%08x'\n", __FILE__, __FUNCTION__, __LINE__, text.c_str(), color, bgcolor);
#endif
    int xt;
    int yt;
    unsigned int i;
    uint32_t c;
    unsigned int start;

    color = cColor::AlignAlpha(color);
    bgcolor = cColor::AlignAlpha(bgcolor);

    clip(x, 0, width - 1);
    clip(y, 0, height - 1);

    xt = x;
    yt = y;
    start = 0;

    if (text.length() > 0)
    {
        if (skipPixels > 0)
        {
            if (!proportional)
            {
                if (skipPixels >= (int) text.length() * font->TotalWidth())
                    start = text.length();
                else
                    while (skipPixels > font->TotalWidth())
                    {
                        skipPixels -= font->TotalWidth();
                        start++;
                    }
            }
            else
            {
                if (skipPixels >= font->Width(text))
                    start = text.length();
                else
                {
                    while (skipPixels > font->SpaceBetween() + font->Width(text.substr(start), 1 /*text[start]*/))
                    {
                        encodedCharAdjustCounter(font->IsUTF8(), text, c, start);
                        skipPixels -= font->Width(c/*text[start]*/);
                        skipPixels -= font->SpaceBetween();
                        start++;
                    }
                }
            }
        }

        i = start;
        while ( i < (unsigned int)text.length() )
        {
            encodedCharAdjustCounter(font->IsUTF8(), text, c, i);

            if (xt > xmax)
            {
                i = text.length();
            }
            else
            {
                if (!proportional)
                {
                    if (skipPixels > 0)
                    {
                        DrawCharacter(xt, yt, xmax, c, font, color, bgcolor, skipPixels);
                        xt += font->TotalWidth() - skipPixels;
                        skipPixels = 0;
                    }
                    else
                    {
                        DrawCharacter(xt, yt, xmax, c, font, color, bgcolor);
                        xt += font->TotalWidth();
                    }
                }
                else
                {
                    if (skipPixels > 0)
                    {
                        xt += DrawCharacter(xt, yt, xmax, c, font, color, bgcolor, skipPixels);
                        skipPixels = 0;
                    }
                    else
                    {
                        xt += DrawCharacter(xt, yt, xmax, c, font, color, bgcolor);
                    }
                    if (xt <= xmax)
                    {
                        xt += font->SpaceBetween();
                    }
                }
            }
            i++;
        }
    }
    return xt;
}

int cBitmap::DrawCharacter(int x, int y, int xmax, uint32_t c, const cFont * font,
                           uint32_t color, uint32_t bgcolor, int skipPixels)
{
#ifdef DEBUG
    printf("%s:%s(%d) %03d * %03d char '%c' color '%08x' bgcolor '%08x'\n", __FILE__, __FUNCTION__, __LINE__, x, y, c, color, bgcolor);
#endif
    const cBitmap * charBitmap;
    cBitmap * drawBitmap;

    //color = cColor::AlignAlpha(color);
    //bgcolor = cColor::AlignAlpha(bgcolor);

    uint32_t dot = 0;
    int xt, yt;

    clip(x, 0, width - 1);
    clip(y, 0, height - 1);

    charBitmap = font->GetCharacter(c);
    if (charBitmap)
    {
        int drawWidth = charBitmap->Width() - skipPixels;
        if ( x + drawWidth-1 > xmax)
            drawWidth = xmax - x + 1;

        drawBitmap = new cBitmap(drawWidth /*charBitmap->Width()-skipPixels*/,charBitmap->Height());
        if (drawBitmap) {
          drawBitmap->SetProcessAlpha(false);
          drawBitmap->Clear(bgcolor);

          for (xt = 0; xt < drawWidth; xt++) {
            for (yt = 0; yt < charBitmap->Height() ; yt++) {
              dot = charBitmap->GetPixel(xt+skipPixels,yt);
              if ((dot | 0xFF000000) == cColor::Black) { // todo: does not work with antialising?
                drawBitmap->DrawPixel(xt, yt, color);
              } else {
                drawBitmap->DrawPixel(xt, yt, bgcolor);
              }
            }
          }
          DrawBitmap(x, y, *drawBitmap);
          delete drawBitmap;
        }
        return drawWidth; //charBitmap->Width() - skipPixels;
    }
    return 0;
}

uint32_t cBitmap::GetPixel(int x, int y) const
{
    if (x < 0 || x > width - 1)
        return cColor::Transparent;
    if (y < 0 || y > height - 1)
        return cColor::Transparent;

    uint32_t value;
    value = bitmap[y * width + x];
    return value;
}

cBitmap * cBitmap::SubBitmap(int x1, int y1, int x2, int y2) const
{
#ifdef DEBUG
    printf("%s:%s(%d) %03d * %03d / %03d * %03d\n", __FILE__, __FUNCTION__, __LINE__, x1, y1, x2, y2);
#endif
    int w, h;
    int xt, yt;
    cBitmap * bmp;
    uint32_t cl;

    sort(x1,x2);
    sort(y1,y2);
    if (x1 < 0 || x1 > width - 1)
        return NULL;
    if (y1 < 0 || y1 > height - 1)
        return NULL;
    clip(x2, 0, width - 1);
    clip(y2, 0, height - 1);

    w = x2 - x1 + 1;
    h = y2 - y1 + 1;
    bmp = new cBitmap(w, h);
    if (!bmp || !bmp->Data())
        return NULL;
    bmp->Clear();
    bmp->SetMonochrome(this->IsMonochrome());

    for (yt = 0; yt < h; yt++)
    {
      for (xt = 0; xt < w; xt++)
      {
        cl = this->GetPixel(xt+x1, yt+y1);
        bmp->DrawPixel(xt,yt, cl);
      }
    }
    return bmp;
}


// convert a new 32 bpp bitmap to an old style 1bpp bitmap (bit order: from least to most sig. bit:  byte: [ 0, 1, 2, 3, 4, 5, 6, 7 ])
// if IsMonochrome(): ignore threshold
const unsigned char* cBitmap::ConvertTo1BPP(const cBitmap & bitmap, int threshold)
{
    if (bitmap.Width() <= 0 || bitmap.Height() <= 0)
        return NULL;

    int cols = (bitmap.Width() + 7 ) / 8;
    unsigned char* monobmp = new unsigned char[ cols * bitmap.Height() ];
    if (!monobmp)
        return NULL;

    memset(monobmp, 0, cols * bitmap.Height());

    uint32_t col;
    unsigned char greyval = 0;
    bool ismono = bitmap.IsMonochrome();
    
    for (int y = 0; y < bitmap.Height(); y++) {
        for (int x = 0; x < bitmap.Width(); x++) {
            col = bitmap.GetPixel(x, y);
            if (! ismono) {
                // col -> grey level
                greyval = (((0x00FF0000 & col) >> 16) * 77 + ((0x0000FF00 * col) >> 8) * 150 + (0x000000FF & col) * 28) / 255;
                col = (greyval >= threshold) ? cColor::White : cColor::Black;
            }

            if ( col == cColor::Black)
                monobmp[ y * cols + (x >> 3) ] |= ( 1 << ( 7 - (x % 8) ) );
        }
    }
    return monobmp;    
}


// convert an old style 1 bpp bitmap to new 32 bpp bitmap (bit order: from least to most sig. bit:  byte: [ 0, 1, 2, 3, 4, 5, 6, 7 ])
const cBitmap* cBitmap::ConvertFrom1BPP(const unsigned char* monobmp, int w, int h, uint32_t fg, uint32_t bg)
{
    if (w <= 0 || h <= 0 || !monobmp)
        return NULL;
    
    cBitmap* bmp = new cBitmap(w, h, bg);
    if (bmp == NULL)
        return NULL;
    
    int cols = (w + 7 ) / 8;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            bmp->DrawPixel(x, y,  ( monobmp[ y * cols + (x >> 3) ] & ( 1 << (7 - (x % 8)) ) ) ? fg : bg );
        }
    }
    return bmp;
}

#if 0
bool cBitmap::LoadPBM(const std::string & fileName)
{
#ifdef DEBUG
    printf("%s:%s(%d)\n", __FILE__, __FUNCTION__, __LINE__);
#endif
    FILE * pbmFile;
    char str[32];
    int i;
    int ch;
    int w;
    int h;

    pbmFile = fopen(fileName.c_str(), "rb");
    if (!pbmFile)
        return false;

    i = 0;
    while ((ch = getc(pbmFile)) != EOF && i < 31)
    {
        if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r')
            break;
        str[i] = ch;
        i++;
    }
    if (ch == EOF)
    {
        fclose(pbmFile);
        return false;
    }
    str[i] = 0;
    if (strcmp(str, "P4") != 0)
        return false;

    while ((ch = getc(pbmFile)) == '#')
    {
        while ((ch = getc(pbmFile)) != EOF)
        {
            if (ch == '\n' || ch == '\r')
                break;
        }
    }
    if (ch == EOF)
    {
        fclose(pbmFile);
        return false;
    }
    i = 0;
    str[i] = ch;
    i += 1;
    while ((ch = getc(pbmFile)) != EOF && i < 31)
    {
        if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r')
            break;
        str[i] = ch;
        i++;
    }
    if (ch == EOF)
    {
        fclose(pbmFile);
        return false;
    }
    str[i] = 0;
    w = atoi(str);

    i = 0;
    while ((ch = getc(pbmFile)) != EOF && i < 31)
    {
        if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r')
            break;
        str[i] = ch;
        i++;
    }
    if (ch == EOF)
    {
        fclose(pbmFile);
        return false;
    }
    str[i] = 0;
    h = atoi(str);

    if (bitmap)
        delete[] bitmap;
    width = w;
    height = h;
    bitmap = new uint32_t [width * height];
    fread(bitmap, width * height, 1, pbmFile);
    fclose(pbmFile);

    return true;
}

void cBitmap::SavePBM(const std::string & fileName)
{
#ifdef DEBUG
    printf("%s:%s(%d)\n", __FILE__, __FUNCTION__, __LINE__);
#endif
    int i;
    char str[32];
    FILE * fp;

    fp = fopen(fileName.c_str(), "wb");
    if (fp)
    {
        sprintf(str, "P4\n%d %d\n", width, height);
        fwrite(str, strlen(str), 1, fp);
        for (i = 0; i < lineSize * height; i++)
        {
            fwrite(&bitmap[i], 1, 1, fp);
        }
        fclose(fp);
    }
}
#endif

} // end of namespace
