/*
 * GraphLCD graphics library
 *
 * extformats.c  -  loading and saving of external formats (via ImageMagick)
 *
 * based on bitmap.[ch] from text2skin: http://projects.vdr-developer.org/projects/show/plg-text2skin
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2011-2013 Wolfgang Astleitner <mrwastl AT users sourceforge net>
 */

#include <stdio.h>
#include <stdint.h>
#include <syslog.h>

#include <cstring>

#include "bitmap.h"
#include "extformats.h"
#include "image.h"

#ifdef HAVE_IMAGEMAGICK_7
  #include <MagickWand/MagickWand.h>
#elifdef HAVE_IMAGEMAGICK
  #include <wand/magick_wand.h>
#endif


namespace GLCD
{

using namespace std;


cExtFormatFile::cExtFormatFile()
{
#ifdef HAVE_IMAGEMAGICK_7
  MagickWandGenesis();
#elifdef HAVE_IMAGEMAGICK
  InitializeMagick(NULL);
#endif
}

cExtFormatFile::~cExtFormatFile()
{
}

bool cExtFormatFile::Load(cImage & image, const string & fileName)
{
    uint16_t scalew = 0;
    uint16_t scaleh = 0;
    return LoadScaled(image, fileName, scalew, scaleh);
}

bool cExtFormatFile::LoadScaled(cImage & image, const string & fileName, uint16_t & scalew, uint16_t & scaleh)
{
#ifdef HAVE_IMAGEMAGICK
  MagickWand* mw = NewMagickWand();

  uint16_t width = 0;
  uint16_t height = 0;
  uint32_t delay;

  if (MagickReadImage(mw, fileName.c_str()) == MagickFalse) {
    syslog(LOG_ERR, "glcdgraphics: Couldn't load '%s' (cExtFormatFile::LoadScaled)", fileName.c_str());
    return false;
  }

  delay = (uint32_t)(MagickGetImageDelay(mw) * 10);

  image.Clear();
  image.SetDelay(delay);

  for (unsigned long imageindex = 0; imageindex < MagickGetNumberImages(mw); imageindex++) {

#ifdef HAVE_IMAGEMAGICK_7
    MagickSetIteratorIndex(mw, imageindex);
#else
    MagickSetImageIndex(mw, imageindex);
#endif

    bool ignoreImage = false;

    if (imageindex == 0) { // If first image
      width = (uint16_t)MagickGetImageWidth(mw);
      height = (uint16_t)MagickGetImageHeight(mw);

      // one out of scalew/h == 0 ? -> auto aspect ratio
      if (scalew && ! scaleh) {
        scaleh = (uint16_t)( ((uint32_t)scalew * (uint32_t)height) / (uint32_t)width );
      } else if (!scalew && scaleh) {
        scalew = (uint16_t)( ((uint32_t)scaleh * (uint32_t)width) / (uint32_t)height );
      }

      // scale image
      if (scalew && ! (scalew == width && scaleh == height)) {
        MagickSampleImage(mw, scalew, scaleh);
        width = scalew;
        height = scaleh;
      } else {
        // not scaled => reset to 0
        scalew = 0;
        scaleh = 0;
      }

      image.SetWidth(width);
      image.SetHeight(height);
    } else {
      if (scalew && scaleh) {
        MagickSampleImage(mw, scalew, scaleh);
      } else 
      if ( (width != (uint16_t)MagickGetImageWidth(mw)) || (height != (uint16_t)MagickGetImageHeight(mw)) ) {
        ignoreImage = true;
      }
    }

    if (! ignoreImage) {
      uint32_t * bmpdata = new uint32_t[height * width];

#ifdef HAVE_IMAGEMAGICK_7
      unsigned int status = MagickExportImagePixels(mw, 0, 0, width, height, "BGRA", CharPixel, (unsigned char*)bmpdata);
#else
      unsigned int status = MagickGetImagePixels(mw, 0, 0, width, height, "BGRA", CharPixel, (unsigned char*)bmpdata);
#endif

      if (status == MagickFalse) {
        syslog(LOG_ERR, "glcdgraphics: Couldn't load '%s' (cExtFormatFile::LoadScaled): MagickGetImagePixels", fileName.c_str());
        return false;
      }

#ifdef HAVE_IMAGEMAGICK_7
      bool isMatte = (MagickGetImageAlphaChannel(mw) == MagickTrue);
#else
      bool isMatte = (MagickGetImageMatte(mw) == MagickTrue);
#endif

      // Give all transparent pixels our defined transparent color
      if (isMatte) {
        for (int iy = 0; iy < (int)height; ++iy) {
          for (int ix = 0; ix < (int)width; ++ix) {
            uint32_t* pixel = &bmpdata[ix+iy*width];
            uint8_t alpha = *pixel >> 24;
            if (alpha == 0)
              *pixel = cColor::Transparent;
          }
        }
      }

      cBitmap * b = new cBitmap(width, height, bmpdata);
      //b->SetMonochrome(isMonochrome);
      image.AddBitmap(b);
      delete[] bmpdata;
      bmpdata = NULL;
    }
  }
  return true;
#else
  return false;
#endif
}

// to be done ...
bool cExtFormatFile::Save(cImage & image, const string & fileName)
{
  return false;
}

} // end of namespace
