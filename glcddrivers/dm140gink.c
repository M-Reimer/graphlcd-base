/*
 * GraphLCD driver library
 *
 * framebuffer.h  -  framebuffer device
 *                   Output goes to a framebuffer device
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2004      Stephan Skrodzki
 * (c) 2011      Wolfgang Astleitner <mrwastl AT users.sourceforge.net>
 */

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <asm/types.h>
#include <linux/hiddev.h>


#include "common.h"
#include "config.h"
#include "dm140gink.h"


namespace GLCD
{

cDriverDM140GINK::cDriverDM140GINK(cDriverConfig * config)
:   cDriver(config),
    fd(-1),
    framebuff(0)
{
}

/* hack - fix improper signed char handling - it's seeing 0x80 as a negative value*/
#define VALUE_FILTER(_value)  (_value>0x7F)?(__s32)(0xFFFFFF00 | _value):(_value)

//**************************************************************
// FUNCTION: SendReport
//
// INPUT: 
//	int fd - file descriptor to the opened HID device
//	const char *buf - Message to write
//	size_t size - size of buf.
//
// OUTPUT:
//  int err - result of the ioctl call (On success 0, On error -1)
//
// DESCRIPTION: This function will write the 'buf' to the opened
//	HID device.  Specifically, it updates the device's usage 
//	reference with the data and then sends a report to the HID.
//**************************************************************
int cDriverDM140GINK::SendReport(const char *cbuf, size_t size)
{
    const unsigned char *buf=reinterpret_cast<const unsigned char *>(cbuf);
    struct hiddev_report_info rinfo;
    struct hiddev_usage_ref uref;
    int err;
    
    //******************************************************
    // Initialize the usage Reference and mark it for OUTPUT
    //******************************************************
    memset(&uref, 0, sizeof(uref));
    uref.report_type = HID_REPORT_TYPE_OUTPUT;
    uref.report_id = 0;
    uref.field_index = 0;

    //**************************************************************
    // Fill in the information that we wish to set
    //**************************************************************
    uref.usage_code  = 0xffa10005; //unused?
    for(size_t i=0;i<size;i++)
    {
	uref.usage_index = i;
	uref.value = VALUE_FILTER(buf[i]);

	//**************************************************************
	// HIDIOCSUSAGE - struct hiddev_usage_ref (write)
	// Sets the value of a usage in an output report.  The user fills
	// in the hiddev_usage_ref structure as above, but additionally 
	// fills in the value field.
	//**************************************************************
	if((err	= ioctl(fd, HIDIOCSUSAGE, &uref)) < 0)
	{
		syslog(LOG_INFO, "%s: Error with sending the USAGE ioctl %d,0x%02X;size:%d\n", config->name.c_str(), (int)i, (int)buf[i], (int)size);
		return err;
	}
	uref.usage_code = 0xffa10006; //unused?
    }

    //**************************************************************
    // HIDIOCSREPORT - struct hiddev_report_info (write)
    // Instructs the kernel to SEND a report to the device.  This 
    // report can be filled in by the user through HIDIOCSUSAGE calls 
    // (below) to fill in individual usage values in the report before
    // sending the report in full to the device.
    //**************************************************************
    memset(&rinfo, 0, sizeof(rinfo));
    rinfo.report_type = HID_REPORT_TYPE_OUTPUT;
    rinfo.report_id = 0;
    rinfo.num_fields = 0;
    if((err = ioctl(fd, HIDIOCSREPORT, &rinfo)) < 0)
    {
	syslog(LOG_INFO, "%s: Error with sending the REPORT ioctl %d\n", config->name.c_str(), err);
    }

    //******************************************************
    // All done, let's return what we did.
    //******************************************************
    return err;
}

int cDriverDM140GINK::Init()
{
    // default values
    width = config->width;
    if (width <= 0)
        width = 112;
    height = config->height;
    if (height <= 0)
        height = 16;

    signed short vendor = 0x040b;
    signed short product = 0x7001;

    for (unsigned int i = 0; i < config->options.size(); i++)
    {
        if (config->options[i].name == "Vendor")
            vendor=config->GetInt(config->options[i].value);
        if (config->options[i].name == "Product")
            product=config->GetInt(config->options[i].value);
    }

    //******************************************************
    // Loop through all the 16 HID ports/devices looking for
    // one that matches our device.
    //******************************************************
    const char *hiddev_prefix = "/dev/usb/hiddev"; /* in devfs */
    for(int i=0;i<16;i++)
    {
	char port[32];
	sprintf(port, "%s%d", hiddev_prefix, i);
	if((fd = open(port,O_WRONLY))>=0)
	{
	    struct hiddev_devinfo device_info;
    	    ioctl(fd, HIDIOCGDEVINFO, &device_info);
    	    

    	    // If we've found our device, no need to look further, time to stop searching
    	    if(vendor==device_info.vendor && product==device_info.product)
    	    {
		//char name[256];
		//ioctl(fd, HIDIOCGNAME(sizeof(name)), name);
		//name[sizeof(name)-1]='\0';
		//syslog(LOG_INFO, "%s: The device %s was opened successfully.\n", config->name.c_str(), name);

    		break; // stop the for loop
    	    }
    	    close(fd); // Added by HL
	    fd=-1;
	}
    }

    if (-1 == fd)
    {
	syslog(LOG_ERR, "%s: Cannot open device 0x%x:0x%x.\n", config->name.c_str(), vendor, product);
        return -1;
    }

    //******************************************************
    // Initialize the internal report structures
    //******************************************************
    if(ioctl(fd, HIDIOCINITREPORT,0)<0)
    {
        syslog(LOG_ERR, "%s: cannot init device.\n", config->name.c_str());
	return -1;
    }

    //******************************************************
    // Set up the display to show graphics
    //******************************************************
    const char panelCmd[]  = {0x01, 0x01, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00};
    const char iconCmd[]   = {0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};  //icon command
    const char iconoff[]   = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};  //icon data

    SendReport(panelCmd, sizeof(panelCmd));
    SendReport(iconCmd, sizeof(iconCmd));
    SendReport(iconoff, sizeof(iconoff));
    
    const char setup[] = {0x01, 0x05, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00};

    SendReport(setup, sizeof(setup));

    screensize = width * height / 8;

    // reserve another memory to draw into
    framebuff = new char[screensize];
    if (!framebuff)
    {
        syslog(LOG_ERR, "%s: failed to alloc memory for device.\n", config->name.c_str());
        return -1;
    }

    *oldConfig = *config;

    // clear display
    Clear();
    Refresh(true);

    syslog(LOG_INFO, "%s: display initialized.\n", config->name.c_str());
    return 0;
}

int cDriverDM140GINK::DeInit()
{
    if (framebuff)
        delete[] framebuff;
    framebuff=NULL;
    if (-1 != fd)
        close(fd);
    fd=-1;
    return 0;
}

int cDriverDM140GINK::CheckSetup()
{
    if (config->device != oldConfig->device ||
        config->port != oldConfig->port ||
        config->width != oldConfig->width ||
        config->height != oldConfig->height)
    {
        DeInit();
        Init();
        return 0;
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

void cDriverDM140GINK::SetPixel(int x, int y, uint32_t data)
{
    if (x >= width || y >= height)
        return;

    if (config->upsideDown)
    {
        x = width - 1 - x;
        y = height - 1 - y;
    }

    int offset = (y/8) * width + x;
    char mask = (1 << (7 - (y%8)));
    if (data == GRAPHLCD_White)
        framebuff[offset] |= mask;
    else
        framebuff[offset] &= (0xFF ^ mask);
}

void cDriverDM140GINK::Clear()
{
    memset(framebuff, 0, screensize);
}

#if 0
void cDriverDM140GINK::Set8Pixels(int x, int y, unsigned char data)
{
    x &= 0xFFF8;

    for (int n = 0; n < 8; ++n)
    {
        if (data & (0x80 >> n))      // if bit is set
            SetPixel(x + n, y, GRAPHLCD_White);
    }
}
#endif

void cDriverDM140GINK::Refresh(bool refreshAll)
{
    char packet[] = {0x1D, 0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00};

    SendReport(packet, sizeof(packet));

    // Send the actual graphics
    for(int i=0; i< screensize;i+=8)
    {
	// make sure we only send 8 bytes at a time
	int size = (screensize - i);
	size = (size > 8) ? 8 : size;
	SendReport(framebuff+i, size);
    }

    const char show[] = {0x01, 0x05, 0x03, 0x03,  0x00, 0x00, 0x00, 0x00};

    SendReport(show, sizeof(show));
}

} // end of namespace
