/*
 * GraphLCD graphics library
 *
 * imagefile.h  -  base class for file loading and saving
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2006 Andreas Regel <andreas.regel AT powarman.de>
 */

#include "image.h"
#include "imagefile.h"
#include "bitmap.h"

namespace GLCD
{

cImageFile::cImageFile(void)
{
}
cImageFile::~cImageFile(void)
{
}

bool cImageFile::Load(cImage & image, const std::string & fileName)
{
    return false;
}

bool cImageFile::Save(cImage & image, const std::string & fileName)
{
    return false;
}


uint32_t cImageFile::Blend(uint32_t FgColour, uint32_t BgColour, uint8_t Level, double antiAliasGranularity) const
{
    if (antiAliasGranularity > 0.0)
       Level = uint8_t(int(Level / antiAliasGranularity + 0.5) * antiAliasGranularity);
    int Af = (FgColour & 0xFF000000) >> 24;
    int Rf = (FgColour & 0x00FF0000) >> 16;
    int Gf = (FgColour & 0x0000FF00) >>  8;
    int Bf = (FgColour & 0x000000FF);
    int Ab = (BgColour & 0xFF000000) >> 24;
    int Rb = (BgColour & 0x00FF0000) >> 16;
    int Gb = (BgColour & 0x0000FF00) >>  8;
    int Bb = (BgColour & 0x000000FF);
    int A = (Ab + (Af - Ab) * Level / 0xFF) & 0xFF;
    int R = (Rb + (Rf - Rb) * Level / 0xFF) & 0xFF;
    int G = (Gb + (Gf - Gb) * Level / 0xFF) & 0xFF;
    int B = (Bb + (Bf - Bb) * Level / 0xFF) & 0xFF;
    return (A << 24) | (R << 16) | (G << 8) | B;
}

bool cImageFile::Scale(cImage & image, uint16_t scalew, uint16_t scaleh, bool AntiAlias)
{
    if (! (scalew || scaleh) )
        return false;

    // one out of scalew/h == 0 ? -> auto aspect ratio
    if (scalew && ! scaleh) {
       scaleh = (uint16_t)( ((uint32_t)scalew * (uint32_t)image.Height()) / (uint32_t)image.Width() );
    } else if (!scalew && scaleh) {
       scalew = (uint16_t)( ((uint32_t)scaleh * (uint32_t)image.Width()) / (uint32_t)image.Height() );
    }

    cImage tempImg = cImage();
    tempImg.SetWidth(scalew);
    tempImg.SetHeight(scaleh);

    // Scaling/Blending based on VDR / osd.c
    // Fixed point scaling code based on www.inversereality.org/files/bitmapscaling.pdf
    // by deltener@mindtremors.com
    //
    // slightly improved by Wolfgang Astleitner (modify factors and ratios so that scaled image is centered when upscaling)

    double FactorX, FactorY;
    int    RatioX,    RatioY;

    if (!AntiAlias) {
        FactorX = (double)scalew / (double)image.Width();
        FactorY = (double)scaleh / (double)image.Height();
        RatioX = (image.Width() << 16) / scalew;
        RatioY = (image.Height() << 16) / scaleh;
    } else {
        FactorX = (double)scalew / (double)(image.Width()-1);
        FactorY = (double)scaleh / (double)(image.Height()-1);
        RatioX = ((image.Width()-1) << 16) / scalew;
        RatioY = ((image.Height()-1) << 16) / scaleh;
    }

    bool downscale = (!AntiAlias || (FactorX <= 1.0 && FactorY <= 1.0));

    for (unsigned int frame = 0; frame < image.Count() ; frame ++ ) {
        cBitmap *b = new cBitmap(scalew, scaleh, GRAPHLCD_Transparent);
        
        cBitmap *currFrame = image.GetBitmap(frame);
      
        if (downscale) {
            // Downscaling - no anti-aliasing:
            const uint32_t *DestRow = b->Data();
            int SourceY = 0;
            for (int y = 0; y < scaleh; y++) {
                int SourceX = 0;
                const uint32_t *SourceRow = currFrame->Data() + (SourceY >> 16) * image.Width();
                uint32_t *Dest = (uint32_t*) DestRow;
                for (int x = 0; x < scalew; x++) {
                    *Dest++ = SourceRow[SourceX >> 16];
                    SourceX += RatioX;
                }
                SourceY += RatioY;
                DestRow += scalew;
            }
        } else {
            // Upscaling - anti-aliasing:
            int SourceY = 0;
            for (int y = 0; y < scaleh /*- 1*/; y++) {
                int SourceX = 0;
                int sy = SourceY >> 16;
                uint8_t BlendY = 0xFF - ((SourceY >> 8) & 0xFF);
                for (int x = 0; x < scalew /*- 1*/; x++) {
                    int sx = SourceX >> 16;
                    uint8_t BlendX = 0xFF - ((SourceX >> 8) & 0xFF);
                    // TODO: antiAliasGranularity
                    uint32_t c1 = Blend(currFrame->GetPixel(sx, sy),     currFrame->GetPixel(sx + 1, sy),     BlendX);
                    uint32_t c2 = Blend(currFrame->GetPixel(sx, sy + 1), currFrame->GetPixel(sx + 1, sy + 1), BlendX);
                    uint32_t c3 = Blend(c1, c2, BlendY);
                    b->DrawPixel(x, y, c3);
                    SourceX += RatioX;
                }
                SourceY += RatioY;
            }
        }
        tempImg.AddBitmap(b);
    }
    // clear all bitmaps from image
    image.Clear();
    // set new resolution
    image.SetWidth(scalew);
    image.SetHeight(scaleh);
    // re-add bitmaps from scaled image container
    for (unsigned int frame = 0; frame < tempImg.Count(); frame ++) {
      image.AddBitmap(new cBitmap(scalew, scaleh, (uint32_t*)tempImg.GetBitmap(frame)->Data()));
    }
    return true;
}

bool cImageFile::LoadScaled(cImage & image, const std::string & fileName, uint16_t & scalew, uint16_t & scaleh)
{
    if (Load(image, fileName)) {
        if (scalew || scaleh) {
            return Scale(image, scalew, scaleh, true);
        } else {
            return true;
        }
    } else {
      scalew = 0;
      scaleh = 0;
      return false;
    }
}

} // end of namespace
