/*
 * GraphLCD graphics library
 *
 * glcd.c  -  GLCD file loading and saving
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
#include <stdint.h>
#include <syslog.h>

#include <cstring>

#include "bitmap.h"
#include "glcd.h"
#include "image.h"


namespace GLCD
{

using namespace std;

const char * kGLCDFileSign = "GLC";

/*
#pragma pack(1)
struct tGLCDHeader
{
    char sign[3];    // = "GLC"
    char format;     // D - single image, A - animation
    uint16_t width;  // width in pixels
    uint16_t height; // height in pixels
    // only for animations
    uint16_t count;  // number of pictures
    uint32_t delay;  // delay in ms
};
#pragma pack()
*/

cGLCDFile::cGLCDFile()
{
}

cGLCDFile::~cGLCDFile()
{
}

bool cGLCDFile::Load(cImage & image, const string & fileName)
{
    FILE * fp;
    long fileSize;
    char sign[4];
    uint8_t buf[6];
    uint16_t width;
    uint16_t height;
    uint16_t count;
    uint32_t delay;

    fp = fopen(fileName.c_str(), "rb");
    if (!fp)
    {
        syslog(LOG_ERR, "glcdgraphics: open %s failed (cGLCDFile::Load).", fileName.c_str());
        return false;
    }

    // get len of file
    if (fseek(fp, 0, SEEK_END) != 0)
    {
        fclose(fp);
        return false;
    }
    fileSize = ftell(fp);

    // rewind and get Header
    if (fseek(fp, 0, SEEK_SET) != 0)
    {
        fclose(fp);
        return false;
    }

    // read header sign
    if (fread(sign, 4, 1, fp) != 1)
    {
        fclose(fp);
        return false;
    }

    // check header sign
    if (strncmp(sign, kGLCDFileSign, 3) != 0)
    {
        syslog(LOG_ERR, "glcdgraphics: load %s failed, wrong header (cGLCDFile::Load).", fileName.c_str());
        fclose(fp);
        return false;
    }

    // read width and height
    if (fread(buf, 4, 1, fp) != 1)
    {
        fclose(fp);
        return false;
    }

    width = (buf[1] << 8) | buf[0];
    height = (buf[3] << 8) | buf[2];
    if (width == 0 || height == 0)
    {
        syslog(LOG_ERR, "glcdgraphics: load %s failed, wrong header (cGLCDFile::Load).", fileName.c_str());
        fclose(fp);
        return false;
    }

    if (sign[3] == 'D')
    {
        count = 1;
        delay = 10;
        // check file length
        if (fileSize != (long) (height * ((width + 7) / 8) + 8))
        {
            syslog(LOG_ERR, "glcdgraphics: load %s failed, wrong size (cGLCDFile::Load).", fileName.c_str());
            fclose(fp);
            return false;
        }
    }
    else if (sign[3] == 'A')
    {
        // read count and delay
        if (fread(buf, 6, 1, fp) != 1)
        {
            syslog(LOG_ERR, "glcdgraphics: load %s failed, wrong header (cGLCDFile::Load).", fileName.c_str());
            fclose(fp);
            return false;
        }
        count = (buf[1] << 8) | buf[0];
        delay = (buf[5] << 24) | (buf[4] << 16) | (buf[3] << 8) | buf[2];
        // check file length
        if (count == 0 ||
            fileSize != (long) (count * (height * ((width + 7) / 8)) + 14))
        {
            syslog(LOG_ERR, "glcdgraphics: load %s failed, wrong size (cGLCDFile::Load).", fileName.c_str());
            fclose(fp);
            return false;
        }
        // Set minimal limit for next image
        if (delay < 10)
            delay = 10;
    }
    else
    {
        syslog(LOG_ERR, "glcdgraphics: load %s failed, wrong header (cGLCDFile::Load).", fileName.c_str());
        fclose(fp);
        return false;
    }

    image.Clear();
    image.SetWidth(width);
    image.SetHeight(height);
    image.SetDelay(delay);
    unsigned char * bmpdata_raw = new unsigned char[height * ((width + 7) / 8)];
    uint32_t * bmpdata = new uint32_t[height * width];
    if (bmpdata && bmpdata_raw)
    {
        for (unsigned int n = 0; n < count; n++)
        {
            if (fread(bmpdata_raw, height * ((width + 7) / 8), 1, fp) != 1)
            {
                delete[] bmpdata;
                fclose(fp);
                image.Clear();
                return false;
            }
            int colsize = (width+7)/8;
            for (int j = 0; j < height; j++) {
                for (int i = 0; i < width; i++) {
                    if (  bmpdata_raw[j*colsize + (i>>3)] & (1 << (7-(i%8))) ) {
                        bmpdata[j*width+i] = cColor::Black;
                    } else {
                        bmpdata[j*width+i] = cColor::White;
                    }
                }
            }
#ifdef DEBUG
    printf("%s:%s(%d) - filename: '%s', count %d\n", __FILE__, __FUNCTION__, __LINE__, fileName.c_str(), n);
#endif
            cBitmap * b = new cBitmap(width, height, bmpdata);
            b->SetMonochrome(true);
            //image.AddBitmap(new cBitmap(width, height, bmpdata));
            image.AddBitmap(b);
        }
        delete[] bmpdata;
    }
    else
    {
        syslog(LOG_ERR, "glcdgraphics: malloc failed (cGLCDFile::Load).");
        if (bmpdata)
            delete[] bmpdata;
        if (bmpdata_raw)
            delete[] bmpdata_raw;
        fclose(fp);
        image.Clear();
        return false;
    }
    fclose(fp);
    if (bmpdata_raw)
        delete[] bmpdata_raw;

    syslog(LOG_DEBUG, "glcdgraphics: image %s loaded.", fileName.c_str());
    return true;
}

bool cGLCDFile::Save(cImage & image, const string & fileName)
{
    FILE * fp;
    uint8_t buf[14];
    uint16_t width;
    uint16_t height;
    uint16_t count;
    uint32_t delay;
    const cBitmap * bitmap;
    int i;

    if (image.Count() == 0)
        return false;

    fp = fopen(fileName.c_str(), "wb");
    if (!fp)
    {
        syslog(LOG_ERR, "glcdgraphics: open %s failed (cGLCDFile::Save).", fileName.c_str());
        return false;
    }

    memcpy(buf, kGLCDFileSign, 3);
    count = image.Count();
    delay = image.Delay();
    if (count == 1)
    {
        buf[3] = 'D';
    }
    else
    {
        buf[3] = 'A';
    }
    bitmap = image.GetBitmap(0);
    width = bitmap->Width();
    height = bitmap->Height();
    buf[4] = (uint8_t) width;
    buf[5] = (uint8_t) (width >> 8);
    buf[6] = (uint8_t) height;
    buf[7] = (uint8_t) (height >> 8);
    if (count == 1)
    {
        if (fwrite(buf, 8, 1, fp) != 1)
        {
            fclose(fp);
            return false;
        }
    }
    else
    {
        buf[8] = (uint8_t) count;
        buf[9] = (uint8_t) (count >> 8);
        buf[10] = (uint8_t) delay;
        buf[11] = (uint8_t) (delay >> 8);
        buf[12] = (uint8_t) (delay >> 16);
        buf[13] = (uint8_t) (delay >> 24);
        if (fwrite(buf, 14, 1, fp) != 1)
        {
            fclose(fp);
            return false;
        }
    }
    for (i = 0; i < count; i++)
    {
        bitmap = image.GetBitmap(i);
        if (bitmap)
        {
            if (bitmap->Width() == width && bitmap->Height() == height)
            {
//                if (fwrite(bitmap->Data(), height * ((width + 7) / 8), 1, fp) != 1)
                if (fwrite(bitmap->Data(), height * width, 1, fp) != 1)
                {
                    fclose(fp);
                    return false;
                }
            }
        }
    }
    fclose(fp);

    syslog(LOG_DEBUG, "glcdgraphics: image %s saved.", fileName.c_str());
    return true;
}

} // end of namespace
