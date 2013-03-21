/*
 * GraphLCD graphics library
 *
 * imagefile.h  -  base class for file loading and saving
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2006      Andreas Regel <andreas.regel AT powarman.de>
 * (c) 2010-2013 Wolfgang Astleitner <mrwastl AT users sourceforge net>
 */

#include "image.h"
#include "imagefile.h"
#include "bitmap.h"

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



bool cImageFile::LoadScaled(cImage & image, const std::string & fileName, uint16_t & scalew, uint16_t & scaleh)
{
    if (Load(image, fileName)) {
        if (scalew || scaleh) {
            return image.Scale(scalew, scaleh, true);
        } else {
            return true;
        }
    } else {
      scalew = 0;
      scaleh = 0;
      return false;
    }
}

} // end of namespace
