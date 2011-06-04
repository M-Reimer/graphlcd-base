/*
 * GraphLCD graphics library
 *
 * common.h  -  various functions
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2004 Andreas Regel <andreas.regel AT powarman.de>
 */

#ifndef _GLCDGRAPHICS_COMMON_H_
#define _GLCDGRAPHICS_COMMON_H_

#include <string>
#include <stdint.h>

// character to return when erraneous utf-8 sequence  (for now: space)
//#define UTF8_ERRCHAR 0x0020
// for debugging issues return '_' instead:
#define UTF8_ERRCHAR 0x005F


namespace GLCD
{

void clip(int & value, int min, int max);
void sort(int & value1, int & value2);
std::string trim(const std::string & s);
bool encodedCharAdjustCounter(const bool isutf8, const std::string & str, uint32_t & c, unsigned int & i, const uint32_t errChar = UTF8_ERRCHAR);

} // end of namespace

#endif
