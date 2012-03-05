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

#include <glcddrivers/driver.h>

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

enum eTextVerticalAlignment
{
    tvaTop,
    tvaMiddle,
    tvaBottom
};

enum eEffect
{
    tfxNone,
    tfxShadow,
    tfxOutline
};

enum eScale
{
    tscNone,
    tscAuto,
    tscAutoX,
    tscAutoY,
    tscFill
};

enum eGradient
{
    tgrdNone,
    tgrdTotal,
    tgrdCurrent,
    tgrdVertical
};



class cSkinColor
{
private:
    cSkinObject *  mObject;
    uint32_t       mColor;
    std::string    mVarId;
public:
    cSkinColor(cSkinObject *Parent, uint32_t color):mVarId("") { mObject = Parent; mColor = color; }
    cSkinColor(cSkinObject *Parent, cColor color):mVarId("") { mObject = Parent; mColor = (uint32_t)color; }
    cSkinColor(cSkinObject *Parent, const std::string varId) { mObject = Parent; mVarId = varId; }
    ~cSkinColor() {};
    
    void SetColor(uint32_t color) { mVarId = ""; mColor = color; }
    void SetColor(cColor color) { mVarId = ""; mColor = (uint32_t)color; }
    void SetVarId(const std::string varId) { mVarId = varId; }
    
    uint32_t GetColor(void);

    operator uint32_t(void) { return GetColor(); }
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
        button,
        block,
        list,
        item,
#define __COUNT_OBJECT__ (item + 1)
    };

private:
    cSkinDisplay * mDisplay;        // parent display
    cSkin * mSkin;
    eType mType;                    // type of object, one of enum eType
    //tPoint mPos1;
    //tPoint mPos2;
    cSkinString mX1;                // either mX1 and mWidth or mX1 and mX2 are defined. not all three
    cSkinString mY1;
    cSkinString mX2;
    cSkinString mY2;
    cSkinString mWidth;
    cSkinString mHeight;
    cSkinColor mColor;
    cSkinColor mBackgroundColor;
    bool mFilled;
    int mRadius;
    int mArc;
    int mDirection;
    eTextAlignment mAlign;
    eTextVerticalAlignment mVerticalAlign;
    bool mMultiline;
    cSkinString mPath;
    cSkinString mCurrent;           // progress bar: current value
    cSkinString mTotal;             // progress bar: maximum valid value
    cSkinString mPeak;              // progress bar: peak value (<= 0: disabled)
    cSkinString mFont;
    cSkinString mText;
    cSkinFunction * mCondition;
    eEffect mEffect;                // effect: none, shadow, or outline
    cSkinColor mEffectColor;        // effect colour (= shadow colour or colour of outline)
    cSkinColor mPeakGradientColor;  // colour of peak marker or gradient color (mutual exclusive)
    eGradient mGradient;            // use gradient effect for progress bar (overrules peak!)

    uint64_t mLastChange;           // timestamp: last change in dynamic object (scroll, frame change, ...)
    int mChangeDelay;               // delay between two changes (frame change, scrolling, ...)
                                    // special values: -2: no further looping (mScrollLoopMode == 'once')
                                    //                 -1: not set (ie: not an animated image)

    std::string mStoredImagePath;   // stored image path
    int  mImageFrameId;             // frame ID of image
    int  mOpacity;                  // opacity of an image ([0, 255], default 255)
    eScale mScale;                  // image scaling (['none', 'autox', 'autoy', 'fill'], default: none)

    int mScrollLoopMode;            // scroll (text) or loop (image) mode: -1: default, 0: never, 1: once, 2: always
    bool mScrollLoopReached;        // if scroll/loop == once: already looped once?
    int mScrollSpeed;               // scroll speed: 0: default, [1 - 10]: speed
    int mScrollTime;                // scroll time interval: 0: default, [100 - 2000]: time interval
    int mScrollOffset;              // scroll offset (pixels)
    std::string mCurrText;          // current text (for checks if text has changed)

    std::string     mAltText;       // alternative text source for text-objects
    cSkinFunction * mAltCondition;  // condition when alternative sources are used
    std::string     mAction;        // action attached to this object

    int mMultilineScrollPosition;   // current scolling position of mMultiline
    cSkinString mMultilineRelScroll;// relative scrolling amount of mMultiline (default: 0)

    cSkinObjects * mObjects;        // used for block objects such as <list>

public:
    cSkinObject(cSkinDisplay * parent);
    cSkinObject(const cSkinObject & Src);
    ~cSkinObject();

    bool ParseType(const std::string &Text);
    bool ParseColor(const std::string &Text, cSkinColor & ParamColor);
    bool ParseCondition(const std::string &Text);
    bool ParseAlignment(const std::string &Text);
    bool ParseVerticalAlignment(const std::string &Text);
    bool ParseEffect(const std::string &Text);
    bool ParseScale(const std::string &Text);
    bool ParseGradient(const std::string &Text);
    bool ParseFontFace(const std::string &Text);
    bool ParseIntParam(const std::string &Text, int & Param);
    //bool ParseWidth(const std::string &Text);
    //bool ParseHeight(const std::string &Text);

    bool ParseScrollLoopMode(const std::string & Text); // parse scroll mode ([never|once|always])
    bool ParseScrollSpeed(const std::string & Text);    // parse scroll speed
    bool ParseScrollTime(const std::string & Text);     // parse scroll time interval

    bool ParseAltCondition(const std::string &Text);    // parse condition for alternative use (eg. alternative sources)

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

    // check if update is required for dynamic objects (image, text, progress, pane)
    // false: no update required, true: update required
    bool NeedsUpdate(uint64_t CurrentTime);

    std::string CheckAction(cGLCDEvent * ev);
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
