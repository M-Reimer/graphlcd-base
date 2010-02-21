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
 * (c) 2004 Andreas Regel <andreas.regel AT powarman.de>
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

const unsigned char bitmask[8]  = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
const unsigned char bitmaskl[8] = {0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff};
const unsigned char bitmaskr[8] = {0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01};

cBitmap::cBitmap(int width, int height, unsigned char * data)
:   width(width),
    height(height),
    bitmap(NULL)
{
    // lines are byte aligned
    lineSize = (width + 7) / 8;

    bitmap = new unsigned char[lineSize * height];
    if (data)
        memcpy(bitmap, data, lineSize * height);
}

cBitmap::cBitmap(const cBitmap & b)
{
    width = b.width;
    height = b.height;
    lineSize = b.lineSize;
    bitmap = new unsigned char[lineSize * height];
    if (b.bitmap)
        memcpy(bitmap, b.bitmap, lineSize * height);
}

cBitmap::~cBitmap()
{
    delete[] bitmap;
}

void cBitmap::Clear()
{
    memset(bitmap, 0, lineSize * height);
}

void cBitmap::Invert()
{
    int i;

    for (i = 0; i < lineSize * height; i++)
    {
        bitmap[i] ^= 0xFF;
    }
}

void cBitmap::DrawPixel(int x, int y, eColor color)
{
    if (x < 0 || x > width - 1)
        return;
    if (y < 0 || y > height - 1)
        return;

    unsigned char c = 0x80 >> (x % 8);
    if (color == clrBlack)
        bitmap[lineSize * y + x / 8] |= c;
    else
        bitmap[lineSize * y + x / 8] &= ~c;
}

void cBitmap::Draw8Pixels(int x, int y, unsigned char pixels, eColor color)
{
    if (x < 0 || x > width - 1)
        return;
    if (y < 0 || y > height - 1)
        return;

    if (color == clrBlack)
        bitmap[lineSize * y + x / 8] |= pixels;
    else
        bitmap[lineSize * y + x / 8] &= ~pixels;
}

void cBitmap::DrawLine(int x1, int y1, int x2, int y2, eColor color)
{
    int d, sx, sy, dx, dy;
    unsigned int ax, ay;

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

void cBitmap::DrawHLine(int x1, int y, int x2, eColor color)
{
    sort(x1,x2);

    if (x1 / 8 == x2 / 8)
    {
        // start and end in the same byte
        Draw8Pixels(x1, y, bitmaskr[x1 % 8] & bitmaskl[x2 % 8], color);
    }
    else
    {
        // start and end in different bytes
        Draw8Pixels(x1, y, bitmaskr[x1 % 8], color);
        x1 = ((x1 + 8) / 8) * 8;
        while (x1 < (x2 / 8) * 8)
        {
            Draw8Pixels(x1, y, 0xff, color);
            x1 += 8;
        }
        Draw8Pixels(x2, y, bitmaskl[x2 % 8], color);
    }
}

void cBitmap::DrawVLine(int x, int y1, int y2, eColor color)
{
    int y;

    sort(y1,y2);

    for (y = y1; y <= y2; y++)
        DrawPixel(x, y, color);
}

void cBitmap::DrawRectangle(int x1, int y1, int x2, int y2, eColor color, bool filled)
{
    int y;

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

void cBitmap::DrawRoundRectangle(int x1, int y1, int x2, int y2, eColor color, bool filled, int type)
{
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
            DrawPixel(x1 + 1, y1 + 1, color == clrWhite ? clrBlack : clrWhite);
            DrawPixel(x1 + 1, y2 - 1, color == clrWhite ? clrBlack : clrWhite);
            DrawPixel(x2 - 1, y1 + 1, color == clrWhite ? clrBlack : clrWhite);
            DrawPixel(x2 - 1, y2 - 1, color == clrWhite ? clrBlack : clrWhite);
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

void cBitmap::DrawEllipse(int x1, int y1, int x2, int y2, eColor color, bool filled, int quadrants)
{
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

void cBitmap::DrawSlope(int x1, int y1, int x2, int y2, eColor color, int type)
{
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
            if (upper && !falling || !upper && falling)
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

void cBitmap::DrawBitmap(int x, int y, const cBitmap & bitmap, eColor color)
{
    unsigned char cl = 0;
    int xt, yt;
    const unsigned char * data = bitmap.Data();
    unsigned short temp;
    int h, w;

    w = bitmap.Width();
    h = bitmap.Height();

    if (data)
    {
        if (!(x % 8))
        {
            // Bitmap is byte alligned (0,8,16,...)
            for (yt = 0; yt < h; yt++)
            {
                for (xt = 0; xt < (w / 8); xt++)
                {
                    cl = *data;
                    Draw8Pixels(x + (xt * 8), y + yt, cl, color);
                    data++;
                }
                if (w % 8)
                {
                    cl = *data;
                    Draw8Pixels(x + ((w / 8) * 8), y + yt, cl & bitmaskl[w % 8 - 1], color);
                    data++;
                }
            }
        }
        else
        {
            // Bitmap is not byte alligned
            for (yt = 0; yt < h; yt++)
            {
                temp = 0;
                for (xt = 0; xt < (w + (x % 8)) / 8; xt++)
                {
                    cl = *(data + yt * ((w + 7) / 8) + xt);
                    temp = temp | ((unsigned short) cl << (8 - (x % 8)));
                    cl = (temp & 0xff00) >> 8;
                    if (!xt)
                    {
                        // first byte
                        Draw8Pixels(x - (x % 8) + (xt * 8), y + yt, cl & bitmaskr[x % 8], color);
                    }
                    else
                    {
                        // not the first byte
                        Draw8Pixels(x - (x % 8) + (xt * 8), y + yt, cl, color);
                    }
                    temp <<= 8;
                }
                if ((w + (x % 8) + 7) / 8 != (w + (x % 8)) / 8)
                {
                    // print the rest
                    cl = *(data + (yt + 1) * ((w + 7) / 8) - 1);
                    temp = temp | ((unsigned short) cl << (8 - (x % 8)));
                    cl = (temp & 0xff00) >> 8;
                    Draw8Pixels(x - (x % 8) + (((w + (x % 8)) / 8) * 8), y + yt, cl & bitmaskl[(w + x) % 8 - 1], color);
                }
            }
        }
    }
}

int cBitmap::DrawText(int x, int y, int xmax, const std::string & text, const cFont * font,
                      eColor color, bool proportional, int skipPixels)
{
    int xt;
    int yt;
    int i;
    char c;
    int start;

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
                    while (skipPixels > font->Width(text[start]))
                    {
                        skipPixels -= font->Width(text[start]);
                        skipPixels -= font->SpaceBetween();
                        start++;
                    }
            }
        }
        for (i = start; i < (int) text.length(); i++)
        {
            c = text[i];
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
                        DrawCharacter(xt, yt, xmax, c, font, color, skipPixels);
                        xt += font->TotalWidth() - skipPixels;
                        skipPixels = 0;
                    }
                    else
                    {
                        DrawCharacter(xt, yt, xmax, c, font, color);
                        xt += font->TotalWidth();
                    }
                }
                else
                {
                    if (skipPixels > 0)
                    {
                        xt += DrawCharacter(xt, yt, xmax, c, font, color, skipPixels);
                        skipPixels = 0;
                    }
                    else
                    {
                        xt += DrawCharacter(xt, yt, xmax, c, font, color);
                    }
                    if (xt <= xmax)
                    {
                        xt += font->SpaceBetween();
                    }
                }
            }
        }
    }
    return xt;
}

int cBitmap::DrawCharacter(int x, int y, int xmax, char c, const cFont * font,
                           eColor color, int skipPixels)
{
    const cBitmap * charBitmap;

    clip(x, 0, width - 1);
    clip(y, 0, height - 1);

    charBitmap = font->GetCharacter(c);
    if (charBitmap)
    {
        cBitmap * drawBitmap = charBitmap->SubBitmap(skipPixels, 0, xmax - x + skipPixels, charBitmap->Height() - 1);
        if (drawBitmap)
            DrawBitmap(x, y, *drawBitmap, color);
        delete drawBitmap;
        return charBitmap->Width() - skipPixels;
    }
    return 0;
}

unsigned char cBitmap::GetPixel(int x, int y) const
{
    unsigned char value;

    value = bitmap[y * lineSize + x / 8];
    value = (value >> (7 - (x % 8))) & 1;
    return value;
}

cBitmap * cBitmap::SubBitmap(int x1, int y1, int x2, int y2) const
{
    int w, h;
    int xt, yt;
    cBitmap * bmp;
    unsigned char cl;
    unsigned char * data;
    unsigned short temp;

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
    if (x1 % 8 == 0)
    {
        // Bitmap is byte alligned (0,8,16,...)
        for (yt = 0; yt < h; yt++)
        {
            data = &bitmap[(y1 + yt) * lineSize + x1 / 8];
            for (xt = 0; xt < (w / 8) * 8; xt += 8)
            {
                cl = *data;
                bmp->Draw8Pixels(xt, yt, cl, clrBlack);
                data++;
            }
            if (w % 8 != 0)
            {
                cl = *data;
                bmp->Draw8Pixels(xt, yt, cl & bitmaskl[w % 8 - 1], clrBlack);
            }
        }
    }
    else
    {
        // Bitmap is not byte alligned
        for (yt = 0; yt < h; yt++)
        {
            temp = 0;
            data = &bitmap[(y1 + yt) * lineSize + x1 / 8];
            for (xt = 0; xt <= ((w / 8)) * 8; xt += 8)
            {
                cl = *data;
                temp = temp | ((unsigned short) cl << (x1 % 8));
                cl = (temp & 0xff00) >> 8;
                if (xt > 0)
                {
                    bmp->Draw8Pixels(xt - 8, yt, cl, clrBlack);
                }
                temp <<= 8;
                data++;
            }
            if (w % 8 != 0)
            {
                // print the rest
                if (8 - (x1 % 8) < w % 8)
                {
                    cl = *data;
                    temp = temp | ((unsigned short) cl << (x1 % 8));
                }
                cl = (temp & 0xff00) >> 8;
                bmp->Draw8Pixels(xt - 8, yt, cl & bitmaskl[(w % 8) - 1], clrBlack);
            }
        }
    }
    return bmp;
}

bool cBitmap::LoadPBM(const std::string & fileName)
{
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

    delete[] bitmap;
    width = w;
    height = h;
    // lines are byte aligned
    lineSize = (width + 7) / 8;
    bitmap = new unsigned char[lineSize * height];
    fread(bitmap, lineSize * height, 1, pbmFile);
    fclose(pbmFile);

    return true;
}

void cBitmap::SavePBM(const std::string & fileName)
{
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

} // end of namespace
