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

#ifndef _GLCDGRAPHICS_IMAGEFILE_H_
#define _GLCDGRAPHICS_IMAGEFILE_H_

#include <string>

namespace GLCD
{

class cImage;

class cImageFile
{
public:
    cImageFile();
    virtual ~cImageFile();
    virtual bool Load(cImage & image, const std::string & fileName);
    virtual bool Save(cImage & image, const std::string & fileName);
};

} // end of namespace

#endif
