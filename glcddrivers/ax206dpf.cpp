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
 * (c) 2011      Lutz Neumann <superelchi AT wolke7.net>
 *
 * HISTORY
 * 
 * v0.1 - 10 Aug 2011 - Inital release
 * v0.2 - 20 Aug 2011 - Optimized display data transfer
 *                      SetBrightness() implemented
 *                      Multi-display support
 * v0.3 - 02 Sep 2011 - Fixed multi-thread problem
 * 
 * 
 */

#include <stdio.h>
#include <syslog.h>
#include <cstring>
#include <algorithm>
#include <pthread.h>
#include <time.h>
#include <usb.h>

#include "common.h"
#include "config.h"

#include "ax206dpf.h"


namespace GLCD
{

static	pthread_mutex_t libax_mutex;


cDriverAX206DPF::cDriverAX206DPF(cDriverConfig * config)
:   cDriver(config)
{
}

int cDriverAX206DPF::Init(void)
{
    zoom = 1;
    portrait = false;
    numxdisplays = numydisplays = 1;
    sizex = sizey = bpp = 0;
    
    for (unsigned int i = 0; i < MAX_DPFS; i++)
    {
        dh[i] = (DISPLAYHANDLE *) malloc(sizeof(DISPLAYHANDLE));
        dh[i]->attached = false;
        dh[i]->address[0] = 0;
        dh[i]->dpfh = NULL;
        dh[i]->LCD = NULL;
    }

    lastbrightness = config->brightness ? config->brightness : 100;
    
    for (unsigned int i = 0; i < config->options.size(); i++)
    {
        if (config->options[i].name == "Portrait") {
            portrait = config->options[i].value == "yes";
        }    
        else if (config->options[i].name == "Zoom") {
            int z = strtol(config->options[i].value.c_str(), (char **) NULL, 0);
            if (z > 0 && z <= 4)
                zoom = z;
            else
                syslog(LOG_ERR, "%s error: zoom %d not supported, using default (%d)!\n",config->name.c_str(), z, zoom);
        }
        else if (config->options[i].name == "Horizontal") {
            int h = strtol(config->options[i].value.c_str(), (char **) NULL, 0);
            if (h > 0 && h <= 4)
                numxdisplays = h;
            else
                syslog(LOG_ERR, "%s error: Horizontal=%d not supported, using default (%d)!\n",config->name.c_str(), h, numxdisplays);
        }
        else if (config->options[i].name == "Vertical") {
            int v = strtol(config->options[i].value.c_str(), (char **) NULL, 0);
            if (v > 0 && v <= 4)
                numydisplays = v;
            else
                syslog(LOG_ERR, "%s error: Vertical=%d not supported, using default (%d)!\n",config->name.c_str(), v, numydisplays);
        }
        else if (config->options[i].name == "Flip") {
            flips = config->options[i].value;
            for (unsigned int j = 0; j < flips.size(); j++)
            {
                if (flips[j] != 'y' && flips[j] != 'n')
                {
                syslog(LOG_ERR, "%s error: flips=%s - illegal character, only 'y' and 'n' supported, using default!\n",config->name.c_str(), flips.c_str());
                flips = "";
                break;
                }
            }
        }
    }

    // See if we have too many displays
    if (numxdisplays * numydisplays > MAX_DPFS)
        syslog(LOG_ERR, "%s: too many displays (%dx%d). Max is %d!\n",config->name.c_str(), numxdisplays, numydisplays, MAX_DPFS);    
    
    // Init all displays
    numdisplays = 0;
   	int error = 0;
    for (unsigned int i = 0; i < numxdisplays * numydisplays; i++)
    {
        error = InitSingleDisplay(i);
        if (error < 0)
            return -1;
        numdisplays++;
    }
    
    if (sizex == 0)
    {
        // no displays detected
        sizex = (portrait) ? DEFAULT_HEIGHT : DEFAULT_WIDTH;
        sizey = (portrait) ? DEFAULT_WIDTH : DEFAULT_HEIGHT;
        bpp = DEFAULT_BPP;
    }
    
    // setup temp transfer LCD array
    tempLCD = (unsigned char *) malloc(sizex * sizey * bpp);
    
    width = sizex * numxdisplays;
    height = sizey * numydisplays;
    
    if (zoom > 1)
    {
        height /= zoom;
        width /= zoom;
    }
    
    ResetMinMax();

    *oldConfig = *config;

    if (numdisplays == 1)
        syslog(LOG_INFO, "%s: AX206DPF initialized (%dx%d).\n", config->name.c_str(), width, height);
    else
    {
        unsigned n = 0;
        for (unsigned int i = 0; i < numdisplays; i++)
            if (dh[i]->attached) n++;
        syslog(LOG_INFO, "%s: using %d display(s) (%d online, %d offline).\n", config->name.c_str(), numdisplays, n, numdisplays - n);
    }
    
    lastscan = time(NULL);
    
    return 0;
}

bool cDriverAX206DPF::RescanUSB()
{
    bool ret = false;
    usb_find_busses();
    if (usb_find_devices() > 0)
    {
        unsigned int a = 0, b = 0;
        for (unsigned int i = 0; i < numdisplays; i++)
        {
            if (dh[i]->attached) a |= 0x01 << i;
            DeInitSingleDisplay(i);
        }
        for (unsigned int i = 0; i < numdisplays; i++)
        {
            InitSingleDisplay(i);
            if (dh[i]->attached) b |= 0x01 << i;
        }
        ret = a != b;
    }
    return ret;
}

int cDriverAX206DPF::InitSingleDisplay(unsigned int di)
{
	char index;
    
    if (config->device.length() != 4 || config->device.compare(0, 3, "dpf"))
        index = '0';
    else
        index = config->device.at(3);
    
    char device[5];
    sprintf(device, "usb%c", index + di);
    int error = dpf_open(device, &dh[di]->dpfh);
    if (error < 0)
    {
        dh[di]->dpfh = NULL;
        dh[di]->attached = false;
        return 0;
    }
    dh[di]->attached = true;
    struct usb_device *dev = usb_device(dh[di]->dpfh->dev.udev);
    char *s1 = dev->bus->dirname;
    char *s2 = dev->filename;
    if (strlen(s1) > 3) s1 = (char *) "???";
    if (strlen(s2) > 3) s2 = (char *) "???";
    sprintf(dh[di]->address, "%s:%s", s1, s2);
    
    // See, if we have to rotate the display
    dh[di]->isPortrait = dh[di]->dpfh->width < dh[di]->dpfh->height;
    dh[di]->rotate90 = dh[di]->isPortrait != portrait;
    dh[di]->flip = (!dh[di]->isPortrait && dh[di]->rotate90);    // adjust to make rotate por/land = physical por/land
    if (flips.size() >= di + 1 && flips[di] == 'y')
        dh[di]->flip = !dh[di]->flip;

    if (sizex == 0)
    {
        // this is the first display found
        // Get width / height from this display (all displays have same geometry)
        sizex = ((!dh[di]->rotate90) ? dh[di]->dpfh->width : dh[di]->dpfh->height);
        sizey = ((!dh[di]->rotate90) ? dh[di]->dpfh->height : dh[di]->dpfh->width);
        bpp = dh[di]->dpfh->bpp;
    }
    else
    {
        // make sure alle displays have the same geometry
        if ((!(sizex == dh[di]->dpfh->width && sizey == dh[di]->dpfh->height) &&
            !(sizex == dh[di]->dpfh->height && sizey == dh[di]->dpfh->width)) ||
            bpp != (unsigned int) dh[di]->dpfh->bpp)
        {
        syslog(LOG_INFO, "%s: all displays must have same geometry. Display %d has not. Giving up.\n", config->name.c_str(), di);
        return -1;
        }
    }
    // setup physical lcd arrays
    dh[di]->LCD = (unsigned char *) malloc(dh[di]->dpfh->height * dh[di]->dpfh->width * dh[di]->dpfh->bpp);
    ClearSingleDisplay(di);

    // Set Display Brightness
    SetSingleDisplayBrightness(di, lastbrightness);


    // Reorder displays
    bool changed = false;
    for (unsigned int i = 0; i < MAX_DPFS - 1; i++)
    {
        for (unsigned int j = i + 1; j < MAX_DPFS; j++)
            {
            if (strcmp(dh[i]->address, dh[j]->address) < 0)
            {
                DISPLAYHANDLE *h = dh[i];
                dh[i] = dh[j];
                dh[j] = h;
                changed = true;
            }
        }
    }

    //for (unsigned int i = 0; i < MAX_DPFS; i++)
    //    fprintf(stderr, "Display %d at %s\n", i, (dh[i]->attached) ? dh[i]->address : "-none-");
    //fprintf(stderr, "\n");

    //fprintf(stderr, "Display %d at %s attached.\n", di, dh[di]->address);
    //syslog(LOG_INFO, "%s: display %d at %s attached\n", config->name.c_str(), di, dh[di]->address);
    
    return 0;
}

void cDriverAX206DPF::DeInitSingleDisplay(unsigned int di)
{
    if (dh[di]->dpfh != NULL)
        dpf_close(dh[di]->dpfh);
    dh[di]->dpfh = NULL;

    if (dh[di]->LCD != NULL)
        free(dh[di]->LCD);
    dh[di]->LCD = NULL;
    
    dh[di]->attached = false;
    dh[di]->address[0] = 0;
}

int cDriverAX206DPF::DeInit(void)
{
    // close displays & free lcd arrays
    for (unsigned int i = 0; i< numdisplays; i++)
        DeInitSingleDisplay(i);

    if (tempLCD)
        free(tempLCD);

    return 0;
}


void cDriverAX206DPF::ResetMinMax(void)
{
    for (unsigned int i = 0; i < numydisplays; i++)
    {
        if (dh[i]->attached)
        {
            dh[i]->minx = dh[i]->dpfh->width - 1;
            dh[i]->maxx = 0;
            dh[i]->miny = dh[i]->dpfh->height - 1;
            dh[i]->maxy = 0;
        }
    }
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

void cDriverAX206DPF::ClearSingleDisplay(unsigned int di)
{
    if (dh[di]->attached)
    {
        memset(dh[di]->LCD, 0, dh[di]->dpfh->width * dh[di]->dpfh->height * dh[di]->dpfh->bpp);       //Black
        dh[di]->minx = 0;
        dh[di]->maxx = dh[di]->dpfh->width - 1;
        dh[di]->miny = 0;
        dh[di]->maxy = dh[di]->dpfh->height - 1;
    }
}

void cDriverAX206DPF::Clear(void)
{
    for (unsigned int i = 0; i < numdisplays; i++)
        ClearSingleDisplay(i);
}

#define _RGB565_0(p) \
	(( ((p >> 16) & 0xf8)	   ) | (((p >> 8) & 0xe0) >> 5))
#define _RGB565_1(p) \
	(( ((p >> 8) & 0x1c) << 3 ) | (((p) & 0xf8) >> 3))

void cDriverAX206DPF::SetPixel(int x, int y, uint32_t data)
{
    bool changed = false;
    
    if (config->upsideDown)
    {
        // global upside down orientation
        x = width - 1 - x;
        y = height - 1 -y;
    }

    int sx = sizex / zoom;
    int sy = sizey / zoom;
    int di = (y / sy) * numxdisplays + (x / sx);
    int lx = (x % sx) * zoom;
    int ly = (y % sy) * zoom;

    if (!dh[di]->attached)
        return;
        
    if (dh[di]->flip)
    {
        // local upside down orientation
        lx = sizex - 1 - lx;
        ly = sizey - 1 - ly;
    }

    if (dh[di]->rotate90)
    {
        // wrong Orientation, rotate
        int i = ly;
        ly = (dh[di]->dpfh->height) - 1 - lx;
        lx = i;
    }

    if (lx < 0 || lx >= (int) dh[di]->dpfh->width || ly < 0 || ly >= (int) dh[di]->dpfh->height)
    {
        syslog(LOG_INFO, "x/y out of bounds (x=%d, y=%d, di=%d, rot=%d, flip=%d, lx=%d, ly=%d)\n", x, y, di, dh[di]->rotate90, dh[di]->flip, lx, ly);
        return;
    }

    unsigned char c1 = _RGB565_0(data);
    unsigned char c2 = _RGB565_1(data);
    
    if (zoom == 1)
    {
        unsigned int i = (ly * dh[di]->dpfh->width + lx) * dh[di]->dpfh->bpp;
        if (dh[di]->LCD[i] != c1 || dh[di]->LCD[i+1] != c2)
        {
            dh[di]->LCD[i]   = c1;
            dh[di]->LCD[i+1] = c2;
            changed = true;
        }
    }
    else
    {
        for (int dy = 0; dy < zoom; dy++)
        {
            unsigned int i = ((ly + dy) * dh[di]->dpfh->width + lx) * dh[di]->dpfh->bpp;
            for (int dx = 0; dx < zoom * dh[di]->dpfh->bpp; dx += dh[di]->dpfh->bpp)
            {
                if (dh[di]->LCD[i+dx] != c1 || dh[di]->LCD[i+dx+1] != c2)
                {
                    dh[di]->LCD[i+dx]   = c1;
                    dh[di]->LCD[i+dx+1] = c2;
                    changed = true;
                }
            }
        }
    }

    if (changed)
    {
        if (lx < dh[di]->minx) dh[di]->minx = lx;
        if (lx > dh[di]->maxx) dh[di]->maxx = lx;
        if (ly < dh[di]->miny) dh[di]->miny = ly;
        if (ly > dh[di]->maxy) dh[di]->maxy = ly;
    }
}

void cDriverAX206DPF::Refresh(bool refreshAll)
{
	short rect[4];

    if (CheckSetup() > 0)
        refreshAll = true;
    
    for (unsigned int di = 0; di < numdisplays; di++)
    {
        if (!dh[di]->attached)
        {
            time_t current = time(NULL);
            if (current - lastscan >= USB_SCAN_INTERVALL)
            {
                lastscan = current;
                if (RescanUSB())
                    ; //return;             // something changed, wait for next refresh
            }
        }
        
        if (!dh[di]->attached)
            continue;
            
        if (refreshAll)
        {
            dh[di]->minx = 0; dh[di]->miny = 0;
            dh[di]->maxx = dh[di]->dpfh->width - 1; dh[di]->maxy = dh[di]->dpfh->height - 1;
        }
        //fprintf(stderr, "%d: (%d,%d)-(%d,%d) ", di, dh[di]->minx, dh[di]->miny, dh[di]->maxx, dh[di]->maxy);
        if (dh[di]->minx > dh[di]->maxx || dh[di]->miny > dh[di]->maxy)
            continue;

        unsigned int cpylength = (dh[di]->maxx - dh[di]->minx + 1) * dh[di]->dpfh->bpp;
        unsigned char *ps = dh[di]->LCD + (dh[di]->miny * dh[di]->dpfh->width + dh[di]->minx) * dh[di]->dpfh->bpp;
        unsigned char *pd = tempLCD;
        for (int y = dh[di]->miny; y <= dh[di]->maxy; y++)
        {
            memcpy(pd, ps, cpylength);
            ps += dh[di]->dpfh->width * dh[di]->dpfh->bpp;
            pd += cpylength;
        }
            
        rect[0] = dh[di]->minx; rect[1] = dh[di]->miny; rect[2] = dh[di]->maxx + 1; rect[3] = dh[di]->maxy + 1;
        pthread_mutex_lock(&libax_mutex);
        int err = dpf_screen_blit(dh[di]->dpfh, tempLCD, rect);
        pthread_mutex_unlock(&libax_mutex);
        if (err < 0)
        {
            //fprintf(stderr, "Display %d detached (err=%d).\n", di, err);
            syslog(LOG_INFO, "%s: display %d communication error (%d). Display detached\n", config->name.c_str(), di, err);
            DeInitSingleDisplay(di);
            RescanUSB();
            lastscan = time(NULL);
        }
    }
    
    ResetMinMax();
    //fprintf(stderr, "\n");
}

uint32_t cDriverAX206DPF::GetBackgroundColor(void)
{
    return GRAPHLCD_Black;
}

void cDriverAX206DPF::SetSingleDisplayBrightness(unsigned int di, unsigned int percent)
{
    if (!dh[di]->attached)
        return;
        
    LIBDPF::DPFValue val;
    val.type = LIBDPF::TYPE_INTEGER;
    
    // Brightness can be 0 .. 7
    if (percent == 0)
        val.value.integer = 0;
    else if (percent >= 100)
        val.value.integer = 7;
    else    
        val.value.integer = (((percent * 10) + 167) * 6) / 1000;
    pthread_mutex_lock(&libax_mutex);
    dpf_setproperty(dh[di]->dpfh, PROPERTY_BRIGHTNESS, &val);
    pthread_mutex_unlock(&libax_mutex);
}

void cDriverAX206DPF::SetBrightness(unsigned int percent)
{
    lastbrightness = percent;
    
    for (unsigned int i = 0; i < numdisplays; i++)
    {
        SetSingleDisplayBrightness(i, percent);
    }
}

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
		//fprintf(stderr, "Got LCD dimensions: %dx%d\n", dpf->width, dpf->height);
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
				//fprintf(stderr, "Found %s at %s:%s\n", dev->desc, d->bus->dirname, d->filename);
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

#ifdef HAVE_DEBUG
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

unsigned char g_buf[] = {
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

	ret = usb_bulk_write(dev, ENDPT_OUT, (char*)g_buf, sizeof(g_buf), 1000);
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
