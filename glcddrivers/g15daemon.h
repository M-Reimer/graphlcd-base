/*
 * GraphLCD driver library
 *
 * g15daemon.h  -  pseudo device for the g15daemon
 *                   Output goes to the g15daemon which then displays it
 *
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
    cDriverConfig * config;
    cDriverConfig * oldConfig;
    char *offbuff;
    int sockfd;
    long int screensize;
    char *fbp;
    int zoom;

    int CheckSetup();
    void SetPixel(int x, int y);

public:
    cDriverG15daemon(cDriverConfig * config);
    virtual ~cDriverG15daemon();

    virtual int Init();
    virtual int DeInit();

    virtual void Clear();
    virtual void Set8Pixels(int x, int y, unsigned char data);
    virtual void Refresh(bool refreshAll = false);
};

} // end of namespace

#endif
