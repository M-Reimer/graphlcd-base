/**
 *  GraphLCD driver library
 *
 *  st7565-reel.c - Plugin for ST7565 display on Reelbox
 *                  driven by an AVR on the front panel
 *
 *  (c) 2004 Georg Acher, BayCom GmbH, http://www.baycom.de
 *      based on simlcd.c by Carsten Siebholz
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

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <termios.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <inttypes.h>

#include "common.h"
#include "config.h"

#include "st7565r-reel.h"
#include "port.h"


namespace GLCD
{

cDriverST7565RReel::cDriverST7565RReel(cDriverConfig * config) : cDriver(config)
{
    port = new cSerialPort();
}

cDriverST7565RReel::~cDriverST7565RReel()
{
    delete port;
}

int cDriverST7565RReel::Init(void)
{
    width = config->width;
    if (width < 0)
        width = 128;
    height = config->height;
    if (height < 0)
        height = 64;

    // setup lcd array
    LCD = new uint32_t *[width];
    if (LCD)
    {
        for (int x = 0; x < width; x++)
        {
            LCD[x] = new uint32_t[height];
        }
    }

    if (port->Open(config->device.c_str()) != 0)
        return -1;

    port->SetBaudRate(115200);

    *oldConfig = *config;
    set_displaymode(0);
    set_clock();
    // clear display
    Clear();

    syslog(LOG_INFO, "%s: ST7565R initialized.\n", config->name.c_str());
    return 0;
}

int cDriverST7565RReel::DeInit(void)
{
    // clear_display();
    // set_displaymode(1); // Clock
    // syslog(LOG_INFO, "deint.\n");

    // free lcd array
    if (LCD)
    {
        for (int x = 0; x < width; x++)
        {
            delete[] LCD[x];
        }
        delete[] LCD;
    }

    if (port->Close() != 0)
        return -1;

    return 0;
}


int cDriverST7565RReel::CheckSetup(void)
{
    if (config->device != oldConfig->device ||
        config->width != oldConfig->width ||
        config->height != oldConfig->height)
    {
        DeInit();
        Init();
        return 0;
    }

    if (config->brightness != oldConfig->brightness)
    {
        oldConfig->brightness = config->brightness;
        SetBrightness(config->brightness);
        usleep(500000);
    }

    if (config->contrast != oldConfig->contrast)
    {
        oldConfig->contrast = config->contrast;
        SetContrast(config->contrast);
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


void cDriverST7565RReel::Clear(void)
{
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            LCD[x][y] = GRAPHLCD_White;
        }
    }
//    syslog(LOG_INFO, "Clear.\n");
}

void cDriverST7565RReel::SetPixel(int x, int y, uint32_t data)
{
    if (x >= width || y >= height)
        return;

    int pos = x % 8;
    if (config->upsideDown)
    {
        x = width - 1 - x;
        y = height - 1 - y;
    }
    else
        pos = 7 - pos; // reverse bit position

    if (data == GRAPHLCD_White)
        LCD[x / 8][y] |= (1 << pos);
    else
        LCD[x / 8][y] &= ( 0xFF ^ (1 << pos) );
}

void cDriverST7565RReel::Refresh(bool refreshAll)
{
    int x,y,xx,yy;
    int i;
    unsigned char c;
    int rx;
    int invert;

    CheckSetup();

    if (!LCD)
        return;

    invert=(config->invert != 0) ? 0xff : 0x00;

    for (y = 0; y < (height); y+=8)
    {
        display_cmd( 0xb0+((y/8)&15));
        for (x = 0; x < width / 8; x+=4)
        {
            unsigned char d[32]={0};

            for(yy=0;yy<8;yy++)
            {
                for (xx=0;xx<4;xx++)
                {
                    c = (LCD[x+xx][y+yy])^invert;

                    for (i = 0; i < 8; i++)
                    {
                        d[i+xx*8]>>=1;
                        if (c & 0x80)
                            d[i+xx*8]|=0x80;
                        c<<=1;
                    }
                }
            }

            rx=4+x*8;
            //    printf("X %i, y%i\n",rx,y);

            display_cmd( 0x10+((rx>>4)&15));
            display_cmd( 0x00+(rx&15));
            display_data( d,32);
        }
    }

// syslog(LOG_INFO, "refresh.\n");
}

void cDriverST7565RReel::Set8Pixels(int x, int y, unsigned char data)
{
    if (!LCD)
        return;

    clip(x, 0, width - 1);
    clip(y, 0, height - 1);

    if (!config->upsideDown)
    {
        // normal orientation
        //    syslog(LOG_INFO, "%s: set8pixel normal.\n");
        LCD[x / 8][y] = LCD[x / 8][y] | data;
    }
    else
    {
        // upside down orientation
        x = width - 1 - x;
        y = height - 1 - y;
        //    syslog(LOG_INFO, "%s: set8pixel flip.\n", LCD);
        LCD[x /8][y] = LCD[x / 8][y] | ReverseBits(data);
    }
}

void cDriverST7565RReel::display_cmd(unsigned char cmd)
{
    unsigned char buf[]={0xa5, 0x05, 3, 0, cmd};
    port->WriteData(buf, 5);
//    syslog(LOG_INFO, "%s: display cmd.\n", cmd);
}

void cDriverST7565RReel::display_data(unsigned char *data, unsigned char l)
{
    if (l > 60)
    {
        syslog(LOG_ERR, "cDriverST7565RReel::display_data buffer length exceeded!");
    }

    unsigned char buf[64]={0xa5,0x05,(unsigned char)(l+2),+1};
    memcpy(buf+4,data,l);
    port->WriteData(buf, l+4);
//    syslog(LOG_INFO, "%s: display data.\n", data);
}
void cDriverST7565RReel::set_displaymode(unsigned char m)
{
    unsigned char buf[]={0xa5,0x09,m};
    port->WriteData(buf, 3);
//    syslog(LOG_INFO, "displaymode.\n");
}

void cDriverST7565RReel::set_clock(void)
{
    time_t t;
    struct tm tm;
    t=time(0);
    localtime_r(&t,&tm);

    unsigned char buf[]={0xa5,0x7,
                         (unsigned char)tm.tm_hour,
                         (unsigned char)tm.tm_min,
                         (unsigned char)tm.tm_sec,
                         (unsigned char)(t>>24),
                         (unsigned char)(t>>16),
                         (unsigned char)(t>>8),
                         (unsigned char)t};
    port->WriteData(buf, 9);
//    syslog(LOG_INFO, "set_clock cmd.\n");
}

void cDriverST7565RReel::clear_display(void)
{
    unsigned char buf[]={0xa5,0x04};
    port->WriteData(buf, 2);
//    syslog(LOG_INFO, "clear_display cmd.\n");
}

void cDriverST7565RReel::SetBrightness(unsigned int percent)
{
    unsigned char buf[]={0xa5,0x02, 0x00, 0x00};
    int n=static_cast<int>(percent*2.5);
    if (n>255)
        n=255;
    buf[2]=(char)(n);
    port->WriteData(buf,4);
}

void cDriverST7565RReel::SetContrast(unsigned int val)
{
    unsigned char buf[]={0xa5,0x03, 0x00, 0x00};
    buf[2]=(char)(val*25);
    port->WriteData(buf,4);
}


}
