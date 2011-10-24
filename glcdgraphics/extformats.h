/*
 * GraphLCD graphics library
 *
 * extformats.h  -  loading and saving of external formats (via ImageMagick)
 *
 * based on bitmap.[ch] from text2skin: http://projects.vdr-developer.org/projects/show/plg-text2skin
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2011      Wolfgang Astleitner <mrwastl AT users sourceforge net>
 */

#ifndef _GLCDGRAPHICS_EXTFORMATS_H_
#define _GLCDGRAPHICS_EXTFORMATS_H_

#include "imagefile.h"

namespace GLCD
{

class cImage;

class cExtFormatFile : public cImageFile
{
public:
    cExtFormatFile();
    virtual ~cExtFormatFile();
    virtual bool Load(cImage & image, const std::string & fileName);
    virtual bool Save(cImage & image, const std::string & fileName);

    virtual bool LoadScaled(cImage & image, const std::string & fileName, uint16_t & scalew, uint16_t & scaleh);
};

} // end of namespace

#endif
