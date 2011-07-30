/*
 * GraphLCD skin library
 *
 * parser.h  -  xml parsing
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * based on text2skin
 *
 */

#ifndef _GLCDSKIN_PARSER_H_
#define _GLCDSKIN_PARSER_H_


#include <string>

// max. version of skin definitions supported by the parser
#define GLCDSKIN_SKIN_VERSION    1.2


namespace GLCD
{

class cSkin;
class cSkinConfig;

cSkin * XmlParse(cSkinConfig & Config, const std::string & name, const std::string & fileName, std::string & errorString);

// provide old function for compatibility
cSkin * XmlParse(cSkinConfig & Config, const std::string & name, const std::string & fileName)
{ std::string errorString = "";
  return XmlParse(Config, name, fileName, errorString);
}

} // end of namespace

#endif
