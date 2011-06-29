/*
 * GraphLCD graphics library
 *
 * pbm.c  -  PBM file loading and saving
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2006-2010 Andreas Regel <andreas.regel AT powarman.de>
 * (c) 2010-2011 Wolfgang Astleitner <mrwastl AT users sourceforge net>
 *               Andreas 'randy' Weinberger
 */

#include <stdio.h>
#include <stdint.h>
#include <syslog.h>
#include <cstdlib>

#include <cstring>

#include "bitmap.h"
#include "pbm.h"
#include "image.h"


namespace GLCD
{

cPBMFile::cPBMFile()
{
}

cPBMFile::~cPBMFile()
{
}

bool cPBMFile::Load(cImage & image, const std::string & fileName)
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

    image.Clear();
    image.SetWidth(w);
    image.SetHeight(h);
    image.SetDelay(100);

    unsigned char * bmpdata_raw = new unsigned char[h * ((w + 7) / 8)];
    uint32_t * bmpdata = new uint32_t[h * w];
    if (bmpdata && bmpdata_raw)
    {
            if (fread(bmpdata_raw, h * ((w + 7) / 8), 1, pbmFile) != 1)
            {
                delete[] bmpdata;
                fclose(pbmFile);
                image.Clear();
                return false;
            }
            int colsize = (w+7)/8;
            for (int j = 0; j < h; j++) {
                for (int i = 0; i < w; i++) {
                    if (  bmpdata_raw[j*colsize + (i>>3)] & (1 << (7-(i%8))) ) {
                        bmpdata[j*w+i] = cColor::Black;
                    } else {
                        bmpdata[j*w+i] = cColor::White;
                    }
                }
            }
            delete [] bmpdata_raw;

            cBitmap * b = new cBitmap(w, h, bmpdata);
            b->SetMonochrome(true);
            //image.AddBitmap(new cBitmap(width, height, bmpdata));
            image.AddBitmap(b);
            delete[] bmpdata;
    }
    else
    {
        syslog(LOG_ERR, "glcdgraphics: malloc failed (cPBMFile::Load).");
        fclose(pbmFile);
        return false;
    }
    fclose(pbmFile);
    syslog(LOG_DEBUG, "glcdgraphics: image %s loaded.", fileName.c_str());

    return true;
}

bool cPBMFile::Save(cImage & image, const std::string & fileName)
{
    FILE * fp;
    char str[32];
    const cBitmap * bitmap;
    unsigned char* rawdata = NULL;
    int rawdata_size = 0;
    const uint32_t * bmpdata = NULL;

    if (image.Count() == 1)
    {
        fp = fopen(fileName.c_str(), "wb");
        if (fp)
        {
            bitmap = image.GetBitmap(0);
            rawdata_size = ((bitmap->Width() + 7) / 8) * bitmap->Height();
            rawdata = new unsigned char[ rawdata_size ];
            bmpdata = bitmap->Data();

            if (bitmap && rawdata && bmpdata)
            {
                memset(rawdata, 0, rawdata_size );
                for (int y = 0; y < bitmap->Height(); y++) {
                    int startpos = y * ((bitmap->Width() + 7) / 8);
                    for (int x = 0; x < bitmap->Width(); x++) {
                        if (bmpdata[ y * bitmap->Width() + x ] == cColor::White) {
                            rawdata[ startpos + (x / 8) ] |= (1 << ( 7 - ( x % 8 ) ));
                        }
                    }
                }
                sprintf(str, "P4\n%d %d\n", bitmap->Width(), bitmap->Height());
                fwrite(str, strlen(str), 1, fp);
//                fwrite(bitmap->Data(), bitmap->LineSize() * bitmap->Height(), 1, fp);
                fwrite(rawdata, rawdata_size, 1, fp);
            }
            fclose(fp);
            delete[] rawdata;
            rawdata = NULL;
        }
    }
    else
    {
        uint16_t i;
        char tmpStr[256];

        for (i = 0; i < image.Count(); i++)
        {
            sprintf(tmpStr, "%.248s.%05d", fileName.c_str(), i);
            fp = fopen(tmpStr, "wb");
            if (fp)
            {
                bitmap = image.GetBitmap(i);
                rawdata_size = ((bitmap->Width() + 7) / 8) * bitmap->Height();
                rawdata = new unsigned char[ rawdata_size ];
                bmpdata = bitmap->Data();

                if (bitmap && rawdata && bmpdata)
                {
                    memset(rawdata, 0, rawdata_size );
                    for (int y = 0; y < bitmap->Height(); y++) {
                        int startpos = y * ((bitmap->Width() + 7) / 8);
                        for (int x = 0; x < bitmap->Width(); x++) {
                            if (bmpdata[ y * bitmap->Width() + x ] == cColor::Black) {
                                rawdata[ startpos + (x / 8) ] |= (1 << ( 7 - ( x % 8 ) ));
                            }
                        }
                    }
                    sprintf(str, "P4\n%d %d\n", bitmap->Width(), bitmap->Height());
                    fwrite(str, strlen(str), 1, fp);
//                    fwrite(bitmap->Data(), bitmap->LineSize() * bitmap->Height(), 1, fp);
                    fwrite(rawdata, rawdata_size, 1, fp);
                }
                fclose(fp);
                delete[] rawdata;
                rawdata = NULL;
            }
        }
    }
    return true;
}

} // end of namespace

