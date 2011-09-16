/*
 * GraphLCD driver library
 *
 * futatbaMDM166A.c  -  Futaba MDM166A LCD
 *                      Output goes to a Futaba MDM166A LCD
 *
 * This file is released under the GNU General Public License. 
 *
 * See the files README and COPYING for details.
 *
 * (c) 2010      Andreas Brachold <vdr07 AT deltab de>
 * (c) 2011      Wolfgang Astleitner <mrwastl AT users.sourceforge.net>
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
#include "futabaMDM166A.h"

static const byte STATE_OFF       = 0x00; //Symbol off
static const byte STATE_ON        = 0x01; //Symbol on
static const byte STATE_ONHIGH    = 0x02; //Symbol on, high intensity, can only be used with the volume symbols

static const byte CMD_PREFIX      = 0x1b;
static const byte CMD_SETCLOCK    = 0x00; //Actualize the time of the display
static const byte CMD_SMALLCLOCK  = 0x01; //Display small clock on display
static const byte CMD_BIGCLOCK    = 0x02; //Display big clock on display
static const byte CMD_SETSYMBOL   = 0x30; //Enable or disable symbol
static const byte CMD_SETDIMM     = 0x40; //Set the dimming level of the display
static const byte CMD_RESET       = 0x50; //Reset all configuration data to default and clear
static const byte CMD_SETRAM      = 0x60; //Set the actual graphics RAM offset for next data write
static const byte CMD_SETPIXEL    = 0x70; //Write pixel data to RAM of the display
static const byte CMD_TEST1       = 0xf0; //Show vertical test pattern
static const byte CMD_TEST2       = 0xf1; //Show horizontal test pattern

static const byte TIME_12         = 0x00; //12 hours format
static const byte TIME_24         = 0x01; //24 hours format

static const byte BRIGHT_OFF      = 0x00; //Display off
static const byte BRIGHT_DIMM     = 0x01; //Display dimmed
static const byte BRIGHT_FULL     = 0x02; //Display full brightness

namespace GLCD {

cHIDQueue::cHIDQueue() {
	hid = NULL;
  	bInit = false;
}

cHIDQueue::~cHIDQueue() {
  cHIDQueue::close();
}

bool cHIDQueue::open()
{
  HIDInterfaceMatcher matcher = { 0x19c2, 0x6a11, NULL, NULL, 0 };
  hid_return ret;

  /* see include/debug.h for possible values */
  hid_set_debug(HID_DEBUG_NONE);
  hid_set_debug_stream(0);
  /* passed directly to libusb */
  hid_set_usb_debug(0);
  
  ret = hid_init();
  if (ret != HID_RET_SUCCESS) {
    syslog(LOG_ERR, "libhid: init - %s (%d)", hiderror(ret), ret);
    return false;
  }
  bInit = true;

  hid = hid_new_HIDInterface();
  if (hid == 0) {
    syslog(LOG_ERR, "libhid: hid_new_HIDInterface() failed, out of memory?\n");
    return false;
  }

  ret = hid_force_open(hid, 0, &matcher, 3);
  if (ret != HID_RET_SUCCESS) {
    syslog(LOG_ERR, "libhid: open - %s (%d)", hiderror(ret), ret);
    hid_close(hid);
    hid_delete_HIDInterface(&hid);
    hid = 0;
    return false;
  }

  while (!empty()) {
    pop();
  }
  //ret = hid_write_identification(stdout, hid);
  //if (ret != HID_RET_SUCCESS) {
  //  syslog(LOG_INFO, "libhid: write_identification %s (%d)", hiderror(ret), ret);
  //  return false;
  //}
  return true;
}

void cHIDQueue::close() {
  hid_return ret;
  if (hid != 0) {
    ret = hid_close(hid);
    if (ret != HID_RET_SUCCESS) {
      syslog(LOG_ERR, "libhid: close - %s (%d)", hiderror(ret), ret);
    }

    hid_delete_HIDInterface(&hid);
    hid = 0;
  }
  if(bInit) {
    ret = hid_cleanup();
    if (ret != HID_RET_SUCCESS) {
      syslog(LOG_ERR, "libhid: cleanup - %s (%d)", hiderror(ret), ret);
    }
    bInit = false;
  }
}

void cHIDQueue::Cmd(const byte & cmd) {
  this->push(CMD_PREFIX);
  this->push(cmd);
}

void cHIDQueue::Data(const byte & data) {
  this->push(data);
}

bool cHIDQueue::Flush() {

  if(empty())
    return true;
  if(!isopen()) {
    return false;
  }

  int const PATH_OUT[1] = { 0xff7f0004 };
  char buf[64];
  hid_return ret;

  while (!empty()) {
    buf[0] = (char) std::min((size_t)63,size());
    for(unsigned int i = 0;i < 63 && !empty();++i) {
      buf[i+1] = (char) front(); //the first element in the queue
      pop();              //remove the first element of the queue
    }
    ret = hid_set_output_report(hid, PATH_OUT, sizeof(PATH_OUT), buf, (buf[0] + 1));
    if (ret != HID_RET_SUCCESS) {
      syslog(LOG_ERR, "libhid: set_output_report - %s (%d)", hiderror(ret), ret);
      while (!empty()) {
        pop();
      }
      cHIDQueue::close();
      return false;
    }
  }
  return true;
}

const char *cHIDQueue::hiderror(hid_return ret) const
{
  switch(ret) {
    case HID_RET_SUCCESS:
      return "success";
    case HID_RET_INVALID_PARAMETER:
      return "invalid parameter";
    case HID_RET_NOT_INITIALISED:
      return "not initialized";
    case HID_RET_ALREADY_INITIALISED:
      return "hid_init() already called";
    case HID_RET_FAIL_FIND_BUSSES:
      return "failed to find any USB busses";
    case HID_RET_FAIL_FIND_DEVICES:
      return "failed to find any USB devices";
    case HID_RET_FAIL_OPEN_DEVICE:
      return "failed to open device";
    case HID_RET_DEVICE_NOT_FOUND:
      return "device not found";
    case HID_RET_DEVICE_NOT_OPENED:
      return "device not yet opened";
    case HID_RET_DEVICE_ALREADY_OPENED:
      return "device already opened";
    case HID_RET_FAIL_CLOSE_DEVICE:
      return "could not close device";
    case HID_RET_FAIL_CLAIM_IFACE:
      return "failed to claim interface; is another driver using it?";
    case HID_RET_FAIL_DETACH_DRIVER:
      return "failed to detach kernel driver";
    case HID_RET_NOT_HID_DEVICE:
      return "not recognized as a HID device";
    case HID_RET_HID_DESC_SHORT:
      return "HID interface descriptor too short";
    case HID_RET_REPORT_DESC_SHORT:
      return "HID report descriptor too short";
    case HID_RET_REPORT_DESC_LONG:
      return "HID report descriptor too long";
    case HID_RET_FAIL_ALLOC:
      return "failed to allocate memory";
    case HID_RET_OUT_OF_SPACE:
      return "no space left in buffer";
    case HID_RET_FAIL_SET_REPORT:
      return "failed to set report";
    case HID_RET_FAIL_GET_REPORT:
      return "failed to get report";
    case HID_RET_FAIL_INT_READ:
      return "interrupt read failed";
    case HID_RET_NOT_FOUND:
      return "not found";
#ifdef HID_RET_TIMEOUT
    case HID_RET_TIMEOUT:
      return "timeout";
#endif
    default:
      return "unknown error or timeout";
  }
  return "unknown error";
}


cDriverFutabaMDM166A::cDriverFutabaMDM166A(cDriverConfig * config)
: cDriver(config)
, m_pDrawMem(0)
, m_pVFDMem(0)
{
    m_nRefreshCounter = 0;
	lastIconState = 0;
}

int cDriverFutabaMDM166A::Init()
{
    // default values
    width = config->width;
    if (width <= 0 || width > 96)
        width = 96;
    height = config->height;
    if (height <= 0 || height > 16)
        height = 16;
    m_iSizeYb = ((height + 7) / 8);


    /*for (unsigned int i = 0; i < config->options.size(); i++)
    {
        if (config->options[i].name == "")
        {
        }
    }*/

    *oldConfig = *config;

    // setup the memory array for the drawing array
    m_pDrawMem = new unsigned char[width * m_iSizeYb];
    // setup the memory array for the display array
    m_pVFDMem = new unsigned char[width * m_iSizeYb];
	if(!m_pDrawMem || !m_pVFDMem) {
	    syslog(LOG_ERR, "FutabaMDM166A: malloc frame buffer failed, out of memory?\n");
		return -1;
	}

    // clear display
    ClearVFDMem();
    Clear();


	if(!cHIDQueue::open()) {
		return -1;
	}
	lastIconState = 0;

	cHIDQueue::Cmd(CMD_RESET);
    // Set Display SetBrightness
    SetBrightness(config->brightness ? config->brightness : 50);

	if(cHIDQueue::Flush()) {
	    syslog(LOG_INFO, "%s: initialized.\n", config->name.c_str());
		return 0;
	}
	return -1;
}

int cDriverFutabaMDM166A::DeInit()
{
	cHIDQueue::close();

    if (m_pVFDMem)
        delete[] m_pVFDMem;
    if (m_pDrawMem)
        delete[] m_pDrawMem;

    return 0;
}

int cDriverFutabaMDM166A::CheckSetup()
{
    if (config->width != oldConfig->width ||
        config->height != oldConfig->height)
    {
        DeInit();
        return Init();
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

void cDriverFutabaMDM166A::ClearVFDMem()
{
    for (unsigned int n = 0; m_pVFDMem && n < (width * m_iSizeYb); n++)
        m_pVFDMem[n] = 0x00;
}

void cDriverFutabaMDM166A::Clear()
{
    for (unsigned int n = 0; m_pDrawMem && n < (width * m_iSizeYb); n++)
        m_pDrawMem[n] = 0x00;
}

void cDriverFutabaMDM166A::SetPixel(int x, int y, uint32_t data)
{
    byte c;
    int n;

    if (!m_pDrawMem)
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

    n = x + ((y / 8) * width);
    c = 0x80 >> (y % 8);

    //m_pDrawMem[n] |= c;
    if (data == GRAPHLCD_White)
        m_pDrawMem[n] |= c;
    else
        m_pDrawMem[n] &= (0xFF ^ c);
}

#if 0
void cDriverFutabaMDM166A::Set8Pixels(int x, int y, byte data)
{
    int n;

    // x - pos is'nt mayby align to 8
    x &= 0xFFF8;

    for (n = 0; n < 8; ++n)
    {
        if (data & (0x80 >> n))      // if bit is set
            SetPixel(x + n, y);
    }
}
#endif

void cDriverFutabaMDM166A::Refresh(bool refreshAll)
{
    unsigned int n, x, yb;

    if (!m_pVFDMem || !m_pDrawMem)
        return;

    bool doRefresh = false;
    unsigned int nWidth = width;
    unsigned int minX = nWidth;
    unsigned int maxX = 0;

	int s = CheckSetup();
	if(s < 0)
		return;
    if (s > 0)
        refreshAll = true;

    for (yb = 0; yb < m_iSizeYb; ++yb)
        for (x = 0; x < nWidth; ++x)
        {
            n = x + (yb * nWidth);
            if (m_pVFDMem[n] != m_pDrawMem[n])
            {
                m_pVFDMem[n] = m_pDrawMem[n];
                minX = std::min(minX, x);
                maxX = std::max(maxX, x + 1);
                doRefresh = true;
            }
        }

    m_nRefreshCounter = (m_nRefreshCounter + 1) % (config->refreshDisplay ? config->refreshDisplay : 50);

    if (!refreshAll && !m_nRefreshCounter)
        refreshAll = true;

    if (refreshAll || doRefresh)
    {
        if (refreshAll) {
            minX = 0;
            maxX = nWidth;
            // and reset RefreshCounter
            m_nRefreshCounter = 0;
         }

        maxX = std::min(maxX, nWidth);

		unsigned int nData = (maxX-minX) * m_iSizeYb;
		if(nData) {
		    // send data to display, controller
			cHIDQueue::Cmd(CMD_SETRAM);
			cHIDQueue::Data(minX*m_iSizeYb);
			cHIDQueue::Cmd(CMD_SETPIXEL);
			cHIDQueue::Data(nData);

		    for (x = minX; x < maxX; ++x)
			    for (yb = 0; yb < m_iSizeYb; ++yb)
		        {
		            n = x + (yb * nWidth);
		            cHIDQueue::Data((m_pVFDMem[n]) ^ (config->invert ? 0xff : 0x00));
		        }
		}
    }
	cHIDQueue::Flush();
}

/**
 * Sets the brightness of the display.
 *
 * \param nBrightness The value the brightness (less 33% = off
 *                  more then 66% = highest brightness,else half brightness ).
 */
void cDriverFutabaMDM166A::SetBrightness(unsigned int percent)
{
	byte nBrightness = 1;
	if (percent < 33) {
		nBrightness = 0;
	} else if (nBrightness > 66) {
		nBrightness = 2;
	}
	cHIDQueue::Cmd(CMD_SETDIMM);
	cHIDQueue::Data(nBrightness);
}

}

	


