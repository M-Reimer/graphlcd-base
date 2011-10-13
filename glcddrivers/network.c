/*
 * GraphLCD driver library
 *
 * network.c  -  Network output device
 *               Output goes to a network client.
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2004      Andreas Regel <andreas.regel AT powarman.de>
 * (c) 2011      Wolfgang Astleitner <mrwastl AT users.sourceforge.net>
 */

#include <stdio.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <cstring>

#include "common.h"
#include "config.h"
#include "network.h"


namespace GLCD
{

cDriverNetwork::cDriverNetwork(cDriverConfig * config)
:   cDriver(config)
{
    childTid = 0;
    running = false;
    clientConnected = false;
}

int cDriverNetwork::Init()
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

    *oldConfig = *config;

    // clear display
    Clear();

    running = true;
    if (pthread_create(&childTid, NULL, (void *(*) (void *)) &ServerThread, (void *)this) != 0)
    {
        syslog(LOG_ERR, "%s: error creating server thread.\n", config->name.c_str());
        running = false;
        return 1;
    }
    syslog(LOG_INFO, "%s: network driver initialized.\n", config->name.c_str());
    return 0;
}

int cDriverNetwork::DeInit()
{
    // stop server thread
    running = false;
    usleep(3000000); // wait 3 seconds
    pthread_cancel(childTid);
    childTid = 0;

    if (newLCD)
        delete[] newLCD;
    if (oldLCD)
        delete[] oldLCD;
    return 0;
}

int cDriverNetwork::CheckSetup()
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

void cDriverNetwork::Clear()
{
    memset(newLCD, 0, lineSize * height);
}


void cDriverNetwork::SetPixel(int x, int y, uint32_t data)
{
    if (x >= width || y >= height)
        return;

    int pos = x % 8;
    if (config->upsideDown)
    {
        x = width - 1 - x;
        y = height - 1 - y;
    } else {
        pos = 7 - pos; // reverse bit position
    }

    if (data == GRAPHLCD_White)
        newLCD[lineSize * y + x / 8] |= (1 << pos);
    else
        newLCD[lineSize * y + x / 8] &= ( 0xFF ^ (1 << pos) );
}


#if 0
void cDriverNetwork::Set8Pixels(int x, int y, unsigned char data)
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
#endif

void cDriverNetwork::Refresh(bool refreshAll)
{
    int i;
    bool refresh;

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

    if (refresh && clientConnected)
    {
        char msg[1024];
        int x;
        int y;
        int sent;

        sprintf(msg, "update begin %d %d\r\n", width, height);
        sent = send(clientSocket, msg, strlen(msg), 0);
        if (sent == -1)
        {
            syslog(LOG_ERR, "%s: error sending message: %s.\n", config->name.c_str(), strerror(errno));
            clientConnected = false;
            return;
        }
        for (y = 0; y < height; y++)
        {
            sprintf(msg, "update line %d ", y);
            for (x = 0; x < lineSize; x++)
            {
                char tmp[3];
                sprintf(tmp, "%02X", newLCD[y * lineSize + x]);
                strcat(msg, tmp);
                oldLCD[i] = newLCD[i];
            }
            strcat(msg, "\r\n");
            sent = send(clientSocket, msg, strlen(msg), 0);
            if (sent == -1)
            {
                syslog(LOG_ERR, "%s: error sending message: %s.\n", config->name.c_str(), strerror(errno));
                clientConnected = false;
                return;
            }
        }
        sprintf(msg, "update end\r\n");
        sent = send(clientSocket, msg, strlen(msg), 0);
        if (sent == -1)
        {
            syslog(LOG_ERR, "%s: error sending message: %s.\n", config->name.c_str(), strerror(errno));
            clientConnected = false;
            return;
        }
    }
}

void * cDriverNetwork::ServerThread(cDriverNetwork * Driver)
{
    int serverSocket;
    struct sockaddr_in address;
    socklen_t addrlen;
    int clientSocket;
    fd_set set;
    fd_set setsave;
    struct timeval timeout;

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1)
    {
        syslog(LOG_ERR, "%s: error creating server socket.\n", Driver->config->name.c_str());
        return NULL;
    }

    int y = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(int));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(2003);
    if (bind(serverSocket, (struct sockaddr *) &address, sizeof(address)) != 0)
    {
        syslog(LOG_ERR, "%s: error port %d is already used.\n", Driver->config->name.c_str(), 2003);
        return NULL;
    }

    listen(serverSocket, 1);
    addrlen = sizeof(struct sockaddr_in);

    FD_ZERO(&set);
    FD_SET(serverSocket, &set);
    setsave = set;

    while (Driver->running)
    {
        set = setsave;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        if (select(FD_SETSIZE, &set, NULL, NULL, &timeout) < 0)
        {
            syslog(LOG_ERR, "%s: error during select.\n", Driver->config->name.c_str());
            break;
        }

        if (FD_ISSET(serverSocket, &set))
        {
            clientSocket = accept(serverSocket, (struct sockaddr *) &address, &addrlen);
            if (clientSocket > 0)
            {
                Driver->clientSocket = clientSocket;
                Driver->clientConnected = true;
            }
        }
    }
    close(serverSocket);
    return NULL;
}

} // end of namespace
