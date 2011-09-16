/*
* GraphLCD driver library
*
* g15daemon.c  -  pseudo device for the g15daemon meta driver
*                   Output goes to the g15daemon which then displays it
*
* This file is released under the GNU General Public License. Refer
* to the COPYING file distributed with this package.
*
* (c) 2005-2010 Andreas Regel <andreas.regel AT powarman.de>
* (c) 2011      Wolfgang Astleitner <mrwastl AT users.sourceforge.net>
*/

#include <fcntl.h>
#include <stdio.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <poll.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

#include "common.h"
#include "config.h"

#include "g15daemon.h"

#define G15SERVER_PORT 15550
#define G15SERVER_ADDR "127.0.0.1"

#define G15_WIDTH 160
#define G15_HEIGHT 43


static int g15_send(int sock, const char *buf, int len)
{
    int total = 0;
    int retval = 0;
    int bytesleft = len;

    while (total < len) {
        retval = send(sock, buf+total, bytesleft, 0);
        if (retval == -1) {
            break;
        }
        bytesleft -= retval;
        total += retval;
    }
    return retval==-1?-1:0;
}

static int g15_recv(int sock, char *buf, int len)
{
    int total = 0;
    int retval = 0;
    int bytesleft = len;

    while (total < len) {
        retval = recv(sock, buf+total, bytesleft, 0);
        if (retval < 1) {
            break;
        }
        total += retval;
        bytesleft -= retval;
    }
    return total;
}

static int open_g15_daemon()
{
    int g15screen_fd;
    struct sockaddr_in serv_addr;

    char buffer[256];

    g15screen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (g15screen_fd < 0)
        return -1;

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family      = AF_INET;
    inet_aton (G15SERVER_ADDR, &serv_addr.sin_addr);
    serv_addr.sin_port        = htons(G15SERVER_PORT);

    if (connect(g15screen_fd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
        return -1;

    memset(buffer,0,256);
    if (g15_recv(g15screen_fd, buffer, 16)<0)
        return -1;

    /* here we check that we're really talking to the g15daemon */
    if (strcmp(buffer,"G15 daemon HELLO") != 0)
        return -1;

    /* we want to use a pixelbuffer */
    g15_send(g15screen_fd,"GBUF",4);

    return g15screen_fd;
}


namespace GLCD
{

cDriverG15daemon::cDriverG15daemon(cDriverConfig * config)
:   cDriver(config),
    offbuff(0),
    sockfd(-1)
{
}

int cDriverG15daemon::Init()
{
    // default values
    width = config->width;
    if (width !=G15_WIDTH)
        width = G15_WIDTH;
    height = config->height;
    if (height !=G15_HEIGHT)
        height = G15_HEIGHT;

    for (unsigned int i = 0; i < config->options.size(); i++) {
        if (config->options[i].name == "") {
        }
    }

    screensize = 6880;

    if ((sockfd = open_g15_daemon())<0)
        return -1;
    // reserve memory to draw into
    offbuff = new char[6880];

    *oldConfig = *config;

    // clear display
    Refresh(true);

    syslog(LOG_INFO, "%s: g15daemon initialized.\n", config->name.c_str());
    return 0;
}

int cDriverG15daemon::DeInit()
{
    if (offbuff);
    delete[] offbuff;
    if (-1 != sockfd)
        close(sockfd);

    return 0;
}

int cDriverG15daemon::CheckSetup()
{
    if (config->device != oldConfig->device ||
        config->port != oldConfig->port ||
        config->width != oldConfig->width ||
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

void cDriverG15daemon::SetPixel(int x, int y, uint32_t data)
{
    if (x >= width || y >= height)
        return;

    if (config->upsideDown)
    {
        x = width - 1 - x;
        y = height - 1 - y;
    }

    offbuff[x + (width * y)] = ( (data == GRAPHLCD_White) ? 1 : 0 );
}

void cDriverG15daemon::Clear()
{
    memset(offbuff, 0, screensize);
}

#if 0
void cDriverG15daemon::Set8Pixels(int x, int y, unsigned char data)
{
    int n;

    x &= 0xFFF8;

    for (n = 0; n < 8; ++n)
    {
        if (data & (0x80 >> n))      // if bit is set
            SetPixel(x + n, y, GRAPHLCD_White);
    }
}
#endif

void cDriverG15daemon::Refresh(bool refreshAll)
{
    g15_send(sockfd, offbuff, screensize);
}

} // end of namespace
