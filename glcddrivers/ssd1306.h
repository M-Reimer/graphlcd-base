/*
 * GraphLCD driver library
 *
 * ssd1306.h  -  SSD1306 OLED driver class
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2014 Andreas Regel <andreas.regel AT powarman.de>
 */

#ifndef _GLCDDRIVERS_SSD1306_H_
#define _GLCDDRIVERS_SSD1306_H_

#include "driver.h"

namespace GLCD
{

class cDriverConfig;

class cDriverSSD1306 : public cDriver
{
private:
    unsigned char ** newLCD; // wanted state
    unsigned char ** oldLCD; // current state
    int refreshCounter;

    int CheckSetup();

    void Reset();
    void WriteCommand(uint8_t command);
    void WriteCommand(uint8_t command, uint8_t argument);
    void WriteCommand(uint8_t command, uint8_t argument1, uint8_t argument2);
    void WriteData(uint8_t * buffer, uint32_t length);

public:
    cDriverSSD1306(cDriverConfig * config);
    virtual ~cDriverSSD1306();

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
