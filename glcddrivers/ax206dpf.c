/*
 * GraphLCD driver library
 *
 * ax206dpf.h  -  AX206dpf driver class
 *                Output goes to AX 206 based photoframe
 *
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
 * (c) 2011      Lutz Neumann <superelchi AT wolke7.de>
 *
 * HISTORY
 * 
 * v0.1 - 10 Aug 2011 - Inital release
 * 
 */

#include <stdio.h>
#include <syslog.h>
#include <cstring>

#include "common.h"
#include "config.h"

#include "ax206dpf.h"


namespace GLCD
{
static     LIBDPF::DPFContext *dpfh;

cDriverAX206DPF::cDriverAX206DPF(cDriverConfig * config)
:   config(config)
{
    oldConfig = new cDriverConfig(*config);
}

cDriverAX206DPF::~cDriverAX206DPF()
{
    delete oldConfig;
}

int cDriverAX206DPF::Init(void)
{
    zoom = 1;
    bool forcePortrait = false;
    for (unsigned int i = 0; i < config->options.size(); i++)
    {
        if (config->options[i].name == "Portrait") {
            forcePortrait = config->options[i].value == "yes";
        }    
        else if (config->options[i].name == "Zoom") {
            int z = strtol(config->options[i].value.c_str(), (char **) NULL, 0);
            if (z > 0 && z <= 4)
                zoom = z;
            else
                syslog(LOG_ERR, "%s error: zoom %d not supported, using default (%d)!\n",config->name.c_str(), z, zoom);
    
        }
    }

    // Init display
        
   	int error;
	char index;

    if (config->device.length() != 4 || config->device.compare(0, 3, "dpf"))
        index = '0';
    else
        index = config->device.at(3);
    
    char usb_device[5];
    sprintf(usb_device, "usb%c", index);
        
	error = dpf_open(usb_device, &dpfh);
	if (error < 0) {
        syslog(LOG_INFO, "%s: can not open dpf device number '%c' (error %d).\n", config->name.c_str(), index, error);
		return -1;
	}

    // See, if we have to rotate the display
    isPortrait = dpfh->width < dpfh->height;
    rotate90 = isPortrait != forcePortrait;

    // Set width / height from display
    unsigned int vwidth = (!rotate90) ? dpfh->width : dpfh->height;
    unsigned int vheight = (!rotate90) ? dpfh->height : dpfh->width;
    width = config->width;
    if (width <= 0 || width > (int) vwidth)
        width = (int) vwidth;
    height = config->height;
    if (height <= 0 || height > (int) vheight)
        height = (int) vheight;
    if (zoom > 1)
    {
        height /= zoom;
        width /= zoom;
    }

    // setup lcd array
    LCD = (unsigned char *) malloc(dpfh->height * dpfh->width * dpfh->bpp);
    ResetMinMax();

    *oldConfig = *config;
    
    // Set Display Brightness -- NOT WORKING!
    //SetBrightness(config->brightness ? config->brightness : 100);
    syslog(LOG_INFO, "%s: AX206DPF initialized (%dx%d).\n", config->name.c_str(), width, height);
    return 0;
}

int cDriverAX206DPF::DeInit(void)
{
    dpf_close(dpfh);

    // free lcd array
    if (LCD)
    {
        free(LCD);
    }

    return 0;
}

void cDriverAX206DPF::ResetMinMax(void)
{
    miny = dpfh->height - 1;
    maxy = 0;
}

int cDriverAX206DPF::CheckSetup(void)
{
    if (config->width != oldConfig->width ||
        config->height != oldConfig->height)
    {
        DeInit();
        Init();
        return 0;
    }
    
    if (config->brightness != oldConfig->brightness)
    {
        oldConfig->brightness = config->brightness;
        SetBrightness(config->brightness);
    }

    if (config->upsideDown != oldConfig->upsideDown ||
        config->invert != oldConfig->invert)
    {
        oldConfig->upsideDown = config->upsideDown;
        oldConfig->invert = config->invert;
        return 1;
    }
    return 0;
}

void cDriverAX206DPF::Clear(void)
{
    memset(LCD, 0, dpfh->height * dpfh->width * dpfh->bpp);       //Black
    miny = 0;
    maxy = dpfh->height - 1;
}

#define _RGB565_0(p) \
	(( ((p >> 16) & 0xf8)	   ) | (((p >> 8) & 0xe0) >> 5))
#define _RGB565_1(p) \
	(( ((p >> 8) & 0x1c) << 3 ) | (((p) & 0xf8) >> 3))

void cDriverAX206DPF::SetPixel(int x, int y, uint32_t data)
{
    bool flip = config->upsideDown;
    if (!isPortrait && rotate90)
    flip = !flip;
    
    if (flip)
    {
        // upside down orientation
        x = width - 1 - x;
        y = height - 1 - y;
    }
    
    unsigned int i;
    if (rotate90)
    {
        // wrong Orientation, rotate
        int xn = y;
        y = dpfh->height - 1 - x;
        x = xn;
    }

    if (x >= ((int) dpfh->width / zoom) || y >= ((int) dpfh->height / zoom))
        return;

    y *= zoom;
    if (zoom == 1)
    {
        i = (y * dpfh->width + x) * dpfh->bpp;
        LCD[i]   = _RGB565_0(data);
        LCD[i+1] = _RGB565_1(data);
    }
    else
    {
        for (int dy = 0; dy < zoom; dy++)
        {
            i = ((y + dy) * dpfh->width + (x * zoom)) * dpfh->bpp;
            for (int dx = 0; dx < zoom * dpfh->bpp; dx += dpfh->bpp)
            {
                LCD[i+dx]   = _RGB565_0(data);
                LCD[i+dx+1] = _RGB565_1(data);
            }
        }
    }
    if (y * zoom < miny) miny = y;
    if (y * zoom > maxy) maxy = y;
}

void cDriverAX206DPF::Refresh(bool refreshAll)
{

    if (CheckSetup() > 0)
        refreshAll = true;

	short rect[4];
    
    if (!refreshAll)
    {
	rect[0] = 0; rect[1] = miny;
	rect[2] = dpfh-> width; rect[3] = maxy + 1;
    }
    else
    {
	rect[0] = 0; rect[1] = 0;
	rect[2] = dpfh->width; rect[3] = dpfh->height;
    }
    
    if (rect[1] > rect[3])
        return;
        
	dpf_screen_blit(dpfh, &LCD[rect[1] * dpfh->width * dpfh->bpp], rect);
    ResetMinMax();
}

GLCD::cColor cDriverAX206DPF::GetBackgroundColor(void)
{
    return GLCD::cColor::Black;
}

/* NOT WORKING - WILL FREEZE THE DISPLAY FROM TIME TO TIME!
void cDriverAX206DPF::SetBrightness(unsigned int percent)
{
    LIBDPF::DPFValue val;
    val.type = LIBDPF::TYPE_INTEGER;
    
    // Brightness can be 0 .. 7
    if (percent == 0)
        val.value.integer = 0;
    else if (percent >= 100)
        val.value.integer = 7;
    else    
        val.value.integer = (((percent * 10) + 167) * 6) / 1000;
    dpf_setproperty(dpfh, PROPERTY_BRIGHTNESS, &val);
}
*/

bool cDriverAX206DPF::GetDriverFeature(const std::string & Feature, int & value)
{
    if (strcasecmp(Feature.c_str(), "depth") == 0) {
        value = 16;
        return true;
    } else if (strcasecmp(Feature.c_str(), "ismonochrome") == 0) {
        value = false;
        return true;
    } else if (strcasecmp(Feature.c_str(), "isgreyscale") == 0 || strcasecmp(Feature.c_str(), "isgrayscale") == 0) {
        value = false;
        return true;
    } else if (strcasecmp(Feature.c_str(), "iscolour") == 0 || strcasecmp(Feature.c_str(), "iscolor") == 0) {
        value = true;
        return true;
    } else if (strcasecmp(Feature.c_str(), "touch") == 0 || strcasecmp(Feature.c_str(), "touchscreen") == 0) {
        value = false;
        return true;
    }
    value = 0;
    return false;
}

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

namespace LIBDPF
{
// -------------------------------------------------------------------
// START SELECTIVE COPY & PASTE "dpflib.c"
// -------------------------------------------------------------------
/** DPF access library for AX206 based HW
 *
 * 12/2010 <hackfin@section5.ch>
 *
 * This is an ugly hack, because we use existing SCSI vendor specific
 * extensions to pass on our requests to the DPF.
 *
 * One day we might use a proper protocol like netpp.
 *
 */

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <scsi/scsi.h>
#include <scsi/sg.h>
#include <sys/ioctl.h>

/** Vendor command for our hacks */
static
unsigned char g_excmd[16] = {
	0xcd, 0, 0, 0,
	   0, 6, 0, 0,
	   0, 0, 0, 0,
	   0, 0, 0, 0
};

int wrap_scsi(DPFContext *h, unsigned char *cmd, int cmdlen, char out,
	unsigned char *data, unsigned long block_len)
{
	int error;
	//if (h->mode == MODE_USB) {
		error = emulate_scsi(h->dev.udev, cmd, cmdlen, out, data, block_len);
	//} else {
	//	error = do_scsi(h->dev.fd, cmd, cmdlen, out, data, block_len);
	//}
	return error;
}

static
int probe(DPFHANDLE h)
{
	int ret;

	// We abuse a command that just responds with a '0' status in the
	// original firmware.
	static unsigned char buf[5];


	static
	unsigned char cmd[16] = {
		0xcd, 0, 0, 0,
		   0, 3, 0, 0,
		   0, 0, 0, 0,
		   0, 0, 0, 0
	};

	cmd[5] = 3;
	ret = wrap_scsi(h, cmd, sizeof(cmd), DIR_IN, 0, 0);

	switch (ret) {
		case 0:
			// The original protocol.
			fprintf(stderr,
				"Warning: This firmware can not lock the flash\n");
			break;
		case 1:
			// The improved hack
			h->flags |= FLAG_CAN_LOCK;
			break;
	}

	cmd[5] = 2; // get LCD parameters
	ret = wrap_scsi(h, cmd, sizeof(cmd), DIR_IN, buf, 5);
	h->width = (buf[0]) | (buf[1] << 8);
	h->height = (buf[2]) | (buf[3] << 8);
	h->bpp = 2;

	return ret;
}


int dpf_open(const char *dev, DPFHANDLE *h)
{
	int error = 0;
	DPFContext *dpf;
	int i;
	usb_dev_handle *u;

	//int fd;

	if (!dev) {
		fprintf(stderr, "Please specify a string like 'usb0' or a sg device\n");
		return DEVERR_OPEN;
	}

	if (strncmp(dev, "usb", 3) == 0) {
		i = dev[3] - '0';
		u = dpf_usb_open(i);
		if (!u) return DEVERR_OPEN;
		//i = MODE_USB;
    } else {
        return DEVERR_OPEN;
    }
	//} else {
	//	fprintf(stderr, "Opening generic SCSI device '%s'\n", dev);
	//	if (sgdev_open(dev, &fd) < 0) return DEVERR_OPEN;
	//	i = MODE_SG;
	//}

	dpf = (DPFHANDLE) malloc(sizeof(DPFContext));
	if (!dpf) return DEVERR_MALLOC;

	dpf->flags = 0;
	dpf->mode = i;

	//if (dpf->mode == MODE_USB) {
		dpf->dev.udev = u;
		error = probe(dpf);
		fprintf(stderr, "Got LCD dimensions: %dx%d\n", dpf->width, dpf->height);
	//} else {
	//	dpf->dev.fd = fd;
	//}

	*h = dpf;
	return error;
}

void dpf_close(DPFContext *h)
{
	//switch (h->mode) {
	//	case MODE_SG:
	//		close(h->dev.fd);
	//		break;
	//	case MODE_USB:
			usb_release_interface(h->dev.udev, 0);
			usb_close(h->dev.udev);
	//		break;
	//}
	free(h);
}

const char *dev_errstr(int err)
{
	switch (err) {
	case DEVERR_FILE: return "File I/O error";
	case DEVERR_OPEN: return "File open error";
	case DEVERR_HEX: return "Hex file error";
	case DEVERR_CHK: return "Checksum error";
	case DEVERR_IO: return "Common I/O error";
	default: return "Unknown error";
	}
}

#define RGB565_0(r, g, b) \
	(( ((r) & 0xf8)		 ) | (((g) & 0xe0) >> 5))
#define RGB565_1(r, g, b) \
	(( ((g) & 0x1c) << 3 ) | (((b) & 0xf8) >> 3))

int dpf_setcol(DPFContext *h, const unsigned char *rgb)
{
	unsigned char *cmd = g_excmd;

	cmd[6] = USBCMD_SETPROPERTY;
	cmd[7] = PROPERTY_FGCOLOR;
	cmd[8] = PROPERTY_FGCOLOR >> 8;

	cmd[9] = RGB565_0(rgb[0], rgb[1], rgb[2]);
	cmd[10] = RGB565_1(rgb[0], rgb[1], rgb[2]);

	return wrap_scsi(h, cmd, sizeof(g_excmd), DIR_OUT, NULL, 0);
}

int dpf_screen_blit(DPFContext *h,
	const unsigned char *buf, short rect[4])
{
	unsigned long len = (rect[2] - rect[0]) * (rect[3] - rect[1]);
	len <<= 1;
	unsigned char *cmd = g_excmd;

	cmd[6] = USBCMD_BLIT;
	cmd[7] = rect[0];
	cmd[8] = rect[0] >> 8;
	cmd[9] = rect[1];
	cmd[10] = rect[1] >> 8;
	cmd[11] = rect[2] - 1;
	cmd[12] = (rect[2] - 1) >> 8;
	cmd[13] = rect[3] - 1;
	cmd[14] = (rect[3] - 1) >> 8;
	cmd[15] = 0;

	return wrap_scsi(h, cmd, sizeof(g_excmd), DIR_OUT,
		(unsigned char*) buf, len);
}

int dpf_setproperty(DPFContext *h, int token, const DPFValue *value)
{
	unsigned char *cmd = g_excmd;

	cmd[6] = USBCMD_SETPROPERTY;
	cmd[7] = token;
	cmd[8] = token >> 8;

	switch (value->type) {
		case TYPE_INTEGER:
			cmd[9] = value->value.integer;
			cmd[10] = value->value.integer >> 8;
			break;
		default:
			break;
	}

	return wrap_scsi(h, cmd, sizeof(g_excmd), DIR_OUT, NULL, 0);
}

// -------------------------------------------------------------------
// END SELECTIVE COPY & PASTE "dpflib.c"
// -------------------------------------------------------------------

// -------------------------------------------------------------------
// START SELECTIVE COPY & PASTE "rawusb.c"
// -------------------------------------------------------------------

/* Low level USB code to access DPF.
 *
 * (c) 2010, 2011 <hackfin@section5.ch>
 *
 * This currently uses the SCSI command set
 *
 * The reason for this is that we want to access the hacked frame
 * non-root and without having to wait for the SCSI interface to
 * intialize.
 *
 * Later, we'll replace the SCSI command stuff.
 */

#define ENDPT_OUT 1
#define ENDPT_IN 0x81

struct known_device {
	const char *desc;
	unsigned short vid;
	unsigned short pid;
} g_known_devices[] = {
	{ "AX206 DPF", 0x1908, 0x0102 },
	{ 0 , 0, 0 } /* NEVER REMOVE THIS */
};

int handle_error(const char *txt)
{
	fprintf(stderr, "Error: %s\n", txt);
	return -1;
}

void usb_flush(usb_dev_handle *dev)
{
	char buf[20];
	usb_bulk_read(dev, ENDPT_IN, buf, 3, 1000);
}

int check_known_device(struct usb_device *d)
{
	struct known_device *dev = g_known_devices;

	while (dev->desc) {
		if ((d->descriptor.idVendor == dev->vid) &&
			(d->descriptor.idProduct == dev->pid)) { 
				fprintf(stderr, "Found %s\n", dev->desc);
				return 1;
		}
		dev++;
	}
	return 0;
}

static struct usb_device *find_dev(int index)
{
	struct usb_bus *b;
	struct usb_device *d;
	int enumeration = 0;

	b = usb_get_busses();

	while (b) {
		d = b->devices;
		while (d) {
			if (check_known_device(d)) {
				if (enumeration == index) return d;
				else enumeration++;
			}

#ifdef DEBUG
			printf("%04x %04x\n",
				   d->descriptor.idVendor,
				   d->descriptor.idProduct);
#endif
			d = d->next;
		}
		b = b->next;
	}
	return NULL;
}

char g_buf[] = {
	0x55, 0x53, 0x42, 0x43, // dCBWSignature
	0xde, 0xad, 0xbe, 0xef, // dCBWTag
	0x00, 0x80, 0x00, 0x00, // dCBWLength
	0x00, // bmCBWFlags: 0x80: data in (dev to host), 0x00: Data out
	0x00, // bCBWLUN
	0x10, // bCBWCBLength

	// SCSI cmd:
	0xcd, 0x00, 0x00, 0x00,
	0x00, 0x06, 0x11, 0xf8,
	0x70, 0x00, 0x40, 0x00,
	0x00, 0x00, 0x00, 0x00,
};

int emulate_scsi(usb_dev_handle *dev, unsigned char *cmd, int cmdlen, char out,
	unsigned char *data, unsigned long block_len)
{
	int len;
	int ret;
	static unsigned char ansbuf[13]; // Do not change size.

	g_buf[14] = cmdlen;
	memcpy(&g_buf[15], cmd, cmdlen);

	g_buf[8] = block_len;
	g_buf[9] = block_len >> 8;
	g_buf[10] = block_len >> 16;
	g_buf[11] = block_len >> 24;

	ret = usb_bulk_write(dev, ENDPT_OUT, g_buf, sizeof(g_buf), 1000);
	if (ret < 0) return ret;

	if (out == DIR_OUT) {
		if (data) {
			ret = usb_bulk_write(dev, ENDPT_OUT, (char* )data,
					block_len, 3000);
			if (ret != (int) block_len) {
				perror("bulk write");
				return ret;
			}
		}
	} else if (data) {
		ret = usb_bulk_read(dev, ENDPT_IN, (char *) data, block_len, 4000);
		if (ret != (int) block_len) {
			perror("bulk data read");
		}
	}
	// get ACK:
	len = sizeof(ansbuf);
	int retry = 0;
	do {
		ret = usb_bulk_read(dev, ENDPT_IN, (char *) ansbuf, len, 5000);
		if (ret != len) {
			perror("bulk ACK read");
			ret = DEVERR_TIMEOUT;
		}
		retry++;
	} while (ret == DEVERR_TIMEOUT && retry < 5);
	if (strncmp((char *) ansbuf, "USBS", 4)) {
		return handle_error("Got invalid reply\n");
	}
	// pass back return code set by peer:
	return ansbuf[12];
}

usb_dev_handle *dpf_usb_open(int index)
{
	struct usb_device *d;
	usb_dev_handle *usb_dev;

	usb_init();
	usb_find_busses();
	usb_find_devices();

	d = find_dev(index);
	if (!d) {
		handle_error("No matching USB device found!");
		return NULL;
	}

	usb_dev = usb_open(d);
	if (usb_dev == NULL) {
		handle_error("Failed to open usb device!");
		return NULL;
	}
	usb_claim_interface(usb_dev, 0);
	return usb_dev;
}

// -------------------------------------------------------------------
// END SELECTIVE COPY & PASTE "rawusb.c"
// -------------------------------------------------------------------
    
    
} // end of namespace LIBDPF

//##########################################################################################
// ** END OF UGLY HACK ** END OF UGLY HACK ** END OF UGLY HACK ** END OF UGLY HACK *
//##########################################################################################
