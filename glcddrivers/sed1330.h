/*
 * GraphLCD driver library
 *
 * sed1330.h  -  SED1330 driver class
 *
 * based on: hd61830.c
 *  (c) 2001-2004 Carsten Siebholz <c.siebholz AT t-online.de>
 *
 * changes for Seiko-Epson displays: Mar 2004
 *  (c) 2004 Heinz Gressenberger <heinz.gressenberger AT stmk.gv.at>
 *
 * init sequence taken from Thomas Baumann's LCD-Test program
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2003      Roland Praml <praml.roland AT t-online.de>
 * (c) 2005-2010 Andreas Regel <andreas.regel AT powarman.de>
 * (c) 2011      Wolfgang Astleitner <mrwastl AT users.sourceforge.net>
 */

#ifndef _GLCDDRIVERS_SED1330_H_
#define _GLCDDRIVERS_SED1330_H_

#include "driver.h"


namespace GLCD
{

class cDriverConfig;
class cParallelPort;

class cDriverSED1330 : public cDriver
{
private:
    cParallelPort * port;
    unsigned char ** newLCD; // wanted state
    unsigned char ** oldLCD; // current state
    int refreshCounter;
    long timeForPortCmdInNs;
    bool useSleepInit;

    int oscillatorFrequency;
    int interface;
    unsigned char A0HI;
    unsigned char A0LO;
    unsigned char RDHI;
    unsigned char RDLO;
    unsigned char ENHI;
    unsigned char ENLO;
    unsigned char WRHI;
    unsigned char WRLO;
    unsigned char RWHI;
    unsigned char RWLO;
    unsigned char CSHI;
    unsigned char CSLO;

    int CheckSetup();
    int InitGraphic();
    void WriteCmd(unsigned char cmd);
    void WriteData(unsigned char data);

public:
    cDriverSED1330(cDriverConfig * config);
    virtual ~cDriverSED1330();

    virtual int Init();
    virtual int DeInit();

    virtual void Clear();
    virtual void SetPixel(int x, int y, uint32_t data);
    //virtual void Set8Pixels(int x, int y, unsigned char data);
    virtual void Refresh(bool refreshAll = false);
};

} // end of namespace

#endif
