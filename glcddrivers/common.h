/*
 * GraphLCD driver library
 *
 * common.c  -  various functions
 *
 * parts were taken from graphlcd plugin for the Video Disc Recorder
 * (c) 2001-2004 Carsten Siebholz <c.siebholz AT t-online.de>
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2004 Andreas Regel <andreas.regel AT powarman.de>
 */

#ifndef _GLCDDRIVERS_COMMON_H_
#define _GLCDDRIVERS_COMMON_H_

#include <string>

namespace GLCD
{

const unsigned char bitmask[8] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
const unsigned char bitmaskl[8] = {0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff};
const unsigned char bitmaskr[8] = {0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01};

// functions to enable/disable very short sleeps.
// Therefore, the priority will be changed. => use with care !
int nSleepInit(void);
int nSleepDeInit(void);

void nSleep(long ns);
void uSleep(long us);

unsigned char ReverseBits(unsigned char value);
void clip(int & value, int min, int max);
void sort(int & value1, int & value2);
std::string trim(const std::string & s);

} // end of namespace

#endif
