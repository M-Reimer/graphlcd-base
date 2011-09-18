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

#ifndef _GLCDSKIN_STRING_H_
#define _GLCDSKIN_STRING_H_

#include <string>
#include <vector>

#include "type.h"

namespace GLCD
{

enum eSkinAttrib
{
    aNone,
    aNumber,
    aString,
    aClean,
    aRest

#define __COUNT_ATTRIB__ (aRest + 1)
};

struct tSkinAttrib
{
    eSkinAttrib Type;
    std::string Text;
    int         Number;

    tSkinAttrib(const std::string & a): Type(aString), Text(a), Number(0) {}
    tSkinAttrib(int n): Type(aNumber), Text(""), Number(n) {}
    tSkinAttrib(eSkinAttrib t): Type(t), Text(""), Number(0) {}
    tSkinAttrib(void): Type(aNone), Text(""), Number(0) {}

    friend bool operator== (const tSkinAttrib & A, const tSkinAttrib & B);
    friend bool operator<  (const tSkinAttrib & A, const tSkinAttrib & B);
};

inline bool operator== (const tSkinAttrib & A, const tSkinAttrib & B)
{
    return A.Type == B.Type
        && A.Text == B.Text
        && A.Number == B.Number;
}

inline bool operator<  (const tSkinAttrib & A, const tSkinAttrib & B)
{
    return A.Type == B.Type
        ? A.Text == B.Text
           ? A.Number < B.Number
           : A.Text < B.Text
       : A.Type < B.Type;
}

struct tSkinToken
{
    int         Id;
    std::string Name;
    uint32_t    Offset;
    tSkinAttrib Attrib;
    int         MaxItems;
    int         Index;

    tSkinToken(void);
    tSkinToken(int id, std::string n, uint32_t o, const std::string & a);

    friend bool operator< (const tSkinToken & A, const tSkinToken & B);

    static std::string Token(const tSkinToken & Token);
};

class cSkinObject;
class cSkin;

class cSkinString
{
private:
#if 0
    typedef std::vector<cSkinString*> tStringList;
    static tStringList mStrings;
#endif

    cSkinObject *           mObject;
    cSkin *                 mSkin;
    std::string             mText;
    std::string             mOriginal;
    std::vector<tSkinToken> mTokens;
    bool                    mTranslate;

public:
#if 0
    static void Reparse(void);
#endif

    cSkinString(cSkinObject *Parent, bool Translate);
    ~cSkinString();

    bool Parse(const std::string & Text, bool Translate = false);
    cType Evaluate(void) const;

    void SetListIndex(int MaxItems, int Index);

    cSkinObject * Object(void) const { return mObject; }
    cSkin * Skin(void) const { return mSkin; }
};

inline void cSkinString::SetListIndex(int MaxItems, int Index)
{
    for (uint32_t i = 0; i < mTokens.size(); ++i)
    {
        mTokens[i].MaxItems = MaxItems;
        mTokens[i].Index = Index;
    }
}

} // end of namespace

#endif
