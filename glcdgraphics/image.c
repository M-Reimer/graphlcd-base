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
 * (c) 2013 Wolfgang Astleitner <mrwastl AT users sourceforge net>
 */

#include "bitmap.h"
#include "image.h"
#include "imagefile.h"
#include "glcd.h"
#include "pbm.h"
#include "extformats.h"

#include <string.h>

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


uint32_t cImage::Blend(uint32_t FgColour, uint32_t BgColour, uint8_t Level, double antiAliasGranularity) const
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

bool cImage::Scale(uint16_t scalew, uint16_t scaleh, bool AntiAlias)
{
    if (! (scalew || scaleh) )
        return false;

    unsigned int orig_w = Width();
    unsigned int orig_h = Height();

    // one out of scalew/h == 0 ? -> auto aspect ratio
    if (scalew && ! scaleh) {
       scaleh = (uint16_t)( ((uint32_t)scalew * (uint32_t)orig_h) / (uint32_t)orig_w );
    } else if (!scalew && scaleh) {
       scalew = (uint16_t)( ((uint32_t)scaleh * (uint32_t)orig_w) / (uint32_t)orig_h );
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
        FactorX = (double)scalew / (double)orig_w;
        FactorY = (double)scaleh / (double)orig_h;
        RatioX = (orig_w << 16) / scalew;
        RatioY = (orig_h << 16) / scaleh;
    } else {
        FactorX = (double)scalew / (double)(orig_w-1);
        FactorY = (double)scaleh / (double)(orig_h-1);
        RatioX = ((orig_w-1) << 16) / scalew;
        RatioY = ((orig_h-1) << 16) / scaleh;
    }

    bool downscale = (!AntiAlias || (FactorX <= 1.0 && FactorY <= 1.0));

    for (unsigned int frame = 0; frame < Count() ; frame ++ ) {
        cBitmap *b = new cBitmap(scalew, scaleh, GRAPHLCD_Transparent);

        cBitmap *currFrame = GetBitmap(frame);

        b->SetMonochrome(currFrame->IsMonochrome());

        if (downscale) {
            // Downscaling - no anti-aliasing:
            const uint32_t *DestRow = b->Data();
            int SourceY = 0;
            for (int y = 0; y < scaleh; y++) {
                int SourceX = 0;
                const uint32_t *SourceRow = currFrame->Data() + (SourceY >> 16) * orig_w;
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
    // clear all bitmaps from this image
    unsigned int temp_delay = Delay();
    Clear();
    // set new resolution
    SetWidth(scalew);
    SetHeight(scaleh);
    SetDelay(temp_delay);
    // re-add bitmaps from scaled image container
    cBitmap * b;
    cBitmap * tempb;
    for (unsigned int frame = 0; frame < tempImg.Count(); frame ++) {
      tempb = tempImg.GetBitmap(frame);
      b = new cBitmap(scalew, scaleh, (uint32_t*)tempb->Data());
      b->SetMonochrome(tempb->IsMonochrome());
      AddBitmap(b);
    }
    return true;
}


/* static methods */
bool cImage::LoadImage(cImage & image, const std::string & fileName) {
    const std::string fext = GetFilenameExtension(fileName);
    cImageFile* imgFile = NULL;
    bool result = true;

    if (fext == "PBM") {
        imgFile = new cPBMFile();
    } else if (fext == "GLCD") {
        imgFile = new cGLCDFile();
    } else {
        imgFile = new cExtFormatFile();
    }

    uint16_t scale_w = 0;
    uint16_t scale_h = 0;

    if (!imgFile || (imgFile->LoadScaled(image, fileName, scale_w, scale_h) == false) )
        result = false;

    if (imgFile) delete imgFile;
    return result;
}


bool cImage::SaveImage(cImage & image, const std::string & fileName) {
    const std::string fext = GetFilenameExtension(fileName);
    cImageFile* imgFile = NULL;
    bool result = false;

    if (fext == "PBM") {
        imgFile = new cPBMFile();
    } else if (fext == "GLCD") {
        imgFile = new cGLCDFile();
    } else {
        imgFile = new cExtFormatFile();
    }
    if ( imgFile && imgFile->Save(image, fileName) )
      result = true;

    if (imgFile) delete imgFile;
    return result;
}


const std::string cImage::GetFilenameExtension(const std::string & fileName) {
    size_t pos = fileName.find_last_of('.');
    std::string ext = "";
    if (pos != std::string::npos) {
      ext = fileName.substr(pos+1);
      for (size_t i = 0; i < ext.size(); i++)
        ext[i] = toupper(ext[i]);
    }
    return ext;
}


} // end of namespace
