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

namespace GLCD
{

class cSkin;
class cSkinConfig;

cSkin * XmlParse(cSkinConfig & Config, const std::string & name, const std::string & fileName);

} // end of namespace

#endif
