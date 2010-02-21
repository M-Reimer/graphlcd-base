/*
 * GraphLCD graphics library
 *
 * pbm.h  - PBM file loading and saving
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2006 Andreas Regel <andreas.regel AT powarman.de>
 */

#ifndef _GLCDGRAPHICS_PBM_H_
#define _GLCDGRAPHICS_PBM_H_

#include "imagefile.h"

namespace GLCD
{

class cImage;

class cPBMFile : public cImageFile
{
public:
    cPBMFile();
    virtual ~cPBMFile();
    virtual bool Load(cImage & image, const std::string & fileName);
    virtual bool Save(cImage & image, const std::string & fileName);
};

} // end of namespace

#endif

