/*
 * GraphLCD driver library
 *
 * drivers.c  -  global driver constants and functions
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2004 Andreas Regel <andreas.regel AT powarman.de>
 */

#include <string.h>

#include "drivers.h"
#include "simlcd.h"
#include "gu140x32f.h"
#include "gu256x64-372.h"
#include "gu256x64-3900.h"
#include "hd61830.h"
#include "ks0108.h"
#include "sed1330.h"
#include "sed1520.h"
#include "t6963c.h"
#include "framebuffer.h"
#include "image.h"
#include "noritake800.h"
#include "avrctl.h"
#include "network.h"
#include "gu126x64D-K610A4.h"
#include "dm140gink.h"
#include "serdisp.h"
#include "g15daemon.h"
#include "usbserlcd.h"
#include "st7565r-reel.h"
#ifdef HAVE_LIBHID
#include "futabaMDM166A.h"
#endif
#ifdef HAVE_DRIVER_AX206DPF
#include "ax206dpf.h"
#endif
#ifdef HAVE_DRIVER_picoLCD_256x64
#include "picoLCD_256x64.h"
#endif
#ifdef HAVE_DRIVER_VNCSERVER
#include "vncserver.h"
#endif
#ifdef HAVE_DRIVER_SSD1306
#include "ssd1306.h"
#endif
#ifdef HAVE_DRIVER_ILI9341
#include "ili9341.h"
#endif

namespace GLCD
{

tDriver drivers[] =
{
    {"simlcd",        kDriverSimLCD},
    {"gu140x32f",     kDriverGU140X32F},
    {"gu256x64-372",  kDriverGU256X64_372},
    {"gu256x64-3900", kDriverGU256X64_3900},
    {"hd61830",       kDriverHD61830},
    {"ks0108",        kDriverKS0108},
    {"sed1330",       kDriverSED1330},
    {"sed1520",       kDriverSED1520},
    {"t6963c",        kDriverT6963C},
    {"framebuffer",   kDriverFramebuffer},
    {"image",         kDriverImage},
    {"noritake800",   kDriverNoritake800},
    {"avrctl",        kDriverAvrCtl},
    {"network",       kDriverNetwork},
    {"gu126x64D-K610A4", kDriverGU126X64D_K610A4},
    {"dm140gink",     kDriverDM140GINK},
    {"usbserlcd",     kDriverUSBserLCD},
    {"st7565r-reel",  kDriverST7565RReel},
#ifdef HAVE_LIBHID
    {"futabaMDM166A", kDriverFutabaMDM166A},
#endif
    {"serdisp",       kDriverSerDisp},
    {"g15daemon",     kDriverG15daemon},
#ifdef HAVE_DRIVER_AX206DPF
    {"ax206dpf",      kDriverAX206DPF},
#endif
#ifdef HAVE_DRIVER_picoLCD_256x64
    {"picolcd256x64", kDriverPicoLCD_256x64},
#endif
#ifdef HAVE_DRIVER_VNCSERVER
    {"vncserver",     kDriverVncServer},
#endif
#ifdef HAVE_DRIVER_SSD1306
    {"ssd1306",       kDriverSSD1306},
#endif
#ifdef HAVE_DRIVER_ILI9341
    {"ili9341",       kDriverILI9341},
#endif
    {"",              kDriverUnknown}
};

tDriver * GetAvailableDrivers(int & count)
{
    for (count = 0; drivers[count].name.length() > 0; count++)
        ;
    return drivers;
}

int GetDriverID(const std::string & driver)
{
    for (int i = 0; drivers[i].name.length() > 0; i++)
        if (drivers[i].name == driver)
            return drivers[i].id;
    return kDriverUnknown;
}

cDriver * CreateDriver(int driverID, cDriverConfig * config)
{
    switch (driverID)
    {
        case kDriverSimLCD:
            return new cDriverSimLCD(config);
        case kDriverGU140X32F:
            return new cDriverGU140X32F(config);
        case kDriverGU256X64_372:
            return new cDriverGU256X64_372(config);
        case kDriverGU256X64_3900:
            return new cDriverGU256X64_3900(config);
        case kDriverHD61830:
            return new cDriverHD61830(config);
        case kDriverKS0108:
            return new cDriverKS0108(config);
        case kDriverSED1330:
            return new cDriverSED1330(config);
        case kDriverSED1520:
            return new cDriverSED1520(config);
        case kDriverT6963C:
            return new cDriverT6963C(config);
        case kDriverFramebuffer:
            return new cDriverFramebuffer(config);
        case kDriverImage:
            return new cDriverImage(config);
        case kDriverNoritake800:
            return new cDriverNoritake800(config);
        case kDriverAvrCtl:
            return new cDriverAvrCtl(config);
        case kDriverNetwork:
            return new cDriverNetwork(config);
        case kDriverGU126X64D_K610A4:
            return new cDriverGU126X64D_K610A4(config);
        case kDriverDM140GINK:
            return new cDriverDM140GINK(config);
        case kDriverUSBserLCD:
            return new cDriverUSBserLCD(config);
        case kDriverST7565RReel:
            return new cDriverST7565RReel(config);
#ifdef HAVE_LIBHID
        case kDriverFutabaMDM166A:
            return new cDriverFutabaMDM166A(config);
#endif
        case kDriverSerDisp:
            return new cDriverSerDisp(config);
        case kDriverG15daemon:
            return new cDriverG15daemon(config);
#ifdef HAVE_DRIVER_AX206DPF
        case kDriverAX206DPF:
            return new cDriverAX206DPF(config);
#endif
#ifdef HAVE_DRIVER_picoLCD_256x64
        case kDriverPicoLCD_256x64:
            return new cDriverPicoLCD_256x64(config);
#endif
#ifdef HAVE_DRIVER_VNCSERVER
        case kDriverVncServer:
            return new cDriverVncServer(config);
#endif
#ifdef HAVE_DRIVER_SSD1306
        case kDriverSSD1306:
            return new cDriverSSD1306(config);
#endif
#ifdef HAVE_DRIVER_ILI9341
        case kDriverILI9341:
            return new cDriverILI9341(config);
#endif
        case kDriverUnknown:
        default:
            return NULL;
    }
}

} // end of namespace
