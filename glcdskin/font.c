#include <syslog.h>

#include <fstream>

#include "font.h"
#include "skin.h"
#include "function.h"

#ifdef HAVE_FONTCONFIG
  #include <fontconfig/fontconfig.h>
#endif

namespace GLCD
{

int cSkinFont::FcInitCount = 0;


cSkinFont::cSkinFont(cSkin * Parent)
:   mSkin(Parent),
    mCondition(NULL),
    mDummyDisplay(mSkin),
    mDummyObject(&mDummyDisplay)
{
}

cSkinFont::~cSkinFont(void) {
#ifdef HAVE_FONTCONFIG
    cSkinFont::FcInitCount --;
    if (cSkinFont::FcInitCount <= 0) {
      FcFini();
    }
#endif
}

bool cSkinFont::ParseUrl(const std::string & url)
{
    bool isFontconfig = false;
    std::string rawfont = "";

    if (url.find("fnt:") == 0)
    {
        mType = ftFNT;
        rawfont = url.substr(4);
        mSize = 0;
    }
    else if (url.find("ft2:") == 0)
    {
        mType = ftFT2;
        rawfont = url.substr(4);
        std::string::size_type pos = rawfont.find(":");
        if (pos == std::string::npos)
        {
            syslog(LOG_ERR, "cFontElement::Load(): No font size specified in %s\n", url.c_str());
            return false;
        }
        std::string tmp = rawfont.substr(pos + 1);
        mSize = atoi(tmp.c_str());
        rawfont = rawfont.substr(0,pos);
    }
#ifdef HAVE_FONTCONFIG
    else if (url.find("fc:") == 0)
    {
        mType = ftFT2;
        isFontconfig = true;
        rawfont = url.substr(3);

        std::string::size_type pos = rawfont.find(":size=");
        if (pos == std::string::npos) {
            syslog(LOG_ERR, "cFontElement::Load(): No font size specified in %s (e.g.: 'size=<fontsize>')\n", url.c_str());
            return false;
        }
        std::string sizeterm = rawfont.substr(pos + 1);

        pos = sizeterm.find(":"); // find terminating :
        if (pos != std::string::npos) {
          sizeterm = sizeterm.substr(0, pos);
        }

        pos = sizeterm.find("=");
        if (pos == std::string::npos) {
            syslog(LOG_ERR, "cFontElement::Load(): Invalid size term '%s' (must be 'size=<fontsize>')\n", url.c_str());
            return false;
        }

        std::string tmp = sizeterm.substr(pos + 1);
        mSize = atoi(tmp.c_str());
        if (mSize <= 0) {
            syslog(LOG_ERR, "cFontElement::Load(): Invalid font size in '%s'\n", url.c_str());
            return false;
        }

        if (cSkinFont::FcInitCount <= 0) {
          FcInit();
        }
        cSkinFont::FcInitCount ++;

        FcPattern *pat = FcNameParse((FcChar8 *) rawfont.c_str() );
        rawfont = "";
        FcPatternAddBool(pat, FC_SCALABLE, FcTrue);
        FcConfigSubstitute(NULL, pat, FcMatchPattern);
        FcDefaultSubstitute(pat);
        FcResult result;
        FcFontSet *fontset = FcFontSort(NULL, pat, FcFalse, NULL, &result);
        if (fontset) {
          FcBool scalable;
          for (int i = 0; i < fontset->nfont; i++) {
            FcPatternGetBool(fontset->fonts[i], FC_SCALABLE, 0, &scalable);
            if (scalable) {
               FcChar8 *s = NULL;
               FcPatternGetString(fontset->fonts[i], FC_FILE, 0, &s);
               rawfont = (char *)s; // set font path
               break;
            }
          }
          FcFontSetDestroy(fontset);
        }
        FcPatternDestroy(pat);

        if (rawfont == "") {
            syslog(LOG_ERR, "cFontElement::Load(): No usable font found for '%s'\n", url.c_str());
            return false;
        }
    }
#endif
    else
    {
        syslog(LOG_ERR, "cSkinFont::ParseUrl(): Unknown font type in %s\n", url.c_str());
        return false;
    }

    if (isFontconfig || (rawfont[0] == '/' || rawfont.find("./") == 0 || rawfont.find("../") == 0)) {
        mFile = rawfont;
    }
    else
    {
        // first try skin's font dir
        mFile = mSkin->Config().SkinPath();
        if (mFile.length() > 0)
        {
            if (mFile[mFile.length() - 1] != '/')
                mFile += '/';
        }
        mFile += "fonts/";
        mFile += rawfont;
#if (__GNUC__ < 3)
        std::ifstream f(mFile.c_str(), std::ios::in | std::ios::binary);
#else
        std::ifstream f(mFile.c_str(), std::ios_base::in | std::ios_base::binary);
#endif
        if (f.is_open())
        {
            f.close();
        }
        else
        {
            // then try generic font dir
            mFile = mSkin->Config().FontPath();
            if (mFile.length() > 0)
            {
                if (mFile[mFile.length() - 1] != '/')
                    mFile += '/';
            }
            mFile += rawfont;
        }
    }

    if (mType == ftFNT)
    {
        return mFont.LoadFNT(mFile);
    }
    else
    {
        return mFont.LoadFT2(mFile, mSkin->Config().CharSet(), mSize);
    }
}

bool cSkinFont::ParseCondition(const std::string & Text)
{
    cSkinFunction *result = new cSkinFunction(&mDummyObject);
    if (result->Parse(Text))
    {
        delete mCondition;
        mCondition = result;
        return true;
    }
    return false;
}

cSkinFonts::cSkinFonts(void)
{
}

cSkinFonts::~cSkinFonts()
{
    iterator it = begin();
    while (it != end())
    {
        delete (*it);
        it++;
    }
}

} // end of namespace
