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

} // end of namespace
