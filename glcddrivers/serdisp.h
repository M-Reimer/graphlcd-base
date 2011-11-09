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

#ifndef _GLCDDRIVERS_SERDISP_H_
#define _GLCDDRIVERS_SERDISP_H_

#include "driver.h"
#include <sys/time.h>

namespace GLCD
{

/* event-type for GPIs, GPOs, and data exchange messages. min. size: 16 byte, max size: 12 + 64) */
typedef struct SDGP_event_s { /* 16 to 78 bytes */
  /* byte  0 */
  uint8_t        type;         /* one of SDGPT_* */
  uint8_t        cmdid;        /* command-ID (one of SD_CMD_*) */
  uint8_t        devid;        /* device ID, 0 == local */
  uint8_t        subid;        /* gp-ID or page-ID */
  /* byte  4 */
  struct timeval timestamp;    /* timestamp (8 bytes) */
  /* byte 12 */
  union {
    int32_t      value;        /* if single value event: value  */
    struct {                   /* if streaming event or package: */
      uint16_t   length;       /*   length of stream if known or 0 if some stop tag is used */
      uint8_t    word_size;    /*   stream elements are bytes/chars (0 or 1), shorts (2), or longs (4) */
      uint8_t     _reserved;   /*   reserved for later use */
    };
    uint8_t       data[64];    /* if data-package type: max. 64 byte payload */
  };
} SDGP_event_t;

/* event-payload-type for simple touchscreen events (no multitouch or similar) */
typedef struct SDGP_evpkt_simpletouch_s { /* 16 bytes */
  /* 12 bytes */
  int16_t    raw_x;               /* raw coordinate X */
  int16_t    raw_y;               /* raw coordinate Y */
  int16_t    raw_touch;           /* raw touch value */
  int16_t    norm_x;              /* normalised coordinate X (norm_x <= dd->width) */
  int16_t    norm_y;              /* normalised coordinate Y (norm_y <= dd->height) */
  int16_t    norm_touch;          /* normalised touch value */
} SDGP_evpkt_simpletouch_t;


typedef struct {
    bool simpleTouchChanged;
    int  simpleTouchX;
    int  simpleTouchY;
    int  simpleTouchT;
}  tTouchEvent;


typedef void (*fp_eventlistener_t) (void* dd, SDGP_event_t* recylce);

class cDriverConfig;

class cDriverSerDisp : public cDriver
{
private:

    long  serdisp_version;

    int   supports_options;

    void* sdhnd; // serdisplib handle
    void* dd;    // display descriptor
    void* sdcd;  // serdisp connect descriptor

    long  (*fp_serdisp_getversioncode) ();

    void* (*fp_SDCONN_open)            (const char sdcdev[]);

    void* (*fp_serdisp_init)           (void* sdcd, const char dispname[], const char extra[]);
    void  (*fp_serdisp_rewrite)        (void* dd);
    void  (*fp_serdisp_update)         (void* dd);
    void  (*fp_serdisp_clearbuffer)    (void* dd);
    void  (*fp_serdisp_setcolour)      (void* dd, int x, int y, long colour);
    int   (*fp_serdisp_feature)        (void* dd, int feature, int value);
    int   (*fp_serdisp_isoption)       (void* dd, const char* optionname);
    void  (*fp_serdisp_setoption)      (void* dd, const char* optionname, long value);
    long  (*fp_serdisp_getoption)      (void* dd, const char* optionname, int* typesize);
    int   (*fp_serdisp_getwidth)       (void* dd);
    int   (*fp_serdisp_getheight)      (void* dd);
    int   (*fp_serdisp_getcolours)     (void* dd);
    int   (*fp_serdisp_getdepth)       (void* dd);
    void  (*fp_serdisp_quit)           (void* dd);
    void  (*fp_serdisp_close)          (void* dd);
    uint8_t (*fp_SDGPI_search)         (void* dd, const char* gpname);
    int   (*fp_SDGPI_isenabled)        (void* dd, uint8_t gpid);
    int   (*fp_SDGPI_enable)           (void* dd, uint8_t gpid, int enable);
    int   (*fp_SDEVLP_add_listener)    (void* dd, uint8_t gpid, fp_eventlistener_t eventlistener );
    const char*
          (*fp_serdisp_defaultdevice)  (const char* dispname);

    int CheckSetup();

    void  eventListener                (void* dd, SDGP_event_t* recycle);
    
    tTouchEvent*                       touchEvent;

protected:
    virtual bool GetDriverFeature  (const std::string & Feature, int & value);
    virtual uint32_t GetDefaultBackgroundColor(void);

public:

    cDriverSerDisp(cDriverConfig * config);

    virtual int Init();
    virtual int DeInit();

    virtual void Clear();
    virtual void SetPixel(int x, int y, uint32_t data);

    virtual void Refresh(bool refreshAll = false);
    virtual void SetBrightness(unsigned int percent);

    virtual bool SetFeature  (const std::string & Feature, int value);

    virtual cGLCDEvent * GetEvent(void);

};

} // end of namespace
#endif

