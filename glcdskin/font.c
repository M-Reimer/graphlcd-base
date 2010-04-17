#include <syslog.h>

#include <fstream>

#include "font.h"
#include "skin.h"
#include "function.h"

namespace GLCD
{

cSkinFont::cSkinFont(cSkin * Parent)
:   mSkin(Parent),
    mCondition(NULL),
    mDummyDisplay(mSkin),
    mDummyObject(&mDummyDisplay)
{
}

bool cSkinFont::ParseUrl(const std::string & url)
{
    std::string::size_type count = std::string::npos;

    if (url.find("fnt:") == 0)
    {
        mType = ftFNT;
        mSize = 0;
    }
    else if (url.find("ft2:") == 0)
    {
        mType = ftFT2;
        std::string::size_type pos = url.find(":", 4);
        if (pos == std::string::npos)
        {
            syslog(LOG_ERR, "cFontElement::Load(): No font size specified in %s\n", url.c_str());
            return false;
        }
        std::string tmp = url.substr(pos + 1);
        mSize = atoi(tmp.c_str());
        count = pos - 4;
    }
    else
    {
        syslog(LOG_ERR, "cSkinFont::ParseUrl(): Unknown font type in %s\n", url.c_str());
        return false;
    }

    if (url[4] == '/' || url.find("./") == 4 || url.find("../") == 4)
        mFile = url.substr(4, count);
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
        mFile += url.substr(4, count);
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
            mFile += url.substr(4, count);
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
