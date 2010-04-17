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

