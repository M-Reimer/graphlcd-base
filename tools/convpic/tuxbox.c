/**
 *  GraphLCD plugin for the Video Disk Recorder
 *
 *  tuxbox.c  -  tuxbox logo class
 *
 *  (c) 2004 Andreas Brachold <vdr04 AT deltab de>
 **/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program;                                              *
 *   if not, write to the Free Software Foundation, Inc.,                  *
 *   59 Temple Place, Suite 330, Boston, MA  02111-1307  USA               *
 *                                                                         *
 ***************************************************************************/

#include <stdio.h>
#include <string.h>
#include <netinet/in.h>

#include <string>

#include <glcdgraphics/bitmap.h>
#include <glcdgraphics/image.h>

#include "tuxbox.h"

#pragma pack(1)
struct ani_header {
    unsigned char magic[4]; // = "LCDA"
    unsigned short format; // Format
    unsigned short width;  // Breite
    unsigned short height; // Höhe
    unsigned short count;  // Anzahl Einzelbilder
    unsigned long delay;  // µs zwischen Einzelbildern
};
#pragma pack()

cTuxBoxFile::cTuxBoxFile()
{
}

cTuxBoxFile::~cTuxBoxFile()
{
}

bool cTuxBoxFile::Load(GLCD::cImage & image, const std::string & fileName)
{
    bool ret = false;
    FILE * fIN;
    long fileLen;
    struct ani_header header;
    bool bInvert = false;

    fIN = fopen(fileName.c_str(), "rb");
    if (fIN)
    {
        // get len of file
        if (fseek(fIN, 0, SEEK_END))
        {
            fclose(fIN);
            return false;
        }
        fileLen = ftell(fIN);

        // rewind and get Header
        if (fseek(fIN, 0, SEEK_SET))
        {
            fclose(fIN);
            return false;
        }

        // Read header
        if (fread(&header, sizeof(header), 1, fIN) != 1)
        {
            fclose(fIN);
            return false;
        }

        image.Clear();
        image.SetWidth(ntohs(header.width));
        image.SetHeight(ntohs(header.height));
        image.SetDelay(ntohl(header.delay) / 1000);

        // check Header
        if (strncmp((const char*)header.magic, "LCDA", sizeof(header.magic)) ||
                !image.Width() || !image.Height() || ntohs(header.format) != 0)
        {
            fprintf(stderr, "ERROR: load %s failed, wrong header.\n", fileName.c_str());
            fclose(fIN);
            return false;
        }

        //fprintf(stderr,"%d %dx%d (%d %d) %d\n",ntohs(header.count),image.Width(),image.Height(),fileLen, ( (ntohs(header.count) * (image.Width() * ((image.Height() + 7) / 8))) + sizeof(header)),lhdr.delay);

        // check file length
        if (!ntohs(header.count)
                || (fileLen != (long) ( (ntohs(header.count) * (image.Width() * ((image.Height() + 7) / 8))) + sizeof(header))))
        {
            fprintf(stderr, "ERROR: load %s failed, wrong size.\n", fileName.c_str());
            fclose(fIN);
            return false;
        }
        // Set minimal limit for next image
        if (image.Delay() < 10)
            image.SetDelay(10);
        for (unsigned int n=0;n<ntohs(header.count);++n)
        {
            ret = false;
//            unsigned int nBmpSize = image.Height() * ((image.Width() + 7) / 8);
            unsigned int nBmpSize = image.Height() * image.Width();
//            unsigned char *bitmap = new unsigned char[nBmpSize];
            uint32_t *bitmap = new uint32_t [nBmpSize];
            if (!bitmap)
            {
                fprintf(stderr, "ERROR: malloc failed.");
                break;
            }
//            unsigned int nAniSize = image.Width() * ((image.Height() + 7) / 8);
            unsigned int nAniSize = image.Width() * image.Height();
//            unsigned char *pAni = new unsigned char[nAniSize];
            uint32_t *pAni = new uint32_t [nAniSize];
            if (!pAni)
            {
                delete[] bitmap;
                fprintf(stderr, "ERROR: malloc failed.");
                break;
            }

            if (1 != fread(pAni, nAniSize, 1, fIN))
            {
                fprintf(stderr,"ERROR: Cannot read filedata: %s\n", fileName.c_str());
                delete[] bitmap;
                delete[] pAni;
                break;
            }

            vert2horz(pAni,bitmap, image.Width(), image.Height());
            delete[] pAni;

            if (bInvert)
                for (unsigned int i=0;i<nBmpSize;++i)
                    bitmap[i] ^= 0xFF;

            image.AddBitmap(new GLCD::cBitmap(image.Width(), image.Height(), bitmap));
            ret = true;
        }
        fclose(fIN);
        if (!ret)
            image.Clear();
    }
    return ret;
}


bool cTuxBoxFile::Save(GLCD::cImage & image, const std::string & fileName)
{
    FILE *      fOut;
    struct ani_header header;
    bool bRet = false;

    if (image.Count() > 0
        && image.Width()
        && image.Height())
    {
        memcpy(header.magic, "LCDA", 4);
        header.format = htons(0);
        header.width = htons(image.Width());
        header.height = htons(image.Height());
        header.count = htons(image.Count());
        header.delay = htonl(image.Delay() * 1000);


        if (image.Width() != 120 || image.Height() != 64)
        {
            fprintf(stderr,"WARNING: Maybe wrong image dimension (for all I know is 120x64 wanted) %s\n", fileName.c_str());
        }

        fOut = fopen(fileName.c_str(), "wb");
        if (!fOut) {
            fprintf(stderr,"ERROR: Cannot create file: %s\n", fileName.c_str());
            return false;
        }

        if (1 != fwrite(&header, sizeof(header), 1, fOut))
        {
            fprintf(stderr,"ERROR: Cannot write fileheader: %s\n", fileName.c_str());
            fclose(fOut);
            return false;
        }

        for (unsigned int n = 0; n < image.Count(); n++)
        {
            bRet = false;
            unsigned int nAniSize = image.Width() * ((image.Height() + 7) / 8);
//            unsigned char *pAni = new unsigned char[nAniSize];
            uint32_t *pAni = new uint32_t [nAniSize];
            if (!pAni)
            {
                fprintf(stderr, "ERROR: malloc failed.");
                break;
            }
            horz2vert(image.GetBitmap(n)->Data(), pAni, image.Width(), image.Height());

            if (1 != fwrite(pAni, nAniSize, 1,  fOut))
            {
                delete [] pAni;
                fprintf(stderr,"ERROR: Cannot write filedata: %s\n", fileName.c_str());
                break;
            }
            delete [] pAni;
            bRet = true;
        }

        fclose(fOut);
    }
    return bRet;
}

/** Translate memory alignment from vertical to horizontal
rotate from {Byte} to {Byte}
{o}[o][o][o][o][o][o][o] => { oooooooo }
{o}[o][o][o][o][o][o][o] => [ oooooooo ]
{o}[o][o][o][o][o][o][o] => [ oooooooo ]
{o}[o][o][o][o][o][o][o] => [ oooooooo ]
{o}[o][o][o][o][o][o][o] => [ oooooooo ]
{o}[o][o][o][o][o][o][o] => [ oooooooo ]
{o}[o][o][o][o][o][o][o] => [ oooooooo ]
{o}[o][o][o][o][o][o][o] => [ oooooooo ]*/
//void cTuxBoxFile::vert2horz(const unsigned char* source, unsigned char* dest, int width, int height) {
void cTuxBoxFile::vert2horz(const uint32_t *source, uint32_t *dest, int width, int height) {
    int x, y, off;
    memset(dest,0,height*((width+7)/8));

    for (y=0; y<height; ++y)
    {
        for (x=0; x<width; ++x)
        {
            off = x + ((y/8) * width);
            if (source[off] & (0x1 << (y % 8)))
            {
                off = (x / 8) + (y * ((width+7)/8));
                dest[off] |= (unsigned char)(0x80 >> (x % 8));
            }
        }
    }
}

/** Translate memory alignment from horizontal to vertical (rotate byte)
rotate from {Byte} to {Byte}
{ oooooooo } => {o}[o][o][o][o][o][o][o]
[ oooooooo ] => {o}[o][o][o][o][o][o][o]
[ oooooooo ] => {o}[o][o][o][o][o][o][o]
[ oooooooo ] => {o}[o][o][o][o][o][o][o]
[ oooooooo ] => {o}[o][o][o][o][o][o][o]
[ oooooooo ] => {o}[o][o][o][o][o][o][o]
[ oooooooo ] => {o}[o][o][o][o][o][o][o]
[ oooooooo ] => {o}[o][o][o][o][o][o][o]*/
//void cTuxBoxFile::horz2vert(const unsigned char* source, unsigned char* dest, int width, int height) {
void cTuxBoxFile::horz2vert(const uint32_t *source, uint32_t *dest, int width, int height) {
    int x, y, off;
    memset(dest,0,width*((height+7)/8));

    for (y=0; y<height; ++y)
    {
        for (x=0; x<width; ++x)
        {
            off = (x / 8) + ((y) * ((width+7)/8));
            if (source[off] & (0x80 >> (x % 8)))
            {
                off = x + ((y/8) * width);
                dest[off] |= (unsigned char)(0x1 << (y % 8));
            }
        }
    }
}
