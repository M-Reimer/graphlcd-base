/*
 * GraphLCD driver library
 *
 * vncserver.h  -  vncserver device
 *                   Output goes to a vncserver device
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2013      Michael Heyer
 */

#include <fcntl.h>
#include <stdio.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "common.h"
#include "config.h"
#include "vncserver.h"


namespace GLCD
{

cDriverVncServer::cDriverVncServer(cDriverConfig * config)
:   cDriver(config),
    offbuff(0)
{
}

int cDriverVncServer::Init()
{
    printf(" init.\n");

    width = config->width;
    height = config->height;

    // Figure out the size of the screen in bytes
    screensize = width * height * 4;
    depth = 24;
    server = rfbGetScreen(NULL,NULL,width,height,8,3,4);
    if (!server)
    {
        syslog(LOG_ERR, "failed to creat vncserver device.\n");
        return -1;
    }

    for (unsigned int i = 0; i < config->options.size(); i++)
    {
        if (config->options[i].name == "HttpDir")
        {
            server->httpDir = (char *)config->options[i].value.c_str();
        }
    }

    // init bounding box
    bbox[0] = width - 1;  // x top
    bbox[1] = height - 1; // y top
    bbox[2] = 0;          // x bottom
    bbox[3] = 0;          // y bottom

    // reserve another memory to draw into
    offbuff = new char[screensize];
    if (!offbuff)
    {
        syslog(LOG_ERR, "%s: failed to alloc memory for vncserver device.\n", config->name.c_str());
        return -1;
    }

    server->frameBuffer=offbuff;
    rfbInitServer(server);

    *oldConfig = *config;

    // clear display
    Refresh(true);
    
    rfbRunEventLoop(server, 5000, true);

    syslog(LOG_INFO, "%s: VncServer initialized.\n", config->name.c_str());
    return 0;
}

int cDriverVncServer::DeInit()
{
    if (offbuff)
        delete[] offbuff;
    return 0;
}

int cDriverVncServer::CheckSetup()
{
    if (config->device != oldConfig->device ||
        config->port != oldConfig->port ||
        32 != oldConfig->width ||
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

void cDriverVncServer::SetPixel(int x, int y, uint32_t data)
{
    int location;

    if (x >= width || y >= height)
        return;

    if (config->upsideDown)
    {
        x = width - 1 - x;
        y = height - 1 - y;
    }
    
    location = (x + y * width) * 4;
    unsigned char r,g,b;
    r = (data & 0x00FF0000) >> 16;
    g = (data & 0x0000FF00) >> 8;
    b = (data & 0x000000FF) >> 0;
    if (config->invert) {
        r = 255 - r;
        g = 255 - g;
        b = 255 - b;
    }
    *(offbuff + location + 0) = r;
    *(offbuff + location + 1) = g;
    *(offbuff + location + 2) = b;

    if (x < bbox[0]) bbox[0] = x;
    if (y < bbox[1]) bbox[1] = y;
    if (x > bbox[2]) bbox[2] = x;
    if (y > bbox[3]) bbox[3] = y;
}

void cDriverVncServer::Clear()
{
    memset(offbuff, 0, screensize);
    processDamage();
}

void cDriverVncServer::Refresh(bool refreshAll)
{
    if (refreshAll) {
        bbox[0] = 0;
        bbox[1] = 0;
        bbox[2] = width - 1;
        bbox[3] = height - 1;
    }
    processDamage();
}

bool cDriverVncServer::GetDriverFeature  (const std::string & Feature, int & value) {
    if (offbuff) {
        if (strcasecmp(Feature.c_str(), "depth") == 0) {
            value = depth;
            return true;
        } else if (strcasecmp(Feature.c_str(), "ismonochrome") == 0) {
            value = 0;
            return true;
        } else if (strcasecmp(Feature.c_str(), "isgreyscale") == 0 || strcasecmp(Feature.c_str(), "isgrayscale") == 0) {
            value = 0;
            return true;
        } else if (strcasecmp(Feature.c_str(), "iscolour") == 0 || strcasecmp(Feature.c_str(), "iscolor") == 0) {
            value = 1;
            return true;
#if 0
        } else if (strcasecmp(Feature.c_str(), "touch") == 0 || strcasecmp(Feature.c_str(), "touchscreen") == 0) {
            if (...) {
                value = (...) ? 1 : 0;
            }
            return true;
#endif
        }
    }
    value = 0;
    return false;
}


/* defines for different damage processing calls needed by _update() */
void cDriverVncServer::processDamage (void) {
    
    if (!((bbox[0] == (width - 1)) && (bbox[1] == (height - 1)) && (bbox[2] == 0) && (bbox[3] == 0))) {
        rfbMarkRectAsModified(server,bbox[0],bbox[1],bbox[2],bbox[3]);
    }

    /* reset bounding box */
    bbox[0] = width - 1;
    bbox[1] = height - 1;
    bbox[2] = 0;
    bbox[3] = 0;
}

} // end of namespace
