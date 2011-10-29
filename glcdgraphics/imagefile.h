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
private:
    uint32_t Blend(uint32_t fgcol, uint32_t bgcol, uint8_t level, double antiAliasGranularity = 0.0) const;
protected:
    bool Scale(cImage & image, uint16_t scalew, uint16_t scaleh, bool AntiAlias = false);
public:
    cImageFile();
    virtual ~cImageFile();
    virtual bool Load(cImage & image, const std::string & fileName);
    virtual bool Save(cImage & image, const std::string & fileName);

    //virtual bool SupportsScaling(void) { return true; }

    virtual bool LoadScaled(cImage & image, const std::string & fileName, uint16_t & scalew, uint16_t & scaleh);
};

} // end of namespace

#endif
