/*
 * GraphLCD skin library
 *
 * string.h  -  skin string class
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * based on text2skin
 *
 */

#include <syslog.h>

#include "skin.h"
#include "object.h"
#include "string.h"

namespace GLCD
{

tSkinToken::tSkinToken(void)
:   Index(-1)
{
}

tSkinToken::tSkinToken(int id, std::string n, uint32_t o, const std::string & a)
:   Id(id),
    Name(n),
    Offset(o),
    Attrib(a),
    MaxItems(-1),
    Index(-1)
{
}

bool operator< (const tSkinToken & A, const tSkinToken & B)
{
    return A.Id == B.Id
        ? A.Name == B.Name
            ? A.Attrib == B.Attrib
                ? A.Index < B.Index
                : A.Attrib < B.Attrib
            : A.Name < B.Name
        : A.Id < B.Id;
}

std::string tSkinToken::Token(const tSkinToken & Token)
{
    std::string result = (std::string) "{" + Token.Name;
    //if (Token.Attrib.length() > 0)
    //  result += ":" + Token.Attrib;
    result += "}";

    return result;
}

#if 0
cSkinString::tStringList cSkinString::mStrings;
#endif

cSkinString::cSkinString(cSkinObject *Parent, bool Translate)
:   mObject(Parent),
    mSkin(Parent->Skin()),
    mTranslate(Translate)
{
#if 0
    mStrings.push_back(this);
#endif
}

cSkinString::~cSkinString()
{
#if 0
    tStringList::iterator it = mStrings.begin();
    for (; it != mStrings.end(); ++it) {
        if ((*it) == this) {
            mStrings.erase(it);
            break;
        }
    }
#endif
}

#if 0
void cSkinString::Reparse(void)
{
    tStringList::iterator it = mStrings.begin();
    for (; it != mStrings.end(); ++it) {
        if ((*it)->mTranslate && (*it)->mText.length() > 0)
            (*it)->Parse((*it)->mOriginal, true);
    }
}
#endif


// copied from xml.c (should be valid for parsing variable names too ...)
static bool IsTokenChar(bool start, int c) {
    return isalpha(c) || c == '_' || (!start && isdigit(c));
}


bool cSkinString::Parse(const std::string & Text, bool Translate)
{
    std::string trans = Translate ? mSkin->Config().Translate(Text) : Text;
    const char * ptr, * last;
    bool inToken = false;
    bool inAttrib = false;
    int offset = 0;

    //Dprintf("parsing: %s\n", Text.c_str());
    mOriginal = Text;
    mText = "";
    mTokens.clear();

    ptr = trans.c_str();
    last = trans.c_str();
    for (; *ptr; ++ptr) {
        if (inToken && *ptr == '\\') {
            if (*(ptr + 1) == '\0') {
                syslog(LOG_ERR, "ERROR: graphlcd/skin/string: Stray \\ in token attribute\n");
                return false;
            }

            ++ptr;
            continue;
        }
        else if (*ptr == '#') {
          if (inToken) {
              syslog(LOG_ERR, "ERROR: graphlcd/skin/string: Unexpected '#' in token");
              return false;
          }

          mText.append(last, ptr - last);

          bool isStartChar = true;
          const char * varNameStart = ptr;
          ptr++;
          while (*ptr && IsTokenChar(isStartChar, *ptr)) {
            isStartChar = false;
            ptr++;
            offset++;
          }
          // add #VARNAME#
          mText.append(varNameStart, (ptr - varNameStart));
          mText.append("#");
          offset +=2; // adds two '#' -> fix offset
          ptr--; // we'd be at the correct position now but the for-loop does a ++ptr -> fix it by stepping back one char
          last = ptr + 1;
        }
        else if (*ptr == '{') {
            if (inToken) {
                syslog(LOG_ERR, "ERROR: graphlcd/skin/string: Unexpected '{' in token");
                return false;
            }

            mText.append(last, ptr - last);
            last = ptr + 1;
            inToken = true;
        }
        else if (*ptr == '}' || (inToken && *ptr == ':')) {
            if (!inToken) {
                syslog(LOG_ERR, "ERROR: graphlcd/skin/string: Unexpected '}' outside of token");
                return false;
            }

            if (inAttrib) {
                if (*ptr == ':') {
                    syslog(LOG_ERR, "ERROR: graphlcd/skin/string: Unexpected ':' inside of token attribute");
                    return false;
                }

                int pos = -1;
                std::string attr;
                attr.assign(last, ptr - last);
                while ((pos = attr.find('\\', pos + 1)) != -1) {
                    switch (attr[pos + 1]) {
                        case 'n':
                            attr.replace(pos, 2, "\n");
                            break;

                        default:
                            attr.erase(pos, 1);
                    }
                }

                tSkinToken &lastToken = mTokens[mTokens.size() - 1];
                if (attr == "clean")
                    lastToken.Attrib = aClean;
                else if (attr == "rest")
                    lastToken.Attrib = aRest;
                else {
                    char *end;
                    int n = strtol(attr.c_str(), &end, 10);
                    //Dprintf("attr: %s, n: %d, end: |%s|\n", attr.c_str(), n, end);
                    if (end != attr.c_str() && *end == '\0') {
                        //Dprintf("a number\n");
                        lastToken.Attrib = n;
                    } else
                        lastToken.Attrib = attr;
                }

                inAttrib = false;
                inToken = false;
            } else {
                if (true)
                {
                    std::string tmp;
                    tmp.assign(last, ptr - last);
                    if (tmp != "") { // ignore empty token
                      tSkinToken token(mSkin->Config().GetTokenId(tmp), tmp, offset, "");
                      mTokens.push_back(token);
                    }
                }
                else
                {
                    syslog(LOG_ERR, "ERROR: graphlcd/skin/string: Unexpected token {%.*s}", (int)(ptr - last), last);
                    return false;
                }

                if (*ptr == ':')
                    inAttrib = true;
                else
                    inToken = false;
            }

            last = ptr + 1;
        }
        else if (!inToken)
            ++offset;
    }

    if (inToken) {
        syslog(LOG_ERR, "ERROR: graphlcd/skin/string: Expecting '}' in token");
        return false;
    }

    mText.append(last, ptr - last);

    if (mTranslate && !Translate && mText.length() > 0)
        Parse(Text, true);
    return true;
}

cType cSkinString::Evaluate(void) const
{
    if (mText.length() == 0 && mTokens.size() == 1)
        return mSkin->Config().GetToken(mTokens[0]);

    std::string result_raw = "";
    int offset = 0;
    for (uint32_t i = 0; i < mTokens.size(); i++) {
        result_raw.append(mText.substr(offset, mTokens[i].Offset - offset) );
        result_raw.append(mSkin->Config().GetToken(mTokens[i]));
        offset = mTokens[i].Offset;
    }
    result_raw.append(mText.c_str() + offset);

    // replace variable placeholders (#VARNAME#) with corresponding values
    std::string result_trans = "";
    size_t idxstart = 0, idxend = 0;
    size_t pos = 0;
    while ( (idxstart=result_raw.find("#", idxstart)) != std::string::npos ) {
      result_trans.append(result_raw.substr(pos, idxstart-pos));
      idxend = result_raw.find("#", idxstart+1);
      cSkinVariable * variable = mSkin->GetVariable(result_raw.substr(idxstart+1, idxend-idxstart-1));
      if (variable) {
        std::string val = (std::string) variable->Value();

        // if value of variable contains token definions: reparse value
        if (val.find("{") != std::string::npos) {
          cSkinString *result = new cSkinString(Object(), false);
          if (result->Parse(val)) {
            val = (std::string) result->Evaluate();
          }
          delete result;
        }
        result_trans.append (val);
        //   syslog(LOG_ERR, "string variable %s", trans.c_str());
      }
      idxstart = idxend+1;
      pos = idxstart;
    }
    result_trans.append(result_raw.substr(pos));

    // re-evaluate resulting string
    if ((mText.size() > 0) && mText[0] != '#' && mObject != NULL ) {
        cSkinFunction *result = new cSkinFunction(mObject);
        if (result->Parse(result_trans, true))  {
            std::string result_rescan = (std::string)result->Evaluate();
            if (result_rescan != "")
                result_trans = result_rescan;            
        }
        delete result;
    }
    return result_trans;
}

} // end of namespace
