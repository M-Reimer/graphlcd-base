/*
 * GraphLCD driver library
 *
 * ax206dpf.h  -  AX206dpf driver class
 *                Output goes to AX 206 based photoframe
 * based on:
 *   simlcd device
 *     (c) 2001-2004 Carsten Siebholz <c.siebholz AT t-online.de>
 *     (c) 2011      Dirk Heiser
 *     (c) 2011      Wolfgang Astleitner <mrwastl AT users.sourceforge.net>
 * 
 * includes code from:
 *  http://sourceforge.net/projects/dpf-ax/
 * 
 *  Original copyright:
 * 
 *  Copyright (C) 2008 Jeroen Domburg <picframe@spritesmods.com>
 *  Modified from sample code by:
 *  Copyright (C) 2005 Michael Reinelt <michael@reinelt.co.at>
 *  Copyright (C) 2005, 2006, 2007 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
 *  Mods by <hackfin@section5.ch>
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2011      Lutz Neumann <superelchi AT wolke7.net>
 * 
 */

#ifndef _GLCDDRIVERS_AX206DPF_H_
#define _GLCDDRIVERS_AX206DPF_H_

#include "driver.h"

namespace LIBDPF {
struct dpf_context;
typedef dpf_context DPFContext;
}

namespace GLCD
{
#define MAX_DPFS 4

#define DEFAULT_WIDTH  320
#define DEFAULT_HEIGHT 240
#define DEFAULT_BPP    2

#define USB_SCAN_INTERVALL  10       // seconds between usb scans for missing displays

typedef struct display_handle {
    bool attached;
    char address[8];
    bool isPortrait;
    bool rotate90;
    bool flip;
    int minx, maxx;
    int miny, maxy;
    LIBDPF::DPFContext *dpfh;
    unsigned char * LCD;
} DISPLAYHANDLE;


class cDriverConfig;

class cDriverAX206DPF : public cDriver
{
private:
    unsigned char * tempLCD;        // temp transfer buffer

    bool portrait;                  // portrait or landscape mode
    int zoom;                       // pixel zoom factor
    unsigned int numdisplays;       // number of detected displays
    unsigned int numxdisplays;      // number of displays (horizontal)
    unsigned int numydisplays;      // number of displays (vertical)
    unsigned int sizex;             // logical horizontal size of one display
    unsigned int sizey;             // logical vertical size of one display
    unsigned int bpp;               // bits per pixel
    
    DISPLAYHANDLE *dh[MAX_DPFS];
    std::string flips;
    time_t lastscan;
    int lastbrightness;

    
    int CheckSetup();
    void ResetMinMax();
    bool RescanUSB();
    int InitSingleDisplay(unsigned int);
    void DeInitSingleDisplay(unsigned int);
    void ClearSingleDisplay(unsigned int);
    void SetSingleDisplayBrightness(unsigned int, unsigned int);

public:
    cDriverAX206DPF(cDriverConfig * config);

    virtual int Init();
    virtual int DeInit();

    virtual void Clear();
    virtual void SetPixel(int x, int y, uint32_t data);
    virtual void Refresh(bool refreshAll = false);
    virtual uint32_t GetBackgroundColor(void);
    virtual void SetBrightness(unsigned int);
    virtual bool GetDriverFeature  (const std::string & Feature, int & value);
};

} // end of namespace GLCD


//##########################################################################################
//
// ** START OF UGLY HACK ** START OF UGLY HACK ** START OF UGLY HACK ** START OF UGLY HACK *
//
// Because I choose NOT to include the static library libdpf.a and/or their header- and
// source-files from the original dpf-ax hack, I did some creative copy & paste from there
// to this place. I will delete this stuff when a usable shared library of libpdf exists.
//
//##########################################################################################

// -------------------------------------------------------------------
// START SELECTIVE COPY & PASTE "dpf.h"
// -------------------------------------------------------------------

/** libdpf header file
 *
 * (c) 2010, 2011 <hackfin@section5.ch>
 *
 */

// -------------------------------------------------------------------
// START SELECTIVE COPY & PASTE "usbuser.h"
// -------------------------------------------------------------------

#include <usb.h>

namespace LIBDPF
{

/* USB user commands
 *
 * Only temporary. Should move to dpflib or into a dclib configuration.
 *
 */

#define PROTOCOL_VERSION  1

/** Our vendor specific USB commands to do stuff on the DPF */

#define USBCMD_GETPROPERTY  0x00    ///< Get property
#define USBCMD_SETPROPERTY  0x01    ///< Set property
#define USBCMD_MEMREAD      0x04    ///< Memory read
#define USBCMD_APPLOAD      0x05    ///< Load and run applet
#define USBCMD_FILLRECT     0x11    ///< Fill screen rectangle
#define USBCMD_BLIT         0x12    ///< Blit to screen
#define USBCMD_COPYRECT     0x13    ///< Copy screen rectangle
#define USBCMD_FLASHLOCK    0x20    ///< Lock USB for flash access
#define USBCMD_PROBE        0xff    ///< Get version code (probe)

/* Some special return codes */
#define USB_IN_SEQUENCE     0x7f    ///< We're inside a command sequence

// Property handling:

#define PROPERTY_BRIGHTNESS  0x01
#define PROPERTY_FGCOLOR     0x02
#define PROPERTY_BGCOLOR     0x03
#define PROPERTY_ORIENTATION 0x10

// -------------------------------------------------------------------
// END SELECTIVE COPY & PASTE "usbuser.h"
// -------------------------------------------------------------------

#define ADDR unsigned int

//#define MODE_SG  0x00  ///< generic device mode (original)
//#define MODE_USB 0x01  ///< libusb operation mode (hacked)
#define FLAG_CAN_LOCK 0x80  ///< Has the locking feature (new firmware)

enum {
	DEVERR_FILE = -16,
	DEVERR_OPEN,
	DEVERR_HEX,
	DEVERR_CHK,
	DEVERR_IO,
	DEVERR_MALLOC,
	DEVERR_TIMEOUT,
};

/** The DPF context structure */

typedef
struct dpf_context {
	unsigned char mode;
	unsigned char flags;
	union {
		usb_dev_handle *udev;
		int fd;
	} dev;
	unsigned int width;
	unsigned int height;
	int bpp;
	int proto;
	char* buff;
	unsigned char* oldpix;
	int offx;
	int offy;
} DPFContext;

/** A value proxy for the property API */
typedef struct dpf_proxy {
	union {
		short	integer;
		char   *sequence;
	} value;
	char type;
} DPFValue;

enum {
	TYPE_INTEGER,
	TYPE_STRING,
}; 

#define DPFHANDLE struct dpf_context *

#ifdef __cplusplus
extern "C" {
#endif

/**
 Opens the DPF device. if dev is not NULL, open device, otherwise, look for
 USB device.
 */
int dpf_open(const char *dev, DPFHANDLE *h);

/** Close DPF device */
void dpf_close(DPFHANDLE h);

/** Blit data to screen
 *
 * \param buf     buffer to 16 bpp RGB 565 image data
 * \param rect    rectangle tuple: [x0, y0, x1, y1]
 */

int dpf_screen_blit(DPFHANDLE h, const unsigned char *buf, short rect[4]);

/** Set property on DPF
 * \param token       Property token
 * \param value       Pointer to value
 */
int dpf_setproperty(DPFHANDLE h, int token, const DPFValue *value);

/* USB raw */

int emulate_scsi(usb_dev_handle *d, unsigned char *cmd, int cmdlen, char out,
	unsigned char *data, unsigned long block_len);

const char *dev_errstr(int err);

// Private stuff:
usb_dev_handle *dpf_usb_open(int index);
int sgdev_open(const char *portname, int *fd);

#ifdef __cplusplus
}
#endif

// Some internal address offsets. They may change, but so far all types
// seem to have the same
//
// w: word, <n>: length, [LE, BE]
//
// FIXME: Use packed struct later.

// FIXME: Should be 0x0020, once we have the firmware replaced
#define OFFSET_PROPS 0x3f0020   ///< w[2]:LE : Resolution X, Y

// -------------------------------------------------------------------
// END SELECTIVE COPY & PASTE "dpf.h"
// -------------------------------------------------------------------

// -------------------------------------------------------------------
// START SELECTIVE COPY & PASTE "sglib.h"
// -------------------------------------------------------------------

/* generic SCSI device stuff: */

#define DIR_IN  0
#define DIR_OUT 1

// -------------------------------------------------------------------
// END SELECTIVE COPY & PASTE "sglib.h"
// -------------------------------------------------------------------

} // end of namespace LIBDPF_HACK

//##########################################################################################
// ** END OF UGLY HACK ** END OF UGLY HACK ** END OF UGLY HACK ** END OF UGLY HACK *
//##########################################################################################

#endif //_GLCDDRIVERS_AX206DPF_H_
