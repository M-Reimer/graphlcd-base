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
    errorDetail = "Unexpected tag "+name+" within "+ _context; \
    syslog(LOG_ERR, "ERROR: graphlcd/skin: %s", errorDetail.c_str()); \
    return false; \
  } while (0)

#define TAG_ERR_CHILD(_context) do { \
    errorDetail = "No child tag "+name+" expected within "+ _context; \
    syslog(LOG_ERR, "ERROR: graphlcd/skin: %s", errorDetail.c_str()); \
    return false; \
  } while (0)

#define TAG_ERR_END(_context) do { \
    errorDetail = "Unexpected closing tag for "+name+" within "+ _context; \
    syslog(LOG_ERR, "ERROR: graphlcd/skin: %s", errorDetail.c_str()); \
    return false; \
  } while (0)

#define ATTRIB_OPT_STRING(_attr,_target) \
  if (attrs.find(_attr) != attrs.end()) { \
    _target = attrs[_attr]; \
  }

#define ATTRIB_MAN_STRING(_attr,_target) \
  ATTRIB_OPT_STRING(_attr,_target) \
  else { \
    errorDetail = "Mandatory attribute "+ (std::string)_attr +" missing in tag "+ name; \
    syslog(LOG_ERR, "ERROR: graphlcd/skin: %s", errorDetail.c_str()); \
    return false; \
  }

#define ATTRIB_OPT_NUMBER(_attr,_target) \
  if (attrs.find(_attr) != attrs.end()) { \
    char *_e; const char *_t = attrs[_attr].c_str(); \
    long _l = strtol(_t, &_e, 10); \
    if (_e ==_t || *_e != '\0') { \
      errorDetail = "Invalid numeric value \""+ (std::string)_t +"\" in attribute "+ (std::string)_attr; \
      syslog(LOG_ERR, "ERROR: graphlcd/skin: %s", errorDetail.c_str()); \
      return false; \
    } else \
      _target = _l; \
  }

#define ATTRIB_MAN_NUMBER(_attr,_target) \
  ATTRIB_OPT_NUMBER(_attr,_target) \
  else { \
    errorDetail = "Mandatory attribute "+ (std::string)_attr +" missing in tag "+name; \
    syslog(LOG_ERR, "ERROR: graphlcd/skin: %s", errorDetail.c_str()); \
    return false; \
  }

#define ATTRIB_OPT_BOOL(_attr,_target) \
  if (attrs.find(_attr) != attrs.end()) { \
    if (attrs[_attr] == "yes") \
      _target = true; \
    else if (attrs[_attr] == "no") \
      _target = false; \
    else { \
      errorDetail = "Invalid boolean value \""+ attrs[_attr] +"\" in attribute "+ _attr; \
      syslog(LOG_ERR, "ERROR: graphlcd/skin: %s", errorDetail.c_str()); \
      return false; \
    } \
  }

#define ATTRIB_MAN_BOOL(_attr,_target) \
  ATTRIB_OPT_BOOL(_attr,_target) \
  else { \
    errorDetail = "Mandatory attribute "+ (std::string)_attr +" missing in tag "+name; \
    syslog(LOG_ERR, "ERROR: graphlcd/skin: %s", errorDetail.c_str()); \
    return false; \
  }

#define ATTRIB_OPT_FUNC(_attr,_func) \
  if (attrs.find(_attr) != attrs.end()) { \
    if (!_func(attrs[_attr])) { \
      errorDetail = "Unexpected value \""+ attrs[_attr] +"\" for attribute "+ (std::string)_attr; \
      syslog(LOG_ERR, "ERROR: graphlcd/skin: %s", errorDetail.c_str()); \
      return false; \
    } \
  }

#define ATTRIB_MAN_FUNC(_attr,_func) \
  ATTRIB_OPT_FUNC(_attr,_func) \
  else { \
    errorDetail = "Mandatory attribute "+ (std::string)_attr +" missing in tag "+name; \
    syslog(LOG_ERR, "ERROR: graphlcd/skin: %s", errorDetail.c_str()); \
    return false; \
  }

#define ATTRIB_OPT_FUNC_PARAM(_attr,_func,_param) \
  if (attrs.find(_attr) != attrs.end()) { \
    if (!_func(attrs[_attr],_param)) { \
      errorDetail = "Unexpected value "+ attrs[_attr] +" for attribute "+ (std::string)_attr; \
      syslog(LOG_ERR, "ERROR: graphlcd/skin: %s", errorDetail.c_str()); \
      return false; \
    } \
  }

#define ATTRIB_MAN_FUNC_PARAM(_attr,_func,_param) \
  ATTRIB_OPT_FUNC_PARAM(_attr,_func,_param) \
  else { \
    errorDetail = "Mandatory attribute "+ (std::string)_attr +" missing in tag "+name; \
    syslog(LOG_ERR, "ERROR: graphlcd/skin: %s", errorDetail.c_str()); \
    return false; \
  }

static std::vector<std::string> context;
static cSkin * skin = NULL;
static cSkinFont * font = NULL;
static cSkinVariable * variable = NULL;
static cSkinVariable * variable_default = NULL;
static cSkinDisplay * display = NULL;
static std::vector <cSkinObject *> parents;
static cSkinObject * object = NULL;
static uint32_t oindex = 0;
static std::string errorDetail = "";
static std::string condblock_cond = "";


static bool CheckSkinVersion(const std::string & version) {
  float currv;
  char* ecptr = NULL;
  const char* verscstr = version.c_str();
  currv = strtof(verscstr, &ecptr);
  if ( (*ecptr != '\0') || (ecptr == NULL) /*|| (ecptr != verscstr)*/ || 
       ((int)(GLCDSKIN_SKIN_VERSION * 100.0) < (int)(currv * 100.0))
     )
  {
   return false;            
  }
  return true;  
}



bool StartElem(const std::string & name, std::map<std::string,std::string> & attrs)
{
    //printf("start element: %s\n", name.c_str());
// if (context.size() > 0) fprintf(stderr, "context: %s\n", context[context.size() - 1].c_str());
    if (context.size() == 0)
    {
        if (name == "skin")
        {
            ATTRIB_MAN_STRING("version", skin->version);
            ATTRIB_MAN_STRING("name", skin->title);
            ATTRIB_OPT_FUNC("enable", skin->ParseEnable);

            if (! CheckSkinVersion(skin->version) ) {
              errorDetail = "skin version '"+ skin->version +"' not supported.";
              syslog(LOG_ERR, "ERROR: graphlcd/skin: %s", errorDetail.c_str());
              return false;            
            }
        }
        else
            TAG_ERR_REMAIN("document");
    }
    else if (name == "condblock")
    {
        int i = context.size() - 1;
        while (i >= 0) {
          if (context[i] == "condblock") {
            errorDetail = "'condblock' must not be nested in another 'condblock'.";
            syslog(LOG_ERR, "ERROR: graphlcd/skin: %s", errorDetail.c_str());
            return false;            
          }
          i--;
        }
        ATTRIB_MAN_STRING("condition", condblock_cond);
    }
    else if (name == "variable")
    {
        variable = new cSkinVariable(skin);
        ATTRIB_MAN_STRING("id", variable->mId);
        ATTRIB_MAN_FUNC("value", variable->ParseValue);
        if (context[context.size() - 1] == "condblock") {
          if (attrs.find("condition") != attrs.end()) {
            errorDetail = "variable \""+variable->mId+"\" must not contain a condition when context = 'condblock'.";
            syslog(LOG_ERR, "ERROR: graphlcd/skin: %s", errorDetail.c_str());
            return false;
          } else {
            if (! variable->ParseCondition(condblock_cond)) {
              errorDetail = "Unexpected value \""+ attrs["condition"] +"\" for attribute "+ (std::string)"condition";
              syslog(LOG_ERR, "ERROR: graphlcd/skin: %s", errorDetail.c_str());
              return false;
            }
          }
        } else {
          ATTRIB_OPT_FUNC("condition", variable->ParseCondition);
        }
        // if a 'default' value is set, create a second variable w/o condition: will be used if condition is not true
        // as variables all have global scope (no matter where defined) and will always be sought from the start of the array,
        // the default variable will be inserted _after_ the variable containing the condition.
        if (attrs.find("default") != attrs.end()) {
          variable_default = new cSkinVariable(skin);
          ATTRIB_MAN_STRING("id", variable_default->mId);
          ATTRIB_MAN_FUNC("default", variable_default->ParseValue);
        }
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
            ATTRIB_OPT_STRING("action", object->mAction);

            if (name == "image")
            {
                ATTRIB_OPT_FUNC_PARAM("x", object->ParseIntParam, object->mPos1.x);
                ATTRIB_OPT_FUNC_PARAM("y", object->ParseIntParam, object->mPos1.y);
                ATTRIB_OPT_FUNC_PARAM("x", object->ParseIntParam, object->mPos2.x);
                ATTRIB_OPT_FUNC_PARAM("y", object->ParseIntParam, object->mPos2.y);
                ATTRIB_OPT_FUNC_PARAM("color", object->ParseColor, object->mColor);
                ATTRIB_OPT_FUNC_PARAM("bgcolor", object->ParseColor, object->mBackgroundColor);
                ATTRIB_MAN_FUNC("path", object->mPath.Parse);
                ATTRIB_OPT_FUNC("loop", object->ParseScrollLoopMode);
            }
            else if (name == "text"
                || name == "scrolltext")
            {
                ATTRIB_OPT_FUNC_PARAM("color", object->ParseColor, object->mColor);
                ATTRIB_OPT_FUNC_PARAM("bgcolor", object->ParseColor, object->mBackgroundColor);
                ATTRIB_OPT_FUNC("align", object->ParseAlignment);
                ATTRIB_OPT_FUNC("valign", object->ParseVerticalAlignment);
                ATTRIB_OPT_FUNC("font", object->ParseFontFace);
                ATTRIB_OPT_BOOL("multiline", object->mMultiline);
                ATTRIB_OPT_FUNC("scrollmode", object->ParseScrollLoopMode);
                ATTRIB_OPT_FUNC("scrollspeed", object->ParseScrollSpeed);
                ATTRIB_OPT_FUNC("scrolltime", object->ParseScrollTime);
                ATTRIB_OPT_STRING("alttext", object->mAltText);
                ATTRIB_OPT_FUNC("altcondition", object->ParseAltCondition);
            }
            else if (name == "button")
            {
                ATTRIB_OPT_FUNC_PARAM("labelcolor", object->ParseColor, object->mColor);
                ATTRIB_OPT_FUNC_PARAM("color", object->ParseColor, object->mBackgroundColor);
                ATTRIB_OPT_FUNC("font", object->ParseFontFace);
                ATTRIB_OPT_NUMBER("radius", object->mRadius);
            }
            else if (name == "pixel")
            {
                ATTRIB_OPT_FUNC_PARAM("color", object->ParseColor, object->mColor);
            }
            else if (name == "line")
            {
                ATTRIB_OPT_FUNC_PARAM("color", object->ParseColor, object->mColor);
            }
            else if (name == "rectangle")
            {
                ATTRIB_OPT_FUNC_PARAM("color", object->ParseColor, object->mColor);
                ATTRIB_OPT_BOOL("filled", object->mFilled);
                ATTRIB_OPT_NUMBER("radius", object->mRadius);
            }
            else if (name == "ellipse" || name == "slope")
            {
                ATTRIB_OPT_FUNC_PARAM("color", object->ParseColor, object->mColor);
                ATTRIB_OPT_BOOL("filled", object->mFilled);
                ATTRIB_OPT_NUMBER("arc", object->mArc);
            }
            else if (name == "progress"
                || name == "scrollbar")
            {
                ATTRIB_OPT_FUNC_PARAM("color", object->ParseColor, object->mColor);
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
        || context[context.size() - 1] == "scrolltext"
        || context[context.size() - 1] == "button")
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
//fprintf(stderr, "  variable         '%s', value: %s\n", variable->mId.c_str(), ((std::string)variable->Value()).c_str());
            variable = NULL;
            if (variable_default != NULL) {
              skin->mVariables.push_back(variable_default);
//fprintf(stderr, "  variable default '%s', value: %s\n", variable_default->mId.c_str(), ((std::string)variable_default->Value()).c_str());
              variable_default = NULL;
            }
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

cSkin * XmlParse(cSkinConfig & Config, const std::string & Name, const std::string & fileName, std::string & errorString)
{
    skin = new cSkin(Config, Name);
    context.clear();

    cXML xml(fileName);
    xml.SetNodeStartCB(StartElem);
    xml.SetNodeEndCB(EndElem);
    xml.SetCDataCB(CharData);
    if (xml.Parse() != 0)
    {
        char buff[8];
        snprintf(buff, 7, "%d", xml.LineNr());
        syslog(LOG_ERR, "ERROR: graphlcd/skin: Parse error in %s, line %d", fileName.c_str(), xml.LineNr());
        // shorter version outgoing errorString (eg. displaying errorString on the display)
        errorString = "Parse error in skin "+Name+", line "+buff;
        if (errorDetail != "")
            errorString += ":\n"+errorDetail;
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
    errorString = "";
    return result;
}

} // end of namespace
