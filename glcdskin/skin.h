/*
 * GraphLCD skin library
 *
 * skin.h  -  skin class
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * based on text2skin
 *
 */

#ifndef _GLCDSKIN_SKIN_H_
#define _GLCDSKIN_SKIN_H_

#include <string>

#include "display.h"
#include "font.h"
#include "type.h"
#include "string.h"
#include "cache.h"
#include "config.h"
#include "variable.h"


namespace GLCD
{

class cSkin
{
    friend bool StartElem(const std::string & name, std::map<std::string,std::string> & attrs);
    friend bool EndElem(const std::string & name);

private:
    cSkinConfig & config;
    std::string name;
    std::string title;
    std::string version;
    tSize baseSize;

    cSkinFonts fonts;
    cSkinDisplays displays;
    cSkinVariables mVariables;
    cImageCache * mImageCache;
    uint64_t  tsEvalTick;
    uint64_t  tsEvalSwitch;

public:
    cSkin(cSkinConfig & Config, const std::string & Name);
    ~cSkin(void);

    void SetBaseSize(int width, int height);

    cSkinFont * GetFont(const std::string & Id);
    cSkinDisplay * GetDisplay(const std::string & Id);
    cSkinVariable * GetVariable(const std::string & Id);

    cSkinConfig & Config(void) { return config; }
    const std::string & Name(void) const { return name; }
    const std::string & Title(void) const { return title; }
    const std::string & Version(void) const { return version; }
    const tSize & BaseSize(void) const { return baseSize; }

    cImageCache * ImageCache(void) { return mImageCache; }

    bool ParseEnable(const std::string &Text);

    cColor GetBackgroundColor(void) { return config.GetDriver()->GetBackgroundColor(); }
    cColor GetForegroundColor(void) { return config.GetDriver()->GetForegroundColor(); }
    
    void     SetTSEvalTick(uint64_t ts) { tsEvalTick = ts; }
    void     SetTSEvalSwitch(uint64_t ts) { tsEvalSwitch = ts; }
    const uint64_t GetTSEvalTick(void) { return tsEvalTick; }
    const uint64_t GetTSEvalSwitch(void) { return tsEvalSwitch; }
};

} // end of namespace

#endif
