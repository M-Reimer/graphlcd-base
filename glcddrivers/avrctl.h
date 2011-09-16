/*
 * GraphLCD driver library
 *
 * avrctl.h  -  AVR controlled LCD driver class
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2005-2010 Andreas Regel <andreas.regel AT powarman.de>
 * (c) 2011      Wolfgang Astleitner <mrwastl AT users.sourceforge.net>
 */

#ifndef _GLCDDRIVERS_AVRCTL_H_
#define _GLCDDRIVERS_AVRCTL_H_

#include "driver.h"

namespace GLCD
{

class cDriverConfig;
class cSerialPort;

class cDriverAvrCtl : public cDriver
{
private:
    cSerialPort * port;
    unsigned char ** newLCD; // wanted state
    unsigned char ** oldLCD; // current state
    int refreshCounter;

    int WaitForAck(void);
    void CmdSysSync(void);
    void CmdDispClearScreen(void);
    void CmdDispSwitchScreen(void);
    void CmdDispSetBrightness(unsigned char percent);
    void CmdDispSetColData(uint16_t column, uint16_t offset, uint16_t length, uint8_t * data);
    void CmdDispUpdate(void);

    int CheckSetup();

public:
    cDriverAvrCtl(cDriverConfig * config);
    virtual ~cDriverAvrCtl();

    virtual int Init();
    virtual int DeInit();

    virtual void Clear();
    virtual void SetPixel(int x, int y, uint32_t data);
    //virtual void Set8Pixels(int x, int y, unsigned char data);
    virtual void Refresh(bool refreshAll = false);
    virtual void SetBrightness(unsigned int percent);
};

} // end of namespace

#endif
