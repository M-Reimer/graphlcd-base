/*
 * GraphLCD graphics library
 *
 * pbm.c  -  PBM file loading and saving
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2006 Andreas Regel <andreas.regel AT powarman.de>
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
    unsigned char * bmpdata = new unsigned char[h * ((w + 7) / 8)];
    if (bmpdata)
    {
        if (fread(bmpdata, h * ((w + 7) / 8), 1, pbmFile) != 1)
        {
            delete[] bmpdata;
            fclose(pbmFile);
            image.Clear();
            return false;
        }
        image.AddBitmap(new cBitmap(w, h, bmpdata));
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

    if (image.Count() == 1)
    {
        fp = fopen(fileName.c_str(), "wb");
        if (fp)
        {
            bitmap = image.GetBitmap(0);
            if (bitmap)
            {
                sprintf(str, "P4\n%d %d\n", bitmap->Width(), bitmap->Height());
                fwrite(str, strlen(str), 1, fp);
                fwrite(bitmap->Data(), bitmap->LineSize() * bitmap->Height(), 1, fp);
            }
            fclose(fp);
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
                if (bitmap)
                {
                    sprintf(str, "P4\n%d %d\n", bitmap->Width(), bitmap->Height());
                    fwrite(str, strlen(str), 1, fp);
                    fwrite(bitmap->Data(), bitmap->LineSize() * bitmap->Height(), 1, fp);
                }
                fclose(fp);
            }
        }
    }
    return true;
}

} // end of namespace

