/*
 * GraphLCD graphics library
 *
 * imagefile.h  -  base class for file loading and saving
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2006 Andreas Regel <andreas.regel AT powarman.de>
 */

#include "image.h"
#include "imagefile.h"

namespace GLCD
{

cImageFile::cImageFile(void)
{
}
cImageFile::~cImageFile(void)
{
}

bool cImageFile::Load(cImage & image, const std::string & fileName)
{
    return false;
}

bool cImageFile::Save(cImage & image, const std::string & fileName)
{
    return false;
}

} // end of namespace
