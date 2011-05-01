/*
 * GraphLCD skin library
 *
 * display.h  -  display class
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * based on text2skin
 *
 */

#ifndef _GLCDSKIN_DISPLAY_H_
#define _GLCDSKIN_DISPLAY_H_

#include <string>
#include <vector>

#include "object.h"

namespace GLCD
{

class cSkin;

class cSkinDisplay
{
    friend bool StartElem(const std::string &name, std::map<std::string,std::string> &attrs);
    friend bool EndElem(const std::string &name);

private:
    cSkin * mSkin;
    std::string mId;
    cSkinObjects mObjects;

public:
    cSkinDisplay(cSkin * Parent);

    cSkin * Skin(void) const { return mSkin; }
    const std::string & Id(void) const { return mId; }

    uint32_t NumObjects(void) const { return mObjects.size(); }
    cSkinObject * GetObject(uint32_t n) const { return mObjects[n]; }

    void Render(cBitmap * screen);

    bool NeedsUpdate(uint64_t CurrentTime);

    std::string CheckAction(cGLCDEvent * ev);
};

class cSkinDisplays: public std::vector<cSkinDisplay *>
{
public:
    cSkinDisplays(void);
    ~cSkinDisplays(void);
};

} // end of namespace

#endif
