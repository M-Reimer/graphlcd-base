/*
 * GraphLCD driver library
 *
 * PicoLCD_256x64.c  -  picoLCD Graphic 256x64
 *                      Output goes to a picoLCD Graphic 256x64 LCD
 *
 *                      Driver is based on lcd4linux driver by Nicu Pavel, Mini-Box.com <npavel@mini-box.com>
 *
 * This file is released under the GNU General Public License. 
 *
 * See the files README and COPYING for details.
 *
 * 2012 by       Jochen Koch <linuxfan1992 AT web de>
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <stdint.h>


#include <stdio.h>
#include <syslog.h>
#include <cstring>

#include "common.h"
#include "config.h"
#include "picoLCD_256x64.h"

namespace GLCD {

cDriverPicoLCD_256x64::cDriverPicoLCD_256x64(cDriverConfig * config)
: cDriver(config)
, pLG_framebuffer(0)
{
    dirty = 1;
    inverted = 0;
    gpo = 0;
    read_timeout = 0;
}

int cDriverPicoLCD_256x64::Init()
{
    DEBUG("picoLCD Graphic initialization");

    // default values
    width = config->width;
    if (width <= 0 || width > SCREEN_W)
        width = SCREEN_W;
    height = config->height;
    if (height <= 0 || height > SCREEN_H)
        height = SCREEN_H;

    *oldConfig = *config;

    if (drv_pLG_open() < 0) {
	return -1;
    }

    /* Init framebuffer buffer */
    pLG_framebuffer = (unsigned char *)malloc(SCREEN_W * SCREEN_H / 8 * sizeof(unsigned char));
    if (!pLG_framebuffer)
    {
        INFO("picoLCD_256x64: frame buffer could not be allocated: malloc() failed");
	return -1;
    }

    /* clear display */
    Clear();
    drv_pLG_clear();
    DEBUG("zeroed");

    // inverted display
    inverted = (config->invert ? 0xff : 0);
    // Set Display SetBrightness
    SetBacklight(config->backlight);
    // Set Display SetContrast
    SetContrast(config->contrast);

    return 0;
}


int cDriverPicoLCD_256x64::DeInit()
{

    DEBUG("picoLCD_256x64: shutting down.");

    /* clear display */
    Clear();
    drv_pLG_clear();
    drv_pLG_close();

    if (pLG_framebuffer) {
	free(pLG_framebuffer);
	pLG_framebuffer = NULL;
    }
    return 0;
}


int cDriverPicoLCD_256x64::CheckSetup()
{
    if (config->width != oldConfig->width ||
        config->height != oldConfig->height)
    {
        DeInit();
        return Init();
    }

    if (config->backlight != oldConfig->backlight)
    {
        oldConfig->backlight = config->backlight;
        SetBacklight(config->backlight);
    }

    if (config->contrast != oldConfig->contrast)
    {
        oldConfig->contrast = config->contrast;
        SetContrast(config->contrast);
    }

    if (config->upsideDown != oldConfig->upsideDown ||
        config->invert != oldConfig->invert)
    {
        oldConfig->upsideDown = config->upsideDown;
        oldConfig->invert = config->invert;
        inverted = (config->invert ? 0xff : 0);
        return 1;
    }
    return 0;
}


void cDriverPicoLCD_256x64::Clear()
{
    for (unsigned int n = 0; pLG_framebuffer && n < (SCREEN_W * SCREEN_H / 8); n++)
        pLG_framebuffer[n] = 0x00;
}


void cDriverPicoLCD_256x64::SetPixel(int x, int y, uint32_t data)
{
    unsigned char c;
    int n;

    if (!pLG_framebuffer)
        return;

    if (x >= width || x < 0)
        return;
    if (y >= height || y < 0)
        return;

    if (config->upsideDown)
    {
        x = width - 1 - x;
        y = height - 1 - y;
    }

    n = x + ((y / 8) * SCREEN_W);
    c = 0x01 << (y % 8);

    if (data == GRAPHLCD_White)
        pLG_framebuffer[n] |= c;
    else
        pLG_framebuffer[n] &= (0xFF ^ c);

    /* display needs to be redrawn from frame buffer */
    dirty = 1;
}


void cDriverPicoLCD_256x64::Refresh(bool refreshAll)
{
    unsigned char cmd3[64] = { OUT_REPORT_CMD_DATA };	/* send command + data */
    unsigned char cmd4[64] = { OUT_REPORT_DATA };	/* send data only */

    int index, x, s;
    unsigned char cs, line;

    if (!pLG_framebuffer)
        return;

    s = CheckSetup();
    if ((s > 0) || dirty)
        refreshAll = true;

    /* do not redraw display if frame buffer has not changed */
    if (!refreshAll) {
	DEBUG("Skipping");
	return;
    }
    DEBUG("entered");

    for (cs = 0; cs < 4; cs++)
    {
        unsigned char chipsel = (cs << 2);	//chipselect
        for (line = 0; line < 8; line++)
        {
	    //ha64_1.setHIDPkt(OUT_REPORT_CMD_DATA, 8+3+32, 8, chipsel, 0x02, 0x00, 0x00, 0xb8|j, 0x00, 0x00, 0x40);
	    cmd3[0] = OUT_REPORT_CMD_DATA;
            cmd3[1] = chipsel;
            cmd3[2] = 0x02;
            cmd3[3] = 0x00;
            cmd3[4] = 0x00;
            cmd3[5] = 0xb8 | line;
            cmd3[6] = 0x00;
            cmd3[7] = 0x00;
            cmd3[8] = 0x40;
            cmd3[9] = 0x00;
            cmd3[10] = 0x00;
            cmd3[11] = 32;

            //ha64_2.setHIDPkt(OUT_REPORT_DATA, 4+32, 4, chipsel | 0x01, 0x00, 0x00, 32);
            cmd4[0] = OUT_REPORT_DATA;
            cmd4[1] = chipsel | 0x01;
            cmd4[2] = 0x00;
            cmd4[3] = 0x00;
            cmd4[4] = 32;

            for (index = 0; index < 32; index++)
            {
                x=64*cs+index;
                cmd3[12 + index] = pLG_framebuffer[line * SCREEN_W + x] ^ inverted;
            }
            for (index = 32; index < 64; index++)
            {
                x=64*cs+index;
                cmd4[5 + (index - 32)] = pLG_framebuffer[line * SCREEN_W + x] ^ inverted;
            }
            drv_pLG_send(cmd3, 44);
            drv_pLG_send(cmd4, 38);
        }
    }
    /* mark display as up-to-date */
    dirty = 0;
}


/*
 * Sets the backlight brightness of the display.
 *
 */
void cDriverPicoLCD_256x64::SetBacklight(unsigned int onoff)
{
    unsigned char cmd[2] = { 0x91 };	/* set backlight */

    cmd[1] = (onoff>0 ? 0xff : 0);
    drv_pLG_send(cmd, 2);
}


void cDriverPicoLCD_256x64::SetContrast(unsigned int val)
{
    unsigned char cmd[2] = { 0x92 };	/* set contrast */

    if (val > 10)
	val = 10;

    cmd[1] = 0x99+val*(0xff-0x99)/10;
    drv_pLG_send(cmd, 2);
}

/****************************************/
/***  hardware dependant functions    ***/
/****************************************/

int cDriverPicoLCD_256x64::drv_pLG_open(void)
{
    struct usb_bus *busses, *bus;
    struct usb_device *dev;
    char driver[1024];
    char product[1024];
    char manufacturer[1024];
    char serialnumber[1024];
    int ret;

    lcd = NULL;

    INFO("scanning for picoLCD 256x64...");

    usb_init();
    usb_find_busses();
    usb_find_devices();
    busses = usb_get_busses();

    for (bus = busses; bus; bus = bus->next) {
	for (dev = bus->devices; dev; dev = dev->next) {
	    if ((dev->descriptor.idVendor == picoLCD_VENDOR) && (dev->descriptor.idProduct == picoLCD_DEVICE)) {

		DEBUG1("found picoLCD on bus %s device %s", bus->dirname, dev->filename);

		lcd = usb_open(dev);

		ret = usb_get_driver_np(lcd, 0, driver, sizeof(driver));

		if (ret == 0) {
		    DEBUG1("interface 0 already claimed by '%s'", driver);
		    DEBUG(" attempting to detach driver...");
		    if (usb_detach_kernel_driver_np(lcd, 0) < 0) {
			DEBUG("usb_detach_kernel_driver_np() failed!");
			return -1;
		    }
		}

		usb_set_configuration(lcd, 1);
		usleep(100);

		if (usb_claim_interface(lcd, 0) < 0) {
		    DEBUG("picoLCD_256x64: usb_claim_interface() failed!");
		    return -1;
		}

		usb_set_altinterface(lcd, 0);

		usb_get_string_simple(lcd, dev->descriptor.iProduct, product, sizeof(product));
		usb_get_string_simple(lcd, dev->descriptor.iManufacturer, manufacturer, sizeof(manufacturer));
		usb_get_string_simple(lcd, dev->descriptor.iSerialNumber, serialnumber, sizeof(serialnumber));

		INFO1("Manufacturer='%s' Product='%s' SerialNumber='%s\n'", manufacturer, product, serialnumber);

		return 0;
	    }
	}
    }
    INFO("could not find a picoLCD");
    return -1;
}


int cDriverPicoLCD_256x64::drv_pLG_read(unsigned char *data, int size)
{
    return usb_interrupt_read(lcd, USB_ENDPOINT_IN + 1, (char *) data, size, read_timeout);
}


void cDriverPicoLCD_256x64::drv_pLG_send(unsigned char *data, int size)
{
    int __attribute__ ((unused)) ret;
    ret = usb_interrupt_write(lcd, USB_ENDPOINT_OUT + 1, (char *) data, size, 1000);
}


int cDriverPicoLCD_256x64::drv_pLG_close(void)
{
    usb_release_interface(lcd, 0);
    usb_close(lcd);

    return 0;
}


void cDriverPicoLCD_256x64::drv_pLG_clear(void)
{
    unsigned char cmd[3] = { 0x93, 0x01, 0x00 };	/* init display */
    unsigned char cmd2[9] = { OUT_REPORT_CMD };	/* init display */
    unsigned char cmd3[64] = { OUT_REPORT_CMD_DATA };	/* clear screen */
    unsigned char cmd4[64] = { OUT_REPORT_CMD_DATA };	/* clear screen */

    int init, index;
    unsigned char cs, line;


    DEBUG("entering\n");
    drv_pLG_send(cmd, 3);

    for (init = 0; init < 4; init++) {
	unsigned char cs = ((init << 2) & 0xFF);

	cmd2[0] = OUT_REPORT_CMD;
	cmd2[1] = cs;
	cmd2[2] = 0x02;
	cmd2[3] = 0x00;
	cmd2[4] = 0x64;
	cmd2[5] = 0x3F;
	cmd2[6] = 0x00;
	cmd2[7] = 0x64;
	cmd2[8] = 0xC0;

	drv_pLG_send(cmd2, 9);
    }

    for (cs = 0; cs < 4; cs++) {
	unsigned char chipsel = (cs << 2);	//chipselect
	for (line = 0; line < 8; line++) {
	    //ha64_1.setHIDPkt(OUT_REPORT_CMD_DATA, 8+3+32, 8, cs, 0x02, 0x00, 0x00, 0xb8|j, 0x00, 0x00, 0x40);
	    cmd3[0] = OUT_REPORT_CMD_DATA;
	    cmd3[1] = chipsel;
	    cmd3[2] = 0x02;
	    cmd3[3] = 0x00;
	    cmd3[4] = 0x00;
	    cmd3[5] = 0xb8 | line;
	    cmd3[6] = 0x00;
	    cmd3[7] = 0x00;
	    cmd3[8] = 0x40;
	    cmd3[9] = 0x00;
	    cmd3[10] = 0x00;
	    cmd3[11] = 32;

	    unsigned char temp = 0;

	    for (index = 0; index < 32; index++) {
		cmd3[12 + index] = temp;
	    }

	    drv_pLG_send(cmd3, 64);

	    //ha64_2.setHIDPkt(OUT_REPORT_DATA, 4+32, 4, cs | 0x01, 0x00, 0x00, 32);

	    cmd4[0] = OUT_REPORT_DATA;
	    cmd4[1] = chipsel | 0x01;
	    cmd4[2] = 0x00;
	    cmd4[3] = 0x00;
	    cmd4[4] = 32;

	    for (index = 32; index < 64; index++) {
		temp = 0x00;
		cmd4[5 + (index - 32)] = temp;
	    }
	    drv_pLG_send(cmd4, 64);
	}
    }
}

#ifdef ENABLE_GPIO_KEYPAD
/**************************************\
*                                      *
* Routines for GPI, GPO & Keypad       *
*                                      *
\**************************************/

#define _USBLCD_MAX_DATA_LEN          24
#define IN_REPORT_KEY_STATE           0x11

typedef enum {
    WIDGET_KEY_UP = 1,
    WIDGET_KEY_DOWN = 2,
    WIDGET_KEY_LEFT = 4,
    WIDGET_KEY_RIGHT = 8,
    WIDGET_KEY_CONFIRM = 16,
    WIDGET_KEY_CANCEL = 32,
    WIDGET_KEY_PRESSED = 64,
    WIDGET_KEY_RELEASED = 128
} KEYPADKEY;

int cDriverPicoLCD_256x64::drv_pLG_gpi( __attribute__ ((unused))
		       int num)
{
    int ret;
    unsigned char read_packet[_USBLCD_MAX_DATA_LEN];
    ret = drv_pLG_read(read_packet, _USBLCD_MAX_DATA_LEN);
    if ((ret > 0) && (read_packet[0] == IN_REPORT_KEY_STATE)) {
//	DEBUG("picoLCD: pressed key= 0x%02x\n", read_packet[1]);
	return read_packet[1];
    }
    return 0;
}


int cDriverPicoLCD_256x64::drv_pLG_gpo(int num, int val)
{
    unsigned char cmd[2] = { 0x81 };	/* set GPO */

    if (num < 0)
	num = 0;
    if (num > 7)
	num = 7;

    if (val < 0)
	val = 0;
    if (val > 1)
	val = 1;

    /* set led bit to 1 or 0 */
    if (val)
	gpo |= 1 << num;
    else
	gpo &= ~(1 << num);

    cmd[1] = gpo;
    drv_pLG_send(cmd, 2);

    return val;
}


void cDriverPicoLCD_256x64::drv_pLG_update_keypad(void)
{
    static int pressed_key = 0;

    int ret;
    unsigned char read_packet[_USBLCD_MAX_DATA_LEN];
    ret = drv_pLG_read(read_packet, _USBLCD_MAX_DATA_LEN);
    if ((ret > 0) && (read_packet[0] == IN_REPORT_KEY_STATE)) {
//	DEBUG("picoLCD: pressed key= 0x%02x\n", read_packet[1]);
	int new_pressed_key = read_packet[1];
	if (pressed_key != new_pressed_key) {
	    /* negative values mark a key release */
//	    drv_generic_keypad_press(-pressed_key);
//	    drv_generic_keypad_press(new_pressed_key);
	    pressed_key = new_pressed_key;
	}
    }
}


int cDriverPicoLCD_256x64::drv_pLG_keypad(const int num)
{
    int val;
    int new_num = num;

    if (new_num == 0)
	return 0;
    else if (new_num > 0)
	val = WIDGET_KEY_PRESSED;
    else {
	/* negative values mark a key release */
	new_num = -num;
	val = WIDGET_KEY_RELEASED;
    }

    switch (new_num) {
    case 1:
	val += WIDGET_KEY_CANCEL;
	break;
    case 2:
	val += WIDGET_KEY_LEFT;
	break;
    case 3:
	val += WIDGET_KEY_RIGHT;
	break;
    case 5:
	val += WIDGET_KEY_UP;
	break;
    case 6:
	val += WIDGET_KEY_CONFIRM;
	break;
    case 7:
	val += WIDGET_KEY_DOWN;
	break;
    default:
	fprintf(stderr,"picoLCD_256x64: unknown keypad value %d", num);
    }

    return val;
}
#endif

// /namespace
}

