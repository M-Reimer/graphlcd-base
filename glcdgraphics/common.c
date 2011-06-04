/*
 * GraphLCD graphics library
 *
 * common.c  -  various functions
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2004 Andreas Regel <andreas.regel AT powarman.de>
 */

#include <ctype.h>
#include <syslog.h>

#include "common.h"


namespace GLCD
{

void clip(int & value, int min, int max)
{
    if (value < min)
        value = min;
    if (value > max)
        value = max;
}

void sort(int & value1, int & value2)
{
    if (value2 < value1)
    {
        int tmp;
        tmp = value2;
        value2 = value1;
        value1 = tmp;
    }
}

std::string trim(const std::string & s)
{
	std::string::size_type start, end;

	start = 0;
	while (start < s.length())
	{
		if (!isspace(s[start]))
			break;
		start++;
	}
	end = s.length() - 1;
	while (end >= 0)
	{
		if (!isspace(s[end]))
			break;
		end--;
	}
	return s.substr(start, end - start + 1);
}


bool encodedCharAdjustCounter(const bool isutf8, const std::string & str, uint32_t & c, unsigned int & i, const uint32_t errChar)
{
    bool rv = false;

    if (i >= str.length())
        return rv;
        
    if ( isutf8 ) {
        uint8_t c0,c1,c2,c3;
        c = str[i];
        c0 = str[i];
        c1 = (i+1 < str.length()) ? str[i+1] : 0;
        c2 = (i+2 < str.length()) ? str[i+2] : 0;
        c3 = (i+3 < str.length()) ? str[i+3] : 0;

        if ( (c0 & 0x80) == 0x00) {
            // one byte: 0xxxxxxx
            c = c0;
            rv = true;
        } else if ( (c0 & 0xE0) == 0xC0 ) {
            // two byte utf8: 110yyyyy 10xxxxxx -> 00000yyy yyxxxxxx
            if ( (c1 & 0xC0) == 0x80 ) {
                c = ( (c0 & 0x1F) << 6 ) | ( (c1 & 0x3F) );
                rv = true;
            } else {
                //syslog(LOG_INFO, "GraphLCD: illegal 2-byte UTF-8 sequence found: %02x %02x, pos=%d, str: %s\n", c0,c1,i,str.c_str());
                c = errChar;
            }
            i += 1;            
        } else if ( (c0 & 0xF0) == 0xE0 ) {
            // three byte utf8: 1110zzzz 10yyyyyy 10xxxxxx -> zzzzyyyy yyxxxxxx
            if ( ((c1 & 0xC0) == 0x80) && ((c2 & 0xC0) == 0x80) ) {
                c = ( (c0 & 0x0F) << 12 ) | ( (c1 & 0x3F) << 6 ) | ( c2 & 0x3F );
                rv = true;
            } else {
                //syslog(LOG_INFO, "GraphLCD: illegal 3-byte UTF-8 sequence found: %02x %02x %02x, pos=%d, str: %s\n", c0,c1,c2,i,str.c_str());
                c = errChar;
            }
            i += 2;
        } else if ( (c0 & 0xF8) == 0xF0 ) {
            // four byte utf8: 11110www 10zzzzzz 10yyyyyy 10xxxxxx -> 000wwwzz zzzzyyyy yyxxxxxx
            if ( ((c1 & 0xC0) == 0x80) && ((c2 & 0xC0) == 0x80) && ((c3 & 0xC0) == 0x80) ) {
                c = ( (c0 & 0x07) << 18 ) | ( (c1 & 0x3F) << 12 ) | ( (c2 & 0x3F) << 6 ) | (c3 & 0x3F);
                rv = true;
            } else {
                //syslog(LOG_INFO, "GraphLCD: illegal 4-byte UTF-8 sequence found: %02x %02x %02x %02x, pos=%d, str: %s\n", c0,c1,c2,c3,i,str.c_str());
                c = errChar;
            }
            i += 3;
        } else {
            // 1xxxxxxx is invalid!
            //syslog(LOG_INFO, "GraphLCD: illegal 1-byte UTF-8 char found: %02x, pos=%d, str: %s\n", c0,i,str.c_str());
            c = errChar;
        }
    } else {
        c = str[i];
        rv = true;
    }
    return rv;
}

} // end of namespace
