/**
 *  GraphLCD plugin for the Video Disk Recorder
 *
 *  bmp.c  -  bmp logo class
 *
 *  (C) 2004 Andreas Brachold <vdr04 AT deltab de>
 *  (C) 2001-2004 Carsten Siebholz <c.siebholz AT t-online de>
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
#include <stdint.h>
#include <string.h>
#include <cstring>
#include <cstdlib>

#include <glcdgraphics/bitmap.h>
#include <glcdgraphics/image.h>

#include "bmp.h"


#pragma pack(1)
typedef struct BMPH {
    uint16_t  bmpIdentifier;
    uint32_t  bmpFileSize;
    uint32_t  bmpReserved;
    uint32_t  bmpBitmapDataOffset;
    uint32_t  bmpBitmapHeaderSize;
    uint32_t  bmpWidth;
    uint32_t  bmpHeight;
    uint16_t  bmpPlanes;
    uint16_t  bmpBitsPerPixel;
    uint32_t  bmpCompression;
    uint32_t  bmpBitmapDataSize;
    uint32_t  bmpHResolution;
    uint32_t  bmpVResolution;
    uint32_t  bmpColors;
    uint32_t  bmpImportantColors;
} BMPHEADER;  // 54 bytes

typedef struct RGBQ {
    uint8_t  rgbBlue;
    uint8_t  rgbGreen;
    uint8_t  rgbRed;
    uint8_t  rgbReserved;
} RGBQUAD;    // 4 bytes
#pragma pack()


uint8_t bitmask[8]  = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
uint8_t bitmaskl[8] = {0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff};
uint8_t bitmaskr[8] = {0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01};

cBMPFile::cBMPFile()
{
}

cBMPFile::~cBMPFile()
{
}

bool cBMPFile::Load(GLCD::cImage & image, const std::string & fileName)
{
    FILE         *fIN;
    BMPHEADER     bmpHeader;
    RGBQUAD      *pPalette;
    char         *pByte;
    char          Dummy;
    long          iNumColors;
    long          iSize;
    uint32_t      x, y;
    uint16_t      iRead;
//    uint8_t *  bitmap = NULL;
    uint32_t *bitmap = NULL;
    bool  bInvert = false;

    if (fileName.length() > 0)
    {
        fIN = fopen(fileName.c_str(), "rb");
        if (fIN)
        {
            if (fread(&bmpHeader, sizeof(BMPHEADER), 1, fIN)!=1)
            {
                fclose(fIN);
                return false;
            }

            // check for Windows BMP
            if (bmpHeader.bmpBitmapHeaderSize != 0x00000028 )
            {
                fprintf(stderr, "ERROR: only Windows BMP images are allowed.\n");
                fclose(fIN);
                return false;
            }

            // check for 2 color
            iNumColors = (1 << bmpHeader.bmpBitsPerPixel);
            if (iNumColors != 2)
            {
                fprintf(stderr, "ERROR: the image has %ld colors, but only images with 2 colors are allowed.\n", iNumColors);
                fclose(fIN);
                return false;
            }

            iSize = bmpHeader.bmpHeight * bmpHeader.bmpWidth;

            pPalette = (RGBQUAD *) malloc( iNumColors*sizeof(RGBQUAD));
            if (!pPalette)
            {
                fprintf(stderr, "ERROR: cannot allocate memory\n");
                fclose(fIN);
                return false;
            }

            if (fread( pPalette, iNumColors*sizeof(RGBQUAD), 1, fIN)!=1)
            {
                free(pPalette);
                fclose(fIN);
                return false;
            }

            // check colors
            if (pPalette->rgbBlue+pPalette->rgbGreen+pPalette->rgbRed <
                    (pPalette+1)->rgbBlue+(pPalette+1)->rgbGreen+(pPalette+1)->rgbRed)
            {
                // index 0 represents 'black', index 1 'white'
                bInvert = !bInvert;
            }
            else
            {
                // index 0 represents 'white', index 1 'black'
            }

            if (fseek(fIN, bmpHeader.bmpBitmapDataOffset, SEEK_SET)==EOF)
            {
                free(pPalette);
                fclose(fIN);
                return false;
            }

            switch (bmpHeader.bmpCompression)
            {
                case 0: // BI_RGB       no compression
                    image.Clear();
                    image.SetWidth(bmpHeader.bmpWidth);
                    image.SetHeight(bmpHeader.bmpHeight);
                    image.SetDelay(100);
//                    bitmap = new unsigned char[bmpHeader.bmpHeight * ((bmpHeader.bmpWidth + 7) / 8)];
                    bitmap = new uint32_t [bmpHeader.bmpHeight * bmpHeader.bmpWidth];
                    if (!bitmap)
                    {
                        fprintf(stderr, "ERROR: cannot allocate memory\n");
                        free(pPalette);
                        fclose(fIN);
                        image.Clear();
                        return false;
                    }

                    for (y = bmpHeader.bmpHeight; y > 0; y--)
                    {
                        pByte = (char*)bitmap + (y-1)*((bmpHeader.bmpWidth+7)/8);
                        iRead = 0;
                        for (x = 0; x < bmpHeader.bmpWidth / 8; x++)
                        {
                            if (fread(pByte, sizeof(char), 1, fIN) != 1)
                            {
                                delete[] bitmap;
                                free(pPalette);
                                fclose(fIN);
                                image.Clear();
                                return false;
                            }
                            iRead++;
                            if (bInvert)
                                *pByte = *pByte ^ 0xff;
                            pByte++;
                        }

                        if (bmpHeader.bmpWidth % 8)
                        {
                            if (fread(pByte, sizeof(char), 1, fIN) != 1)
                            {
                                delete [] bitmap;
                                free(pPalette);
                                fclose(fIN);
                                image.Clear();
                                return false;
                            }
                            iRead++;
                            if (bInvert)
                                *pByte = *pByte^0xff;
                            *pByte = *pByte & bitmaskl[bmpHeader.bmpWidth%8];
                            pByte++;
                        }

                        // Scan line must be 4-byte-alligned
                        while (iRead % 4)
                        {
                            if (fread(&Dummy, sizeof(char), 1, fIN) != 1)
                            {
                                delete [] bitmap;
                                free(pPalette);
                                fclose(fIN);
                                image.Clear();
                                return false;
                            }
                            iRead++;
                        }
                    }
                    image.AddBitmap(new GLCD::cBitmap(bmpHeader.bmpWidth, bmpHeader.bmpHeight, bitmap));
                    break;
                case 1: // BI_RLE4      RLE 4bit/pixel
                case 2: // BI_RLE8      RLE 8bit/pixel
                case 3: // BI_BITFIELDS
                default:
                    fprintf(stderr, "ERROR: only uncompressed RGB images are allowed.\n");

                    free(pPalette);
                    fclose(fIN);
                    return false;
            }
            fclose(fIN);
        }
        else
        {
            fprintf(stderr, "ERROR: cannot open picture %s\n", fileName.c_str());
        }
    }
    else
    {
        fprintf(stderr, "ERROR: no FileName given!\n");
    }
    return true;
}

bool cBMPFile::Save(const GLCD::cBitmap * bitmap, const std::string & fileName)
{
    FILE         *fOut;
    BMPHEADER     bmpHeader;
    RGBQUAD       bmpColor1, bmpColor2;
    uint32_t      dBDO, dBDSx, dBDS;
    char         *pByte;
    char          Dummy = 0x00;
    uint32_t      x, y;
    uint16_t      iWrote;
//    const uint8_t * bmpdata = bitmap->Data();
    const uint32_t * bmpdata = bitmap->Data();

    if (bitmap
        && bitmap->Width() > 0
        && bitmap->Height() > 0)
    {
        memset(&bmpHeader, 0, sizeof(BMPHEADER));

        dBDO = sizeof(BMPHEADER)+2*sizeof(RGBQUAD);
        dBDSx = ((bitmap->Width() + 7) / 8 + 3) & 0xfffffffc;
        dBDS = dBDSx * bitmap->Height();

        bmpHeader.bmpIdentifier       = 0x4d42; // "BM"
        bmpHeader.bmpFileSize         = dBDO + dBDS;
        bmpHeader.bmpBitmapDataOffset = dBDO;
        bmpHeader.bmpBitmapHeaderSize = 0x28;
        bmpHeader.bmpWidth            = bitmap->Width();
        bmpHeader.bmpHeight           = bitmap->Height();
        bmpHeader.bmpPlanes           = 0x01;
        bmpHeader.bmpBitsPerPixel     = 0x01;
        bmpHeader.bmpCompression      = 0x00;
        bmpHeader.bmpBitmapDataSize   = dBDS;
        bmpHeader.bmpHResolution      = 0xb13;  // 72dpi
        bmpHeader.bmpVResolution      = 0xb13;  // 72dpi
        bmpHeader.bmpColors           = 0x02;
        bmpHeader.bmpImportantColors  = 0x02;

        bmpColor1.rgbBlue     = 0x00;
        bmpColor1.rgbGreen    = 0x00;
        bmpColor1.rgbRed      = 0x00;
        bmpColor1.rgbReserved = 0x00;
        bmpColor2.rgbBlue     = 0xff;
        bmpColor2.rgbGreen    = 0xff;
        bmpColor2.rgbRed      = 0xff;
        bmpColor2.rgbReserved = 0x00;


        fOut = fopen(fileName.c_str(), "wb");
        if (!fOut)
        {
            fprintf(stderr,"Cannot create file: %s\n", fileName.c_str());
            return false;
        }
        fwrite(&bmpHeader, sizeof(BMPHEADER), 1, fOut);
        fwrite(&bmpColor1, sizeof(RGBQUAD), 1, fOut);
        fwrite(&bmpColor2, sizeof(RGBQUAD), 1, fOut);

        for (y=bitmap->Height(); y>0; y--)
        {
            pByte = (char*)bmpdata + (y-1)*((bitmap->Width()+7)/8);
            iWrote = 0;
            for (x=0; x<(uint32_t) bitmap->Width()/8; x++)
            {
                *pByte = *pByte^0xff;
                if (fwrite(pByte, sizeof(char), 1, fOut)!=1)
                {
                    fclose(fOut);
                    return false;
                }
                iWrote++;
                pByte++;
            }
            // Scan line must be 4-byte-alligned
            while (iWrote%4)
            {
                if (fwrite(&Dummy, sizeof(char), 1, fOut)!=1)
                {
                    fclose(fOut);
                    return 3;
                }
                iWrote++;
            }
        }
        fclose(fOut);
    }
    return true;
}

bool cBMPFile::Save(GLCD::cImage & image, const std::string & fileName)
{
    const GLCD::cBitmap * bitmap;

    if (image.Count() == 1)
    {
        bitmap = image.GetBitmap(0);
        if (bitmap)
        {
            if (!Save(bitmap, fileName))
            {
                return false;
            }
        }
    }
    else
    {
        uint16_t i;
        char tmpStr[256];

        for (i = 0; i < image.Count(); i++)
        {
            sprintf(tmpStr, "%.248s.%05d", fileName.c_str(), i);
            bitmap = image.GetBitmap(i);
            if (bitmap)
            {
                if (!Save(bitmap, tmpStr))
                {
                    return false;
                }
            }
        }
    }
    return true;
}
