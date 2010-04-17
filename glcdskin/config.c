#include "config.h"
#include "type.h"

namespace GLCD
{

std::string cSkinConfig::SkinPath(void)
{
    return ".";
}

std::string cSkinConfig::FontPath(void)
{
    return ".";
}

std::string cSkinConfig::CharSet(void)
{
    return "iso-8859-15";
}

std::string cSkinConfig::Translate(const std::string & Text)
{
    return Text;
}

cType cSkinConfig::GetToken(const tSkinToken & Token)
{
    return "";
}

int cSkinConfig::GetTokenId(const std::string & Name)
{
    return 0;
}

int cSkinConfig::GetTabPosition(int Index, int MaxWidth, const cFont & Font)
{
    return 0;
}

} // end of namespace
