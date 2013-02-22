/*
 * GraphLCD driver library
 *
 * PicoLCD_256x64.h  -  picoLCD Graphic 256x64
 *                      Output goes to a picoLCD Graphic 256x64 LCD
 *
 *                      Driver is based on lcd4linux driver by Nicu Pavel, Mini-Box.com <npavel@mini-box.com>
 *
 * This file is released under the GNU General Public License. 
 *
 * See the files README and COPYING for details.
 *
 *  2012 by      Jochen Koch <linuxfan1992 AT web de>
 */

#ifndef _GLCDDRIVERS_PicoLCD_256x64_H_
#define _GLCDDRIVERS_PicoLCD_256x64_H_

#include "driver.h"
#include "stdio.h"
#include <usb.h>
#include <syslog.h>

#define HAVE_STDBOOL_H

#define picoLCD_VENDOR  0x04d8
#define picoLCD_DEVICE  0xc002

#define OUT_REPORT_LED_STATE		0x81
#define OUT_REPORT_LCD_BACKLIGHT	0x91
#define OUT_REPORT_LCD_CONTRAST		0x92

#define OUT_REPORT_CMD			0x94
#define OUT_REPORT_DATA			0x95
#define OUT_REPORT_CMD_DATA		0x96

#define SCREEN_H			64
#define SCREEN_W			256

#ifdef HAVE_DEBUG
#define DEBUG(x) fprintf(stderr,"picoLCD_256x64: %s(): " #x "\n", __FUNCTION__);
#define DEBUG1(x,...) fprintf(stderr,"picoLCD_256x64: %s(): " #x "\n", __FUNCTION__, __VA_ARGS__);
#else
#define DEBUG(x)
#define DEBUG1(x,...)
#endif

#define INFO(x) syslog(LOG_INFO, "picoLCD_256x64: %s\n", x);
#define INFO1(x,...) syslog(LOG_INFO, "picoLCD_256x64: " #x "\n", __VA_ARGS__);

namespace GLCD
{
	class cDriverConfig;

	class cDriverPicoLCD_256x64 : public cDriver 
	{
                /* "dirty" marks the display to be redrawn from frame buffer */
                int dirty;

                /* USB read timeout in ms (the picoLCD 256x64 times out on every read
                   unless a key has been pressed!)  */
                int read_timeout;
 
                unsigned char *pLG_framebuffer;

                /* used to display white text on black background or inverse */
                unsigned char inverted;

                unsigned int gpo;

                usb_dev_handle *lcd;

		int CheckSetup();
	protected:
                int drv_pLG_open(void);
                int drv_pLG_read(unsigned char *data, int size);
                void drv_pLG_send(unsigned char *data, int size);
                int drv_pLG_close(void);
                void drv_pLG_clear(void);
#ifdef ENABLE_GPIO_KEYPAD
                // GPI, GPO, Keypad
                int drv_pLG_gpi(int num);
                int drv_pLG_gpo(int num, int val);
                void drv_pLG_update_keypad(void);
                int drv_pLG_keypad(const int num);
#endif

	public:
		cDriverPicoLCD_256x64(cDriverConfig * config);

		virtual int Init();
		virtual int DeInit();

		virtual void Clear();
                virtual void SetPixel(int x, int y, uint32_t data);
		//virtual void Set8Pixels(int x, int y, byte data);
		virtual void Refresh(bool refreshAll = false);

		virtual void SetBacklight(unsigned int percent);
		virtual void SetContrast(unsigned int percent);
	};
};
#endif



