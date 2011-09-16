/*
 * GraphLCD driver library
 *
 * ks0108.h  -  KS0108 driver class
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2003-2011 Andreas 'randy' Weinberger <vdr AT smue.org>
 * (c) 2011      Wolfgang Astleitner <mrwastl AT users.sourceforge.net>
 */

#ifndef _GLCDDRIVERS_KS0108_H_
#define _GLCDDRIVERS_KS0108_H_

#include "driver.h"


namespace GLCD
{

class cDriverConfig;
class cParallelPort;

class cDriverKS0108 : public cDriver
{
private:
    cParallelPort * port;
    unsigned char ** LCD;      // linear lcd display "memory"
    unsigned char ** LCD_page; // paged lcd display "memory"
    int refreshCounter;
    long timeForPortCmdInNs;
    long timeForLCDInNs;
    bool useSleepInit;

    int CheckSetup();
    int InitGraphic();
    void KS0108Cmd(unsigned char data, int cs);
    void KS0108Data(unsigned char data, int cs);

    int SEAD;
    int SEPA;
    int SEDS;
    int DIOF;
    int DION;

    int CEHI;
    int CELO;
    int CDHI;
    int CDLO;
    int CS1HI;
    int CS1LO;
    int CS2HI;
    int CS2LO;

    int CS1;
    int CS2;
    int CS3;
    int CS4;

    unsigned char control;

public:
    cDriverKS0108(cDriverConfig * config);
    virtual ~cDriverKS0108();

    virtual int Init();
    virtual int DeInit();

    virtual void Clear();
    virtual void SetPixel(int x, int y, uint32_t data);
    //virtual void Set8Pixels(int x, int y, unsigned char data);
    virtual void Refresh(bool refreshAll = false);
};

} // end of namespace

#endif
