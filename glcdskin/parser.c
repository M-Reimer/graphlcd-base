/*
 * GraphLCD skin library
 *
 * parser.c  -  xml parsing
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * based on text2skin
 *
 */

#include <stdio.h>
#include <syslog.h>

#include <vector>
#include <string>

#include "parser.h"
#include "xml.h"
#include "skin.h"

namespace GLCD
{

#define TAG_ERR_REMAIN(_context) do { \
    syslog(LOG_ERR, "ERROR: Text2Skin: Unexpected tag %s within %s", \
        name.c_str(), _context); \
    return false; \
  } while (0)

#define TAG_ERR_CHILD(_context) do { \
    syslog(LOG_ERR, "ERROR: Text2Skin: No child tag %s expected within %s", \
        name.c_str(), _context); \
    return false; \
  } while (0)

#define TAG_ERR_END(_context) do { \
    syslog(LOG_ERR, "ERROR: Text2Skin: Unexpected closing tag for %s within %s", \
        name.c_str(), _context); \
    return false; \
  } while (0)

#define ATTRIB_OPT_STRING(_attr,_target) \
  if (attrs.find(_attr) != attrs.end()) { \
    _target = attrs[_attr]; \
  }

#define ATTRIB_MAN_STRING(_attr,_target) \
  ATTRIB_OPT_STRING(_attr,_target) \
  else { \
    syslog(LOG_ERR, "ERROR: Text2Skin: Mandatory attribute %s missing in tag %s", \
        _attr, name.c_str()); \
    return false; \
  }

#define ATTRIB_OPT_NUMBER(_attr,_target) \
  if (attrs.find(_attr) != attrs.end()) { \
    char *_e; const char *_t = attrs[_attr].c_str(); \
    long _l = strtol(_t, &_e, 10); \
    if (_e ==_t || *_e != '\0') { \
      syslog(LOG_ERR, "ERROR: Text2Skin: Invalid numeric value \"%s\" in attribute %s", \
          _t, _attr); \
      return false; \
    } else \
      _target = _l; \
  }

#define ATTRIB_MAN_NUMBER(_attr,_target) \
  ATTRIB_OPT_NUMBER(_attr,_target) \
  else { \
    syslog(LOG_ERR, "ERROR: Text2Skin: Mandatory attribute %s missing in tag %s", \
        _attr, name.c_str()); \
    return false; \
  }

#define ATTRIB_OPT_BOOL(_attr,_target) \
  if (attrs.find(_attr) != attrs.end()) { \
    if (attrs[_attr] == "yes") \
      _target = true; \
    else if (attrs[_attr] == "no") \
      _target = false; \
    else { \
      syslog(LOG_ERR, "ERROR: Text2Skin: Invalid boolean value \"%s\" in attribute %s", \
          attrs[_attr].c_str(), _attr); \
      return false; \
    } \
  }

#define ATTRIB_MAN_BOOL(_attr,_target) \
  ATTRIB_OPT_BOOL(_attr,_target) \
  else { \
    syslog(LOG_ERR, "ERROR: Text2Skin: Mandatory attribute %s missing in tag %s", \
        _attr, name.c_str()); \
    return false; \
  }

#define ATTRIB_OPT_FUNC(_attr,_func) \
  if (attrs.find(_attr) != attrs.end()) { \
    if (!_func(attrs[_attr])) { \
      syslog(LOG_ERR, "ERROR: Text2Skin: Unexpected value %s for attribute %s", \
          attrs[_attr].c_str(), _attr); \
      return false; \
    } \
  }

#define ATTRIB_MAN_FUNC(_attr,_func) \
  ATTRIB_OPT_FUNC(_attr,_func) \
  else { \
    syslog(LOG_ERR, "ERROR: Text2Skin: Mandatory attribute %s missing in tag %s", \
        _attr, name.c_str()); \
    return false; \
  }

#define ATTRIB_OPT_FUNC_PARAM(_attr,_func,_param) \
  if (attrs.find(_attr) != attrs.end()) { \
    if (!_func(attrs[_attr],_param)) { \
      syslog(LOG_ERR, "ERROR: Text2Skin: Unexpected value %s for attribute %s", \
          attrs[_attr].c_str(), _attr); \
      return false; \
    } \
  }

#define ATTRIB_MAN_FUNC_PARAM(_attr,_func,_param) \
  ATTRIB_OPT_FUNC_PARAM(_attr,_func,_param) \
  else { \
    syslog(LOG_ERR, "ERROR: Text2Skin: Mandatory attribute %s missing in tag %s", \
        _attr, name.c_str()); \
    return false; \
  }

static std::vector<std::string> context;
static cSkin * skin = NULL;
static cSkinFont * font = NULL;
static cSkinVariable * variable = NULL;
static cSkinDisplay * display = NULL;
static std::vector <cSkinObject *> parents;
static cSkinObject * object = NULL;
static uint32_t oindex = 0;

bool StartElem(const std::string & name, std::map<std::string,std::string> & attrs)
{
    //printf("start element: %s\n", name.c_str());

    if (context.size() == 0)
    {
        if (name == "skin")
        {
            ATTRIB_MAN_STRING("version", skin->version);
            ATTRIB_MAN_STRING("name", skin->title);
        }
        else
            TAG_ERR_REMAIN("document");
    }
    else if (name == "variable")
    {
        variable = new cSkinVariable(skin);
        ATTRIB_MAN_STRING("id", variable->mId);
        ATTRIB_MAN_FUNC("value", variable->ParseValue);
        ATTRIB_OPT_FUNC("condition", variable->ParseCondition);
    }
    else if (context[context.size() - 1] == "skin")
    {
        if (name == "font")
        {
            font = new cSkinFont(skin);
            ATTRIB_MAN_STRING("id", font->mId);
            ATTRIB_MAN_FUNC("url", font->ParseUrl);
            ATTRIB_OPT_FUNC("condition", font->ParseCondition);
        }
        else if (name == "display")
        {
            display = new cSkinDisplay(skin);
            ATTRIB_MAN_STRING("id", display->mId);
        }
        else
            TAG_ERR_REMAIN("skin");
    }
    else if (context[context.size() - 1] == "display"
             || context[context.size() - 1] == "list"
             || context[context.size() - 1] == "block")
    {
        if (object != NULL)
        {
            parents.push_back(object);
            object = NULL;
        }

        object = new cSkinObject(display);
        if (object->ParseType(name))
        {
            ATTRIB_OPT_FUNC_PARAM("x1", object->ParseIntParam, object->mPos1.x);
            ATTRIB_OPT_FUNC_PARAM("y1", object->ParseIntParam, object->mPos1.y);
            ATTRIB_OPT_FUNC_PARAM("x2", object->ParseIntParam, object->mPos2.x);
            ATTRIB_OPT_FUNC_PARAM("y2", object->ParseIntParam, object->mPos2.y);
            ATTRIB_OPT_FUNC("width", object->ParseWidth);
            ATTRIB_OPT_FUNC("height", object->ParseHeight);
            ATTRIB_OPT_FUNC("condition", object->ParseCondition);

            if (name == "image")
            {
                ATTRIB_OPT_FUNC_PARAM("x", object->ParseIntParam, object->mPos1.x);
                ATTRIB_OPT_FUNC_PARAM("y", object->ParseIntParam, object->mPos1.y);
                ATTRIB_OPT_FUNC_PARAM("x", object->ParseIntParam, object->mPos2.x);
                ATTRIB_OPT_FUNC_PARAM("y", object->ParseIntParam, object->mPos2.y);
                ATTRIB_OPT_FUNC("color", object->ParseColor);
                ATTRIB_MAN_FUNC("path", object->mPath.Parse);
                ATTRIB_OPT_FUNC("loop", object->ParseScrollLoopMode);
            }
            else if (name == "text"
                || name == "scrolltext")
            {
                ATTRIB_OPT_FUNC("color", object->ParseColor);
                ATTRIB_OPT_FUNC("align", object->ParseAlignment);
                ATTRIB_OPT_FUNC("font", object->ParseFontFace);
                ATTRIB_OPT_BOOL("multiline", object->mMultiline);
                ATTRIB_OPT_FUNC("scrollmode", object->ParseScrollLoopMode);
                ATTRIB_OPT_FUNC("scrollspeed", object->ParseScrollSpeed);
                ATTRIB_OPT_FUNC("scrolltime", object->ParseScrollTime);
#if 0
                if (name == "blink")
                {
                    ATTRIB_OPT_NUMBER("delay", object->mDelay);

                    if (object->mDelay == 0)
                        object->mDelay = 1000;
                }
                else if (name == "marquee")
                {
                    ATTRIB_OPT_NUMBER("delay", object->mDelay);

                    if (object->mDelay == 0)
                        object->mDelay = 500;
                }
#endif
            }
            else if (name == "pixel")
            {
                ATTRIB_OPT_FUNC("color", object->ParseColor);
            }
            else if (name == "line")
            {
                ATTRIB_OPT_FUNC("color", object->ParseColor);
            }
            else if (name == "rectangle")
            {
                ATTRIB_OPT_FUNC("color", object->ParseColor);
                ATTRIB_OPT_BOOL("filled", object->mFilled);
                ATTRIB_OPT_NUMBER("radius", object->mRadius);
            }
            else if (name == "ellipse" || name == "slope")
            {
                ATTRIB_OPT_FUNC("color", object->ParseColor);
                ATTRIB_OPT_BOOL("filled", object->mFilled);
                ATTRIB_OPT_NUMBER("arc", object->mArc);
            }
            else if (name == "progress"
                || name == "scrollbar")
            {
                ATTRIB_OPT_FUNC("color", object->ParseColor);
                ATTRIB_OPT_NUMBER("direction", object->mDirection);
                ATTRIB_OPT_FUNC("current", object->mCurrent.Parse);
                ATTRIB_OPT_FUNC("total", object->mTotal.Parse);
            }
#if 0
            else if (name == "item") {
                ATTRIB_MAN_NUMBER("height",  object->mPos2.y);
                --object->mPos2.y;
            }
#endif
        }
        else
            TAG_ERR_REMAIN(context[context.size() - 1].c_str());
    }
    else
        TAG_ERR_CHILD(context[context.size() - 1].c_str());
    context.push_back(name);
    return true;
}

bool CharData(const std::string & text)
{
    if (text.length() == 0)
        return true;
    if (context.size() == 0)
        return true;
    //printf("char data : %s\n", text.c_str());

    //printf("context: %s\n", context[context.size() - 1].c_str());
    if (context[context.size() - 1] == "text"
        || context[context.size() - 1] == "scrolltext")
    {
        if (!object->mText.Parse(text))
            return false;
    }
    else
        syslog(LOG_ERR, "ERROR: Bad character data");
    return true;
}

bool EndElem(const std::string & name)
{
    //Dprintf("end element: %s\n", name.c_str());
    if (context[context.size() - 1] == name)
    {
        if (name == "font")
        {
            skin->fonts.push_back(font);
            font = NULL;
        }
        else if (name == "variable")
        {
            skin->mVariables.push_back(variable);
            variable = NULL;
        }
        else if (name == "display")
        {
            //display->mNumMarquees = mindex;
            skin->displays.push_back(display);
            display = NULL;
            oindex = 0;
        }
        else if (object != NULL || parents.size() > 0)
        {
            if (object == NULL)
            {
                //printf("rotating parent to object\n");
                object = parents[parents.size() - 1];
                parents.pop_back();
            }
#if 0
            if (object->mCondition == NULL)
            {
                switch (object->mType)
                {
                    case cSkinObject::text:
                    case cSkinObject::scrolltext:
                        object->mCondition = new cxFunction(object->mText);
                        break;

                    default:
                        break;
                }
            }

            object->mIndex = oindex++;
#endif
            if (parents.size() > 0)
            {
                //printf("pushing to parent\n");
                cSkinObject *parent = parents[parents.size() - 1];
                if (parent->mObjects == NULL)
                    parent->mObjects = new cSkinObjects();
                parent->mObjects->push_back(object);
            }
            else
                display->mObjects.push_back(object);
            object = NULL;
        }
        context.pop_back();
    }
    else
        TAG_ERR_END(context[context.size() - 1].c_str());
    return true;
}

cSkin * XmlParse(cSkinConfig & Config, const std::string & Name, const std::string & fileName)
{
    skin = new cSkin(Config, Name);
    context.clear();

    cXML xml(fileName);
    xml.SetNodeStartCB(StartElem);
    xml.SetNodeEndCB(EndElem);
    xml.SetCDataCB(CharData);
    if (xml.Parse() != 0)
    {
        syslog(LOG_ERR, "ERROR: Text2Skin: Parse error in %s, line %d", fileName.c_str(), xml.LineNr());
        delete skin;
        skin = NULL;
        delete display;
        display = NULL;
        delete object;
        object = NULL;
        return NULL;
    }

    cSkin * result = skin;
    skin = NULL;
    return result;
}

} // end of namespace
