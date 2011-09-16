/*
 * GraphLCD driver library
 *
 * driver.h  -  driver base class
 *
 * parts were taken from graphlcd plugin for the Video Disc Recorder
 * (c) 2001-2004 Carsten Siebholz <c.siebholz AT t-online.de>
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2004-2010 Andreas Regel <andreas.regel AT powarman.de>
 * (c) 2010-2011 Wolfgang Astleitner <mrwastl AT users sourceforge net>
 */

#ifndef _GLCDDRIVERS_DRIVER_H_
#define _GLCDDRIVERS_DRIVER_H_

#include <stdint.h>
#include "../glcdgraphics/bitmap.h"

// for strcasecmp
#include <strings.h>

namespace GLCD
{

class cGLCDEvent {
public:
    virtual ~cGLCDEvent() {}
};

class cSimpleTouchEvent : public cGLCDEvent {
public:
    int x;
    int y;
    int touch;
    cSimpleTouchEvent();
};

class cDriverConfig;

class cDriver
{
protected:
    int      width;
    int      height;
    uint32_t bgcol;
    uint32_t fgcol;
    cDriverConfig * config;
    cDriverConfig * oldConfig;

    virtual bool GetDriverFeature  (const std::string & Feature, int & value) { return false; }
    virtual uint32_t GetDefaultBackgroundColor(void) { return GRAPHLCD_Black; }
            uint32_t GetDefaultForegroundColor(void) { return GetDefaultBackgroundColor() ^ 0x00FFFFFF; }
public:
    cDriver(cDriverConfig * config);
    virtual ~cDriver();

    int Width() const { return width; }
    int Height() const { return height; }
    
    const std::string ConfigName();
    const std::string DriverName();

    virtual int Init() { return 0; }
    virtual int DeInit() { return 0; }

    virtual void Clear() {}
    virtual void SetPixel(int x, int y, uint32_t data) {}
            void Set8Pixels(int x, int y, unsigned char data);
//    virtual void SetScreen(const unsigned char * data, int width, int height, int lineSize);
    virtual void SetScreen(const uint32_t *data, int width, int height);
    virtual void Refresh(bool refreshAll = false) {}

    virtual void SetBrightness(unsigned int percent) {}


    virtual bool SetFeature  (const std::string & Feature, int value)   { return false; }

    uint32_t GetBackgroundColor(bool driverdefault=false) {
        return (
            (driverdefault || bgcol == GRAPHLCD_ERRCOL || bgcol == GRAPHLCD_Transparent)
            ? GetDefaultBackgroundColor()
            : bgcol
        );
    }
    uint32_t GetForegroundColor(bool driverdefault=false) {
        return (
            (driverdefault || fgcol == GRAPHLCD_ERRCOL || fgcol == GRAPHLCD_Transparent)
            ? GetDefaultForegroundColor()
            : fgcol
        );
    }

    // not to be overridden, override GetDriverFeature() instead
    // the following feature names (case insensitive!) are guaranteed to give results:
    // 'depth'                       colour depth, default: 1
    // 'ismonochrome'                is lcd a monochrome display?, default: true (1)
    // the following feature names are pre-defined but default to false (0)
    // 'isgreyscale', 'isgrayscale'  is lcd a greyscale display?
    // 'iscolour', 'iscolor'         is lcd a colour display?
    // 'touch', 'touchscreen'        is a touchscreen supported and available?
    bool GetFeature  (const std::string & Feature, int & value) {
        if (GetDriverFeature(Feature, value)) {
            return true;
        } else if (strcasecmp(Feature.c_str(), "depth") == 0) {
            value = 1;
            return true;
        } else if (strcasecmp(Feature.c_str(), "ismonochrome") == 0) {
            value = 1; // true == 1, false == 0
            return true;
        } else if (strcasecmp(Feature.c_str(), "isgreyscale") == 0 || strcasecmp(Feature.c_str(), "isgrayscale") == 0) {
            value = 0; // true == 1, false == 0
            return true;
        } else if (strcasecmp(Feature.c_str(), "iscolour") == 0 || strcasecmp(Feature.c_str(), "iscolor") == 0) {
            value = 0; // true == 1, false == 0
            return true;
        } else if (strcasecmp(Feature.c_str(), "touch") == 0 || strcasecmp(Feature.c_str(), "touchscreen") == 0) {
            value = 0; // true == 1, false == 0
            return true;
        }
        value = 0;
        return false;
    }

    virtual cGLCDEvent * GetEvent(void) { return NULL; }

};

} // end of namespace

#endif
