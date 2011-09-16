/*
 * GraphLCD driver library
 *
 * g15daemon.h  -  pseudo device for the g15daemon
 *                   Output goes to the g15daemon which then displays it
 *
 * (c) 2005-2010 Andreas Regel <andreas.regel AT powarman.de>
 * (c) 2011      Wolfgang Astleitner <mrwastl AT users.sourceforge.net>
 */

#ifndef _GLCDDRIVERS_G15DAEMON_H_
#define _GLCDDRIVERS_G15DAEMON_H_

#include "driver.h"


namespace GLCD
{

class cDriverConfig;

class cDriverG15daemon : public cDriver
{
private:
    unsigned char ** LCD;
    char *offbuff;
    int sockfd;
    long int screensize;
    char *fbp;
    int zoom;

    int CheckSetup();

public:
    cDriverG15daemon(cDriverConfig * config);

    virtual int Init();
    virtual int DeInit();

    virtual void Clear();
    virtual void SetPixel(int x, int y, uint32_t data);
    //virtual void Set8Pixels(int x, int y, unsigned char data);
    virtual void Refresh(bool refreshAll = false);
};

} // end of namespace

#endif
