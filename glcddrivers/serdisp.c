/*
 * GraphLCD driver library
 *
 * serdisp.h  -  include support for displays supported by serdisplib (if library is installed)
 *               http://serdisplib.sourceforge.net
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2003-2011 Wolfgang Astleitner <mrwastl AT users.sourceforge.net>
 */

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <dlfcn.h>

#include "common.h"
#include "config.h"
#include "serdisp.h"

// for memcpy
#include <string.h>

#define SERDISP_VERSION(a,b) ((long)(((a) << 8) + (b)))
#define SERDISP_VERSION_GET_MAJOR(_c)  ((int)( (_c) >> 8 ))
#define SERDISP_VERSION_GET_MINOR(_c)  ((int)( (_c) & 0xFF ))

// taken from serdisp_control.h
#define FEATURE_CONTRAST  0x01
#define FEATURE_REVERSE   0x02
#define FEATURE_BACKLIGHT 0x03
#define FEATURE_ROTATE    0x04

#define SD_COL_BLACK      0xFF000000
#define SD_COL_WHITE      0xFFFFFFFF

// taken from serdisp_gpevents.h
#define SDGPT_SIMPLETOUCH  0x10  /* simple touch screen event, type: SDGP_evpkt_simpletouch_t */

namespace GLCD
{

static void wrapEventListener(void* dd, SDGP_event_t* recylce);

static int simpleTouchX=0, simpleTouchY=0, simpleTouchT=0;
static bool simpleTouchChanged=false;


cDriverSerDisp::cDriverSerDisp(cDriverConfig * config)
:   config(config)
{
    oldConfig = new cDriverConfig(*config);

    dd = (void *) NULL;
    simpleTouchChanged = false;
}

cDriverSerDisp::~cDriverSerDisp(void)
{
    delete oldConfig;
}

int cDriverSerDisp::Init(void)
{
    char* errmsg; // error message returned by dlerror()
    int bg_forced = 0;  /* bg-colour set through graphlcd option 'BGColour' */
    int fg_forced = 0;  /* fg-colour set through graphlcd option 'FGColour' */

    std::string controller;
    std::string optionstring = "";
    std::string wiringstring;


    // dynamically load serdisplib using dlopen() & co.

    sdhnd = dlopen("libserdisp.so", RTLD_LAZY);
    if (!sdhnd) { // try /usr/local/lib
        sdhnd = dlopen("/usr/local/lib/libserdisp.so", RTLD_LAZY);
    }

    if (!sdhnd) { // serdisplib seems not to be installed
        syslog(LOG_ERR, "%s: error: unable to dynamically load library '%s'. Err: %s (cDriver::Init)\n",
        config->name.c_str(), "libserdisp.so", "not found");
        return -1;
    }

    dlerror(); // clear error code

    /* pre-init some flags, function pointers, ... */
    supports_options = 0;
    fg_colour = 1;
    bg_colour = -1;

    // get serdisp version
    fp_serdisp_getversioncode = (long int (*)()) dlsym(sdhnd, "serdisp_getversioncode");

    if (dlerror()) { // no serdisp_getversioncode() -> version of serdisplib is < 1.95
        syslog(LOG_ERR, "%s: error: serdisplib version >= 1.95 required\n", config->name.c_str());
        return -1;
    }

    serdisp_version = fp_serdisp_getversioncode();
    syslog(LOG_DEBUG, "%s: INFO: detected serdisplib version %d.%d (cDriver::Init)\n",
        config->name.c_str(), SERDISP_VERSION_GET_MAJOR(serdisp_version), SERDISP_VERSION_GET_MINOR(serdisp_version));


    fp_SDCONN_open = (void*(*)(const char*)) dlsym(sdhnd, "SDCONN_open");
    if ( (errmsg = dlerror()) != NULL  ) { // should not happen
        syslog(LOG_ERR, "%s: error: cannot load symbol %s. Err:%s (cDriver::Init)\n",
            config->name.c_str(), "SDCONN_open", errmsg);
        return -1;
    }
    fp_serdisp_quit = (void (*)(void*)) dlsym(sdhnd, "serdisp_quit");
    if ( (errmsg = dlerror()) != NULL  ) { // should not happen
        syslog(LOG_ERR, "%s: error: cannot load symbol %s. Err:%s (cDriver::Init)\n",
            config->name.c_str(), "serdisp_quit", errmsg);
        return -1;
    }
    fp_serdisp_setcolour = (void (*)(void*, int, int, long int)) dlsym(sdhnd, "serdisp_setcolour");
    if ( (errmsg = dlerror()) != NULL  ) { // should not happen
        syslog(LOG_ERR, "%s: error: cannot load symbol %s. Err:%s (cDriver::Init)\n",
            config->name.c_str(), "serdisp_setcolour", errmsg);
        return -1;
    }
    fg_colour = SD_COL_BLACK; /* set foreground colour to black */

    if (serdisp_version >= SERDISP_VERSION(1,96) ) {
        supports_options = 1;

        fp_serdisp_isoption = (int (*)(void*, const char*)) dlsym(sdhnd, "serdisp_isoption");
        if ( (errmsg = dlerror()) != NULL  ) { // should not happen
            syslog(LOG_ERR, "%s: error: cannot load symbol %s. Err:%s (cDriver::Init)\n",
                config->name.c_str(), "serdisp_isoption", errmsg);
            return -1;
        }
        fp_serdisp_setoption = (void (*)(void*, const char*, long int)) dlsym(sdhnd, "serdisp_setoption");
        if ( (errmsg = dlerror()) != NULL  ) { // should not happen
            syslog(LOG_ERR, "%s: error: cannot load symbol %s. Err:%s (cDriver::Init)\n",
                config->name.c_str(), "serdisp_setoption", errmsg);
            return -1;
        }
        fp_serdisp_getoption = (long int (*)(void*, const char*, int*)) dlsym(sdhnd, "serdisp_getoption");
        if ( (errmsg = dlerror()) != NULL  ) { // should not happen
            syslog(LOG_ERR, "%s: error: cannot load symbol %s. Err:%s (cDriver::Init)\n",
                config->name.c_str(), "serdisp_getoption", errmsg);
            return -1;
        }
    } /* >= 1.96 */

    // load other symbols that will be required
    fp_serdisp_init = (void*(*)(void*, const char*, const char*)) dlsym(sdhnd, "serdisp_init");
    if ( (errmsg = dlerror()) != NULL  ) { // should not happen
        syslog(LOG_ERR, "%s: error: cannot load symbol %s. Err:%s (cDriver::Init)\n",
            config->name.c_str(), "serdisp_init", errmsg);
        return -1;
    }

    fp_serdisp_rewrite = (void (*)(void*)) dlsym(sdhnd, "serdisp_rewrite");
    if ( (errmsg = dlerror()) != NULL  ) { // should not happen
        syslog(LOG_ERR, "%s: error: cannot load symbol %s. Err:%s (cDriver::Init)\n",
            config->name.c_str(), "serdisp_rewrite", errmsg);
        return -1;
    }

    fp_serdisp_update = (void (*)(void*)) dlsym(sdhnd, "serdisp_update");
    if ( (errmsg = dlerror()) != NULL  ) { // should not happen
        syslog(LOG_ERR, "%s: error: cannot load symbol %s. Err:%s (cDriver::Init)\n",
            config->name.c_str(), "serdisp_update", errmsg);
        return -1;
    }

    fp_serdisp_clearbuffer = (void (*)(void*)) dlsym(sdhnd, "serdisp_clearbuffer");
    if ( (errmsg = dlerror()) != NULL  ) { // should not happen
        syslog(LOG_ERR, "%s: error: cannot load symbol %s. Err:%s (cDriver::Init)\n",
            config->name.c_str(), "serdisp_clearbuffer", errmsg);
        return -1;
    }

    fp_serdisp_feature = (int (*)(void*, int, int)) dlsym(sdhnd, "serdisp_feature");
    if ( (errmsg = dlerror()) != NULL  ) { // should not happen
        syslog(LOG_ERR, "%s: error: cannot load symbol %s. Err:%s (cDriver::Init)\n",
            config->name.c_str(), "serdisp_feature", errmsg);
        return -1;
    }

    fp_serdisp_close = (void (*)(void*))dlsym(sdhnd, "serdisp_close");
    if ( (errmsg = dlerror()) != NULL  ) { // should not happen
        syslog(LOG_ERR, "%s: error: cannot load symbol %s. Err:%s (cDriver::Init)\n",
            config->name.c_str(), "serdisp_close", errmsg);
        return -1;
    }

    fp_serdisp_getwidth = (int (*)(void*)) dlsym(sdhnd, "serdisp_getwidth");
    if ( (errmsg = dlerror()) != NULL  ) { // should not happen
        syslog(LOG_ERR, "%s: error: cannot load symbol %s. Err:%s (cDriver::Init)\n",
            config->name.c_str(), "serdisp_getwidth", errmsg);
        return -1;
    }

    fp_serdisp_getheight = (int (*)(void*)) dlsym(sdhnd, "serdisp_getheight");
    if ( (errmsg = dlerror()) != NULL  ) { // should not happen
        syslog(LOG_ERR, "%s: error: cannot load symbol %s. Err:%s (cDriver::Init)\n",
            config->name.c_str(), "serdisp_getheight", errmsg);
        return -1;
    }

    fp_serdisp_getcolours = (int (*)(void*)) dlsym(sdhnd, "serdisp_getcolours");
    if ( (errmsg = dlerror()) != NULL  ) { // should not happen
        syslog(LOG_ERR, "%s: error: cannot load symbol %s. Err:%s (cDriver::Init)\n",
            config->name.c_str(), "serdisp_getcolours", errmsg);
        return -1;
    }

    // don't care if the following functions are not available
    fp_serdisp_getdepth = (int (*)(void*)) dlsym(sdhnd, "serdisp_getdepth");

    fp_SDGPI_search    = (uint8_t (*)(void*, const char*))  dlsym(sdhnd, "SDGPI_search");
    fp_SDGPI_isenabled = (int     (*)(void*, uint8_t))      dlsym(sdhnd, "SDGPI_isenabled");
    fp_SDGPI_enable    = (int     (*)(void*, uint8_t, int)) dlsym(sdhnd, "SDGPI_enable");
    fp_SDEVLP_add_listener = (int (*)(void*, uint8_t, fp_eventlistener_t)) dlsym(sdhnd, "SDEVLP_add_listener");


    // done loading all required symbols

    // setting up the display
    width = 0;
    height = 0;

    for (unsigned int i = 0; i < config->options.size(); i++)
    {
        if (config->options[i].name == "Controller") {
            controller = config->options[i].value;
        } else if (config->options[i].name == "Options") {
            optionstring = config->options[i].value;
        } else if (config->options[i].name == "Wiring") {
            wiringstring = config->options[i].value;
        } else if (config->options[i].name == "FGColour") {
            fg_colour = strtoul(config->options[i].value.c_str(), (char **)NULL, 0);
            fg_colour |= 0xFF000000L;  /* force alpha to 0xFF */
            fg_forced = 1;
        } else if (config->options[i].name == "BGColour") {
            bg_colour = strtoul(config->options[i].value.c_str(), (char **)NULL, 0);
            bg_colour |= 0xFF000000L;  /* force alpha to 0xFF */
            bg_forced = 1;
        }
    }

    if (wiringstring.length()) {
        optionstring = "WIRING=" + wiringstring + ((optionstring != "") ? ";" + optionstring : "");
    }

    if (controller == "")
    {
        syslog(LOG_ERR, "%s error: no controller given!\n", config->name.c_str());
        return -1;
    }


    if (config->device == "")
    {
        // use DirectIO

        // neither device nor port is set
        if (config->port == 0)
            return -1;

        char temp[10];
        snprintf(temp, 8, "0x%x", config->port);

        sdcd = fp_SDCONN_open(temp);

        if (sdcd == 0) {
            syslog(LOG_ERR, "%s: error: unable to open port 0x%x for display %s. (cDriver::Init)\n",
                config->name.c_str(), config->port, controller.c_str());
            return -1;
        }

        uSleep(10);
    }
    else
    {
        sdcd = fp_SDCONN_open(config->device.c_str());

        if (sdcd == 0) {
            syslog(LOG_ERR, "%s: error: unable to open device %s for display %s. (cDriver::Init)\n",
                config->name.c_str(), config->device.c_str(), controller.c_str());
            return -1;
        }
    }

    dd = fp_serdisp_init(sdcd, controller.c_str(), optionstring.c_str());

    if (!dd)
    {
        syslog(LOG_ERR, "%s: error: cannot open display %s. Err:%s (cDriver::Init)\n",
            config->name.c_str(), controller.c_str(), "no handle");
        return -1;
    }

    // self-emitting displays (like OLEDs): default background colour => black
    if ( supports_options && fp_serdisp_isoption(dd, "SELFEMITTING") && (fp_serdisp_getoption(dd, "SELFEMITTING", 0)) ) {
       if (!bg_forced)
         bg_colour = SD_COL_BLACK; /* set background colour to black */
       if (!fg_forced)
         fg_colour = SD_COL_WHITE; /* set foreground colour to white */
    }

    width = config->width;
    if (width <= 0)
        width = fp_serdisp_getwidth(dd);
    height = config->height;
    if (height <= 0)
        height = fp_serdisp_getheight(dd);

    if (serdisp_version < SERDISP_VERSION(1,96) ) {
        fp_serdisp_feature(dd, FEATURE_ROTATE, config->upsideDown);
        fp_serdisp_feature(dd, FEATURE_CONTRAST, config->contrast);
        fp_serdisp_feature(dd, FEATURE_BACKLIGHT, config->backlight);
        fp_serdisp_feature(dd, FEATURE_REVERSE, config->invert);
    } else {
        /* standard options */
        if (config->upsideDown)
            fp_serdisp_setoption(dd, "ROTATE", config->upsideDown);
        fp_serdisp_setoption(dd, "CONTRAST", config->contrast);
        fp_serdisp_setoption(dd, "BACKLIGHT", config->backlight);
        fp_serdisp_setoption(dd, "INVERT", config->invert);

        /* driver dependend options */
        for (unsigned int i = 0; i < config->options.size(); i++) {
            std::string optionname = config->options[i].name;
            if (optionname != "UpsideDown" && optionname != "Contrast" &&
                optionname != "Backlight" && optionname != "Invert") {

                if ( fp_serdisp_isoption(dd, optionname.c_str()) == 1 )  /* if == 1: option is existing AND r/w */
                    fp_serdisp_setoption(dd, optionname.c_str(), strtol(config->options[i].value.c_str(), NULL, 0));
            }
        }

    }

    *oldConfig = *config;

    // set initial brightness
    SetBrightness(config->brightness);
    // clear display
    Clear();

    syslog(LOG_INFO, "%s: SerDisp with %s initialized.\n", config->name.c_str(), controller.c_str());
    return 0;
}

int cDriverSerDisp::DeInit(void)
{
    //fp_serdisp_quit(dd);
    /* use serdisp_close instead of serdisp_quit so that showpic and showtext are usable together with serdisplib */
    fp_serdisp_close(dd);

    (int) dlclose(sdhnd);
    sdhnd = NULL;

    return 0;
}

int cDriverSerDisp::CheckSetup()
{
    bool update = false;

    if (config->device != oldConfig->device ||
        config->port != oldConfig->port ||
        config->width != oldConfig->width ||
        config->height != oldConfig->height)
    {
        DeInit();
        Init();
        return 0;
    }

    if (config->contrast != oldConfig->contrast)
    {
        fp_serdisp_feature(dd, FEATURE_CONTRAST, config->contrast);
        oldConfig->contrast = config->contrast;
        update = true;
    }
    if (config->backlight != oldConfig->backlight)
    {
        fp_serdisp_feature(dd, FEATURE_BACKLIGHT, config->backlight);
        oldConfig->backlight = config->backlight;
        update = true;
    }
    if (config->upsideDown != oldConfig->upsideDown)
    {
        fp_serdisp_feature(dd, FEATURE_ROTATE, config->upsideDown);
        oldConfig->upsideDown = config->upsideDown;
        update = true;
    }
    if (config->invert != oldConfig->invert)
    {
        fp_serdisp_feature(dd, FEATURE_REVERSE, config->invert);
        oldConfig->invert = config->invert;
        update = true;
    }

    if (config->brightness != oldConfig->brightness)
    {   
        oldConfig->brightness = config->brightness;
        SetBrightness(config->brightness);
        update = true;
    }

#if 0
    /* driver dependend options */
    if ( supports_options ) {
        for (unsigned int i = 0; i < config->options.size(); i++) {
            std::string optionname = config->options[i].name;
            if (optionname != "UpsideDown" && optionname != "Contrast" &&
                optionname != "Backlight" && optionname != "Invert") {

                if ( fp_serdisp_isoption(dd, optionname.c_str()) == 1 )  /* if == 1: option is existing AND r/w */
                    fp_serdisp_setoption(dd, optionname.c_str(), strtol(config->options[i].value.c_str(), NULL, 0));
                oldConfig->options[i] = config->options[i];
                update = true;
            }
        }
    }
#endif

    if (update)
        return 1;
    return 0;
}

void cDriverSerDisp::Clear(void)
{
    if (bg_colour == -1)
        fp_serdisp_clearbuffer(dd);
    else {  /* if bg_colour is set, draw background 'by hand' */
        int x,y;
        for (y = 0; y < fp_serdisp_getheight(dd); y++)
            for (x = 0; x < fp_serdisp_getwidth(dd); x++)
                fp_serdisp_setcolour(dd, x, y, bg_colour);
    }
}

void cDriverSerDisp::Set8Pixels(int x, int y, unsigned char data) {
    int i, start, pixel;

    data = ReverseBits(data);

    start = (x >> 3) << 3;

    for (i = 0; i < 8; i++) {
        pixel = data & (1 << i);
        if (pixel) {
          SetPixel(start + i, y, fg_colour);
        } else if (!pixel && bg_colour != -1) { /* if bg_colour is set: use it if pixel is not set */
          SetPixel(start + i, y, bg_colour);
        }
    }
}

void cDriverSerDisp::SetPixel(int x, int y, uint32_t data)
{
    fp_serdisp_setcolour(dd, x, y, data);
}

#if 0
// temporarily overwrite SetScreen() until problem with 'to Clear() or not to Clear()' is solved
void cDriverSerDisp::SetScreen(const unsigned char * data, int wid, int hgt, int lineSize)
{
    int x, y;

    if (wid > width)
        wid = width;
    if (hgt > height)
        hgt = height;

    //Clear();
    if (data)
    {
        for (y = 0; y < hgt; y++)
        {
            for (x = 0; x < (wid / 8); x++)
            {
                Set8Pixels(x * 8, y, data[y * lineSize + x]);
            }
            if (width % 8)
            {
                Set8Pixels((wid / 8) * 8, y, data[y * lineSize + wid / 8] & bitmaskl[wid % 8 - 1]);
            }
        }
    } else {
      Clear();
    }
}
#endif

void cDriverSerDisp::Refresh(bool refreshAll)
{
    if (CheckSetup() == 1)
        refreshAll = true;

    if (refreshAll)
        fp_serdisp_rewrite(dd);
    else
        fp_serdisp_update(dd);
}

void cDriverSerDisp::SetBrightness(unsigned int percent)
{
    if ( supports_options && (fp_serdisp_isoption(dd, "BRIGHTNESS") == 1) )  /* if == 1: option is existing AND r/w */
        fp_serdisp_setoption(dd, "BRIGHTNESS", (long)percent);
}

GLCD::cColor cDriverSerDisp::GetBackgroundColor(void) {
    if ( supports_options && fp_serdisp_isoption(dd, "SELFEMITTING") && (fp_serdisp_getoption(dd, "SELFEMITTING", 0)) ) {
       return GLCD::cColor::Black;
    }
    return GLCD::cColor::White;
}


bool cDriverSerDisp::SetFeature (const std::string & Feature, int value)
{
    if (strcasecmp(Feature.c_str(), "TOUCHSCREEN") == 0 || strcasecmp(Feature.c_str(), "TOUCH") == 0) {
        if (fp_SDGPI_search && fp_SDGPI_isenabled && fp_SDGPI_enable) {
            uint8_t gpid = fp_SDGPI_search(dd, Feature.c_str());
            if (gpid == 0xFF)
                return false;

            int ena = fp_SDGPI_isenabled(dd, gpid);
            bool enable = (value == 1) ? true : false;
            if (ena == enable) { // already enabled or disabled
                return true;
            } else {
                bool rc = (fp_SDGPI_enable(dd, gpid, ((enable) ? 1 : 0)) >= 0) ? true : false;

                if (enable && rc && fp_SDEVLP_add_listener) {
                    fp_SDEVLP_add_listener(dd, gpid, wrapEventListener);
                }
                return true;
            }
        }
    }
    return false;
}

bool cDriverSerDisp::GetDriverFeature  (const std::string & Feature, int & value) {
    if (dd) {
        if (strcasecmp(Feature.c_str(), "depth") == 0) {
            value = fp_serdisp_getdepth(dd);
            return true;
        } else if (strcasecmp(Feature.c_str(), "ismonochrome") == 0) {
            value = (fp_serdisp_getdepth(dd) == 1) ? 1 : 0;
            return true;
        } else if (strcasecmp(Feature.c_str(), "isgreyscale") == 0 || strcasecmp(Feature.c_str(), "isgrayscale") == 0) {
            value = (fp_serdisp_getdepth(dd) > 1 && fp_serdisp_getdepth(dd) < 8) ? 1 : 0;
            return true;
        } else if (strcasecmp(Feature.c_str(), "iscolour") == 0 || strcasecmp(Feature.c_str(), "iscolor") == 0) {
            value = (fp_serdisp_getdepth(dd) >= 8) ? 1 : 0;
            return true;
        } else if (strcasecmp(Feature.c_str(), "touch") == 0 || strcasecmp(Feature.c_str(), "touchscreen") == 0) {
            if (fp_SDGPI_search && fp_SDGPI_isenabled) {
                uint8_t gpid = fp_SDGPI_search(dd, Feature.c_str());
                value = (gpid != 0xFF && fp_SDGPI_isenabled(dd, gpid)) ? 1 : 0;
            }
            return true;
        }
    }
    value = 0;
    return false;
}

cGLCDEvent * cDriverSerDisp::GetEvent(void) {
    if (GLCD::simpleTouchChanged == false)
        return NULL;

    cSimpleTouchEvent * ev = new cSimpleTouchEvent();

    ev->x = simpleTouchX;
    ev->y = simpleTouchY;
    ev->touch = simpleTouchT;
    simpleTouchChanged = false;
    return ev;
}

static void wrapEventListener(void* dd, SDGP_event_t* event) {
    if (!event) return;
    if (event->type == SDGPT_SIMPLETOUCH) {
        SDGP_evpkt_simpletouch_t  simpletouch;
        memcpy(&simpletouch, &event->data, sizeof(SDGP_evpkt_simpletouch_t));
        simpleTouchChanged = true;
        simpleTouchX = simpletouch.norm_x;
        simpleTouchY = simpletouch.norm_y;
        simpleTouchT = simpletouch.norm_touch;
    }
}

} // end of namespace
