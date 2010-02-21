/*
 * GraphLCD driver library
 *
 * simlcd.c  -  SimLCD driver class
 *              Output goes to a file instead of lcd.
 *              Use SimLCD tool to view this file.
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2001-2004 Carsten Siebholz <c.siebholz AT t-online.de>
 */

#include <stdio.h>
#include <syslog.h>

#include "common.h"
#include "config.h"
#include "simlcd.h"


namespace GLCD
{

cDriverSimLCD::cDriverSimLCD(cDriverConfig * config)
:   config(config)
{
    oldConfig = new cDriverConfig(*config);
}

cDriverSimLCD::~cDriverSimLCD()
{
    delete oldConfig;
}

int cDriverSimLCD::Init()
{
    width = config->width;
    if (width <= 0)
        width = 240;
    height = config->height;
    if (height <= 0)
        height = 128;

    for (unsigned int i = 0; i < config->options.size(); i++)
    {
        if (config->options[i].name == "")
        {
        }
    }

    // setup lcd array
    LCD = new unsigned char *[(width + 7) / 8];
    if (LCD)
    {
        for (int x = 0; x < (width + 7) / 8; x++)
        {
            LCD[x] = new unsigned char[height];
            memset(LCD[x], 0, height);
        }
    }

    *oldConfig = *config;

    // clear display
    Clear();

    syslog(LOG_INFO, "%s: SIMLCD initialized.\n", config->name.c_str());
    return 0;
}

int cDriverSimLCD::DeInit()
{
    // free lcd array
    if (LCD)
    {
        for (int x = 0; x < (width + 7) / 8; x++)
        {
            delete[] LCD[x];
        }
        delete[] LCD;
    }

    return 0;
}

int cDriverSimLCD::CheckSetup()
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

void cDriverSimLCD::Clear()
{
    for (int x = 0; x < (width + 7) / 8; x++)
        memset(LCD[x], 0, height);
}

void cDriverSimLCD::Set8Pixels(int x, int y, unsigned char data)
{
    if (x >= width || y >= height)
        return;

    if (!config->upsideDown)
    {
        // normal orientation
        LCD[x / 8][y] = LCD[x / 8][y] | data;
    }
    else
    {
        // upside down orientation
        x = width - 1 - x;
        y = height - 1 - y;
        LCD[x / 8][y] = LCD[x / 8][y] | ReverseBits(data);
    }
}

void cDriverSimLCD::Refresh(bool refreshAll)
{
    FILE * fp = NULL;
    int x;
    int y;
    int i;
    unsigned char c;

    if (CheckSetup() > 0)
        refreshAll = true;

    fp = fopen("/tmp/simlcd.sem", "r");
    if (!fp || refreshAll)
    {
        if (fp)
            fclose(fp);
        fp = fopen("/tmp/simlcd.dat", "w");
        if (fp)
        {
            for (y = 0; y < height; y++)
            {
                for (x = 0; x < (width + 7) / 8; x++)
                {
                    c = LCD[x][y] ^ (config->invert ? 0xff : 0x00);
                    for (i = 0; i < 8; i++)
                    {
                        if (c & 0x80)
                        {
                            fprintf(fp,"#");
                        }
                        else
                        {
                            fprintf(fp,".");
                        }
                        c = c << 1;
                    }
                }
                fprintf(fp,"\n");
            }
            fclose(fp);
        }

        fp = fopen("/tmp/simlcd.sem", "w");
        fclose(fp);
    }
    else
    {
        fclose(fp);
    }
}

} // end of namespace
