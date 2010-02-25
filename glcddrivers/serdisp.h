/*
 * GraphLCD driver library
 *
 * serdisp.h  -  include support for displays supported by serdisplib (if library is installed)
 *               http://serdisplib.sourceforge.net
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2003-2010 Wolfgang Astleitner <mrwastl AT users.sourceforge.net>
 */

#ifndef _GLCDDRIVERS_SERDISP_H_
#define _GLCDDRIVERS_SERDISP_H_

#include "driver.h"


namespace GLCD
{

class cDriverConfig;

class cDriverSerDisp : public cDriver
{
private:

    cDriverConfig * config;
    cDriverConfig * oldConfig;

    long  serdisp_version;

    int   supports_options;
    long  fg_colour;
    long  bg_colour;

    void* sdhnd; // serdisplib handle
    void* dd;    // display descriptor
    void* sdcd;  // serdisp connect descriptor

    long  (*fp_serdisp_getversioncode) ();

    void* (*fp_SDCONN_open)            (const char sdcdev[]);

    void* (*fp_PP_open)                (const char sdcdev[]);
    void* (*fp_PP_close)               (void* sdcd);

    void* (*fp_serdisp_init)           (void* sdcd, const char dispname[], const char extra[]);
    void  (*fp_serdisp_rewrite)        (void* dd);
    void  (*fp_serdisp_update)         (void* dd);
    void  (*fp_serdisp_clearbuffer)    (void* dd);
    void  (*fp_serdisp_setpixcol)      (void* dd, int x, int y, long colour);  // serdisp_setpixel or serdisp_setcolour
    int   (*fp_serdisp_feature)        (void* dd, int feature, int value);
    int   (*fp_serdisp_isoption)       (void* dd, const char* optionname);
    void  (*fp_serdisp_setoption)      (void* dd, const char* optionname, long value);
    long  (*fp_serdisp_getoption)      (void* dd, const char* optionname, int* typesize);
    int   (*fp_serdisp_getwidth)       (void* dd);
    int   (*fp_serdisp_getheight)      (void* dd);
    void  (*fp_serdisp_quit)           (void* dd);
    void  (*fp_serdisp_close)          (void* dd);

    int CheckSetup();

public:

    cDriverSerDisp(cDriverConfig * config);
    virtual ~cDriverSerDisp();

    virtual int Init();
    virtual int DeInit();

    virtual void Clear();
    virtual void Set8Pixels(int x, int y, unsigned char data);
    virtual void Refresh(bool refreshAll = false);
    virtual void SetBrightness(unsigned int percent);
};

#endif

} // end of namespace
