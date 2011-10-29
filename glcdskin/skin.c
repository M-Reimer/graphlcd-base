/*
 * GraphLCD skin library
 *
 * skin.c  -  skin class
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * based on text2skin
 *
 */

#include "skin.h"
#include "function.h"

namespace GLCD
{

cSkin::cSkin(cSkinConfig & Config, const std::string & Name)
:   config(Config),
    name(Name)
{
    mImageCache = new cImageCache(this, 100);
    tsEvalTick = 0;
    tsEvalSwitch = 0;
}

cSkin::~cSkin(void)
{
    delete mImageCache;
}

void cSkin::SetBaseSize(int width, int height)
{
    baseSize.w = width;
    baseSize.h = height;
}

cSkinFont * cSkin::GetFont(const std::string & Id)
{
    cSkinFonts::iterator it = fonts.begin();
    while (it != fonts.end())
    {
        if ((*it)->Id() == Id)
        {
            if ((*it)->Condition() == NULL || (*it)->Condition()->Evaluate())
                return (*it);
        }
        it++;
    }
    return NULL;
}

cSkinDisplay * cSkin::GetDisplay(const std::string & Id)
{
    cSkinDisplays::iterator it = displays.begin();
    while (it != displays.end())
    {
        if ((*it)->Id() == Id)
        {
            return (*it);
        }
        it++;
    }
    return NULL;
}

cSkinVariable * cSkin::GetVariable(const std::string & Id)
{
    cSkinVariables::iterator it = mVariables.begin();
    while (it != mVariables.end())
    {
        if ((*it)->Id() == Id)
        {
            if ((*it)->Condition() == NULL || (*it)->Condition()->Evaluate())
                return (*it);
        }
        it++;
    }
    return NULL;
}


bool cSkin::ParseEnable(const std::string & Text)
{
    cDriver * driver = config.GetDriver();

    if (!driver)
        return false;

    driver->SetFeature(Text, 1);
    return true; // always return true else loading the skin would fail if touchscreen is not available
}


} // end of namespace
