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
 * (c) 2011      Dirk Heiser
 * (c) 2011      Wolfgang Astleitner <mrwastl AT users.sourceforge.net>
 */

#include <stdio.h>
#include <syslog.h>
#include <cstring>

#include "common.h"
#include "config.h"
#include "simlcd.h"

#define DISPLAY_REFRESH_FILE "/tmp/simlcd.sem"
#define DISPLAY_DATA_FILE    "/tmp/simlcd.dat"
#define TOUCH_REFRESH_FILE   "/tmp/simtouch.sem"
#define TOUCH_DATA_FILE      "/tmp/simtouch.dat"

#define FG_CHAR "#"
#define BG_CHAR "."

namespace GLCD
{

cDriverSimLCD::cDriverSimLCD(cDriverConfig * config) : cDriver(config)
{
}

int cDriverSimLCD::Init(void)
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
    LCD = new uint32_t *[width];
    if (LCD)
    {
        for (int x = 0; x < width; x++)
        {
            LCD[x] = new uint32_t[height];
            //memset(LCD[x], 0, height);
        }
    }

    *oldConfig = *config;

    // clear display
    Clear();

    syslog(LOG_INFO, "%s: SIMLCD initialized.\n", config->name.c_str());
    return 0;
}

int cDriverSimLCD::DeInit(void)
{
    // free lcd array
    if (LCD)
    {
        for (int x = 0; x < width; x++)
        {
            delete[] LCD[x];
        }
        delete[] LCD;
    }

    return 0;
}

int cDriverSimLCD::CheckSetup(void)
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

void cDriverSimLCD::Clear(void)
{
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            LCD[x][y] = GRAPHLCD_White; 
        }
    }
}

void cDriverSimLCD::SetPixel(int x, int y, uint32_t data)
{
    if (x >= width || y >= height)
        return;

    if (config->upsideDown)
    {
        // upside down orientation
        x = width - 1 - x;
        y = height - 1 - y;
    }
    LCD[x][y] = data;
}

void cDriverSimLCD::Refresh(bool refreshAll)
{
    FILE * fp = NULL;
    int x;
    int y;

    if (CheckSetup() > 0)
        refreshAll = true;

    fp = fopen(DISPLAY_REFRESH_FILE, "r");
    if (!fp || refreshAll)
    {
        if (fp)
            fclose(fp);
        fp = fopen(DISPLAY_DATA_FILE, "w");
        if (fp)
        {
            for (y = 0; y < height; y++)
            {
                for (x = 0; x < width; x++)
                {
                    if (LCD[x][y] == GRAPHLCD_Black)
                    {
                        if (!config->invert)
                        {
                            fprintf(fp,FG_CHAR);
                        } else
                        {
                            fprintf(fp,BG_CHAR);
                        }
                    }
                    else
                    {
                        if (!config->invert)
                        {
                            fprintf(fp,BG_CHAR);
                        } else
                        {
                            fprintf(fp,FG_CHAR);
                        }
                    }
                }
                fprintf(fp,"\n");
            }
            fclose(fp);
        }

        fp = fopen(DISPLAY_REFRESH_FILE, "w");
        fclose(fp);
    }
    else
    {
        fclose(fp);
    }
}

uint32_t cDriverSimLCD::GetBackgroundColor(void)
{
    return GRAPHLCD_White;
}

bool cDriverSimLCD::GetDriverFeature(const std::string & Feature, int & value)
{
    if (strcasecmp(Feature.c_str(), "depth") == 0) {
        value = 1;
        return true;
    } else if (strcasecmp(Feature.c_str(), "ismonochrome") == 0) {
        value = true;
        return true;
    } else if (strcasecmp(Feature.c_str(), "isgreyscale") == 0 || strcasecmp(Feature.c_str(), "isgrayscale") == 0) {
        value = false;
        return true;
    } else if (strcasecmp(Feature.c_str(), "iscolour") == 0 || strcasecmp(Feature.c_str(), "iscolor") == 0) {
        value = false;
        return true;
    } else if (strcasecmp(Feature.c_str(), "touch") == 0 || strcasecmp(Feature.c_str(), "touchscreen") == 0) {
        value = false;
        return true;
    }
    value = 0;
    return false;
}

} // end of namespace
