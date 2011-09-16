/*
 * GraphLCD driver library
 *
 * sed1520.h  -  SED1520 driver class
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2003-2011 Andreas 'randy' Weinberger <vdr AT smue.org>
 * (c) 2011      Wolfgang Astleitner <mrwastl AT users.sourceforge.net>
 */

#ifndef _GLCDDRIVERS_SED1520_H_
#define _GLCDDRIVERS_SED1520_H_

#include "driver.h"


namespace GLCD
{

class cDriverConfig;
class cParallelPort;

class cDriverSED1520 : public cDriver
{
private:
    cParallelPort * port;
    unsigned char ** LCD;      // linear lcd display "memory"
    unsigned char ** LCD_page; // paged lcd display "memory"
    int refreshCounter;
    long timeForPortCmdInNs;
    bool useSleepInit;

    int SEAD;
    int SEPA;
    int SEDS;
    int DION;
    int DIOF;

    int CS1LO;
    int CS2LO;
    int CS1HI;
    int CS2HI;

    int CDHI;
    int CDLO;

    int LED;
    int LEDHI;

    int CheckSetup();
    int InitGraphic();
    void SED1520Cmd(unsigned char data, int cmscd);
    void SED1520Data(unsigned char data, int datacs);

public:
    cDriverSED1520(cDriverConfig * config);
    virtual ~cDriverSED1520();

    virtual int Init();
    virtual int DeInit();

    virtual void Clear();
    virtual void SetPixel(int x, int y, uint32_t data);
    //virtual void Set8Pixels(int x, int y, unsigned char data);
    virtual void Refresh(bool refreshAll = false);
};

} // end of namespace

#endif
