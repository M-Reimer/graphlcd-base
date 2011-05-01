/*
 * GraphLCD skin library
 *
 * display.c  -  display class
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * based on text2skin
 *
 */

#include "display.h"

namespace GLCD
{

cSkinDisplay::cSkinDisplay(cSkin * parent)
:   mSkin(parent),
    mId("")
{
}

void cSkinDisplay::Render(cBitmap * screen)
{
    for (uint32_t i = 0; i < NumObjects(); ++i)
        GetObject(i)->Render(screen);
}


bool cSkinDisplay::NeedsUpdate(uint64_t CurrentTime)
{
    for (uint32_t i = 0; i < NumObjects(); ++i) {
        if ( GetObject(i)->NeedsUpdate(CurrentTime) ) {
            return true;
        }
    }
    return false;
}


std::string cSkinDisplay::CheckAction(cGLCDEvent * ev) {
    std::string rv = "";

    if (!ev)
        return "";

    for (uint32_t i = 0; i < NumObjects(); ++i) {
        if ( (rv = GetObject(i)->CheckAction(ev) ) != "" ) {
            return rv;
        }
    }
    return "";
}


cSkinDisplays::cSkinDisplays(void)
{
}

cSkinDisplays::~cSkinDisplays()
{
    iterator it = begin();
    while (it != end())
    {
        delete (*it);
        it++;
    }
}

} // end of namespace

