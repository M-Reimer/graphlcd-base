#include "config.h"
#include "type.h"

#include <sys/time.h>

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

uint64_t cSkinConfig::Now(void)
{
    struct timeval tv;

    gettimeofday(&tv, 0);
    return (uint64_t)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
}


} // end of namespace
