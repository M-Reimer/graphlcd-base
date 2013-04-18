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

#ifndef _GLCDDRIVERS_VNCSERVER_H_
#define _GLCDDRIVERS_VNCSERVER_H_

#include "driver.h"
#include <rfb/rfb.h>

namespace GLCD
{

class cDriverConfig;

class cDriverVncServer : public cDriver
{
private:
    unsigned char ** LCD;
    char *offbuff;
    rfbScreenInfoPtr server;
    long int screensize;
    int bbox[4];
    int depth;

    int CheckSetup();
    void processDamage (void);
protected:
    virtual bool GetDriverFeature  (const std::string & Feature, int & value);  
public:
    cDriverVncServer(cDriverConfig * config);

    virtual int Init();
    virtual int DeInit();

    virtual void Clear();
    virtual void SetPixel(int x, int y, uint32_t data);
    virtual void Refresh(bool refreshAll = false);
};

} // end of namespace

#endif
