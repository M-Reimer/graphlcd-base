/*
 * GraphLCD driver library
 *
 * image.c  -  Image output device
 *             Output goes to a image file instead of LCD.
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2004 Andreas Regel <andreas.regel AT powarman.de>
 */

#include <stdio.h>
#include <syslog.h>
#include <cstring>

#include "common.h"
#include "config.h"
#include "image.h"


namespace GLCD
{

cDriverImage::cDriverImage(cDriverConfig * config)
:   config(config)
{
    oldConfig = new cDriverConfig(*config);
}

cDriverImage::~cDriverImage()
{
    delete oldConfig;
}

int cDriverImage::Init()
{
    width = config->width;
    if (width <= 0)
        width = 240;
    height = config->height;
    if (height <= 0)
        height = 128;
    lineSize = (width + 7) / 8;

    for (unsigned int i = 0; i < config->options.size(); i++)
    {
        if (config->options[i].name == "")
        {
        }
    }

    newLCD = new unsigned char[lineSize * height];
    if (newLCD)
        memset(newLCD, 0, lineSize * height);
    oldLCD = new unsigned char[lineSize * height];
    if (oldLCD)
        memset(oldLCD, 0, lineSize * height);

    counter = 0;

    *oldConfig = *config;

    // clear display
    Clear();

    syslog(LOG_INFO, "%s: image driver initialized.\n", config->name.c_str());
    return 0;
}

int cDriverImage::DeInit()
{
    if (newLCD)
        delete[] newLCD;
    if (oldLCD)
        delete[] oldLCD;
    return 0;
}

int cDriverImage::CheckSetup()
{
    if (config->width != oldConfig->width ||
        config->height != oldConfig->height)
    {
        DeInit();
        Init();
        return 0;
    }

    if (config->upsideDown != oldConfig->upsideDown ||
        config->invert != oldConfig->invert)
    {
        oldConfig->upsideDown = config->upsideDown;
        oldConfig->invert = config->invert;
        return 1;
    }
    return 0;
}

void cDriverImage::Clear()
{
    memset(newLCD, 0, lineSize * height);
}

void cDriverImage::Set8Pixels(int x, int y, unsigned char data)
{
    if (x >= width || y >= height)
        return;

    if (!config->upsideDown)
    {
        // normal orientation
        newLCD[lineSize * y + x / 8] |= data;
    }
    else
    {
        // upside down orientation
        x = width - 1 - x;
        y = height - 1 - y;
        newLCD[lineSize * y + x / 8] |= ReverseBits(data);
    }
}

void cDriverImage::Refresh(bool refreshAll)
{
    int i;
    bool refresh;
    char fileName[256];
    char str[32];
    FILE * fp;
    unsigned char c;

    refresh = false;
    if (CheckSetup() > 0)
        refresh = true;

    for (i = 0; i < lineSize * height; i++)
    {
        if (newLCD[i] != oldLCD[i])
        {
            refresh = true;
            break;
        }
    }

    if (refresh)
    {
        sprintf(fileName, "%s/%s%05d.%s", "/tmp", "lcd", counter, "pbm");
        fp = fopen(fileName, "wb");
        if (fp)
        {
            sprintf(str, "P4\n%d %d\n", width, height);
            fwrite(str, strlen(str), 1, fp);
            for (i = 0; i < lineSize * height; i++)
            {
                c = newLCD[i] ^ (config->invert ? 0xff : 0x00);
                fwrite(&c, 1, 1, fp);
                oldLCD[i] = newLCD[i];
            }
            fclose(fp);
        }
        counter++;
        if (counter > 99999)
            counter = 0;
    }
}

} // end of namespace
