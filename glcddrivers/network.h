/*
 * GraphLCD driver library
 *
 * network.h  -  Network output device
 *               Output goes to a network client.
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2004      Andreas Regel <andreas.regel AT powarman.de>
 * (c) 2011      Wolfgang Astleitner <mrwastl AT users.sourceforge.net>
 */

#ifndef _GLCDDRIVERS_NETWORK_H_
#define _GLCDDRIVERS_NETWORK_H_

#include <pthread.h>

#include "driver.h"


namespace GLCD
{

class cDriverConfig;

class cDriverNetwork : public cDriver
{
private:
    unsigned char * newLCD;
    unsigned char * oldLCD;
    int lineSize;
    bool running;
    pthread_t childTid;
    int clientSocket;
    bool clientConnected;

    int CheckSetup();
    static void * ServerThread(cDriverNetwork * Driver);

public:
    cDriverNetwork(cDriverConfig * config);

    virtual int Init();
    virtual int DeInit();

    virtual void Clear();
    virtual void SetPixel(int x, int y, uint32_t data);
    //virtual void Set8Pixels(int x, int y, unsigned char data);
    virtual void Refresh(bool refreshAll = false);
};

} // end of namespace

#endif
