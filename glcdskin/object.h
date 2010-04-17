/*
 * GraphLCD skin library
 *
 * object.h  -  skin object class
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * based on text2skin
 *
 */

#ifndef _GLCDSKIN_OBJECT_H_
#define _GLCDSKIN_OBJECT_H_

#include <stdint.h>

#include <vector>
#include <string>
#include <map>

#include <glcdgraphics/bitmap.h>

#include "type.h"
#include "string.h"

namespace GLCD
{

class cSkin;
class cSkinDisplay;
class cSkinObjects;
class cSkinFunction;

struct tPoint
{
    int x, y;
    tPoint(int _x = 0, int _y = 0) { x = _x; y = _y; }
};

struct tSize
{
    int w, h;
    tSize(int _w = 0, int _h = 0) { w = _w; h = _h; }
};

enum eTextAlignment
{
    taCenter,
    taLeft,
    taRight
};

class cSkinObject
{
    friend bool StartElem(const std::string & name, std::map<std::string,std::string> & attrs);
    friend bool CharData(const std::string & text);
    friend bool EndElem(const std::string & name);

public:
    enum eType
    {
        pixel,
        line,
        rectangle,
        ellipse,
        slope,
        image,
        progress,
        text,
        scrolltext,
        scrollbar,
        block,
        list,
        item,
#define __COUNT_OBJECT__ (item + 1)
    };

private:
    cSkinDisplay * mDisplay;
    cSkin * mSkin;
    eType mType;
    tPoint mPos1;
    tPoint mPos2;
    eColor mColor;
    bool mFilled;
    int mRadius;
    int mArc;
    int mDirection;
    eTextAlignment mAlign;
    bool mMultiline;
    cSkinString mPath;
    cSkinString mCurrent;
    cSkinString mTotal;
    cSkinString mFont;
    cSkinString mText;
    cSkinFunction * mCondition;

    cSkinObjects * mObjects; // used for block objects such as <list>

public:
    cSkinObject(cSkinDisplay * parent);
    cSkinObject(const cSkinObject & Src);
    ~cSkinObject();

    bool ParseType(const std::string &Text);
    bool ParseColor(const std::string &Text);
    bool ParseCondition(const std::string &Text);
    bool ParseAlignment(const std::string &Text);
    bool ParseFontFace(const std::string &Text);
    bool ParseIntParam(const std::string &Text, int & Param);
    bool ParseWidth(const std::string &Text);
    bool ParseHeight(const std::string &Text);

    void SetListIndex(int MaxItems, int Index);

    eType Type(void) const { return mType; }
    cSkinFunction * Condition(void) const { return mCondition; }
	cSkinDisplay * Display(void) const { return mDisplay; }
    cSkin * Skin(void) const { return mSkin; }

    const std::string & TypeName(void) const;
    tPoint Pos(void) const;
    tSize Size(void) const;

    uint32_t NumObjects(void) const;
    cSkinObject * GetObject(uint32_t Index) const;

    void Render(cBitmap * screen);
};

class cSkinObjects: public std::vector<cSkinObject *>
{
public:
    cSkinObjects(void);
    ~cSkinObjects();
};

// recursive dependancy
inline uint32_t cSkinObject::NumObjects(void) const
{
    return mObjects ? mObjects->size() : 0;
}

inline cSkinObject * cSkinObject::GetObject(uint32_t Index) const
{
    return mObjects ? (*mObjects)[Index] : NULL;
}

} // end of namespace

#endif
