/*
 * GraphLCD driver library
 *
 * ili9341.h  -  ILI9341 OLED driver class
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2015 Andreas Regel <andreas.regel AT powarman.de>
 */

#ifndef _GLCDDRIVERS_ILI9341_H_
#define _GLCDDRIVERS_ILI9341_H_

#include "driver.h"

namespace GLCD
{

class cDriverConfig;

class cDriverILI9341 : public cDriver
{
private:
    uint32_t * newLCD; // wanted state
    uint32_t * oldLCD; // current state
    int refreshCounter;

    int CheckSetup();

    void Reset();
    void SetWindow(uint16_t x0, uint16_t x1, uint16_t y0, uint16_t y1);
    void WriteCommand(uint8_t command);
    void WriteData(uint8_t data);
    void WriteData(uint8_t * buffer, uint32_t length);

public:
    cDriverILI9341(cDriverConfig * config);
    virtual ~cDriverILI9341();

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
