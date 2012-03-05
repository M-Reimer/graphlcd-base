#include "display.h"
#include "object.h"
#include "skin.h"
#include "cache.h"
#include "function.h"

#include <typeinfo>

namespace GLCD
{

static const std::string ObjectNames[] =
{
    "pixel",
    "line",
    "rectangle",
    "ellipse",
    "slope",
    "image",
    "progress",
    "text",
    "scrolltext",
    "scrollbar",
    "button",
    "block",
    "list",
    "item",
    "condblock"
};

cSkinObject::cSkinObject(cSkinDisplay * Parent)
:   mDisplay(Parent),
    mSkin(Parent->Skin()),
    mType((eType) __COUNT_OBJECT__),
    //mPos1(0, 0),
    //mPos2(-1, -1),
    mX1(this, false),
    mY1(this, false),
    mX2(this, false),
    mY2(this, false),
    mWidth(this, false),
    mHeight(this, false),
    mColor(this, cColor(cColor::Black)),
    mBackgroundColor(this, cColor(cColor::Transparent)),
    mFilled(false),
    mRadius(0),
    mArc(0),
    mDirection(0),
    mAlign(taLeft),
    mVerticalAlign(tvaTop),
    mMultiline(false),
    mPath(this, false),
    mCurrent(this, false),
    mTotal(this, false),
    mPeak(this, false),
    mFont(this, false),
    mText(this, false),
    mCondition(NULL),
    mEffect(tfxNone),
    mEffectColor(this, cColor(cColor::White)),
    mPeakGradientColor(this, cColor(cColor::ERRCOL)), // color for peak or gradient; if ERRCOL -> use mColor
    mGradient(tgrdNone),            // default: no gradient
    mLastChange(0),
    mChangeDelay(-1),               // delay between two images frames: -1: not animated / don't care
    mStoredImagePath(""),
    mImageFrameId(0),               // start with 1st frame
    mOpacity(255),                  // default: full opacity
    mScale(tscNone),                // scale image: default: don't scale
    mScrollLoopMode(-1),            // scroll (text) or loop (image) mode: default (-1)
    mScrollLoopReached(false),      // if scroll/loop == once: already looped once?
    mScrollSpeed(0),                // scroll speed: default (0)
    mScrollTime(0),                 // scroll time interval: default (0)
    mScrollOffset(0),               // scroll offset (pixels)
    mCurrText(""),                  // current text (for checks if text has changed)
    mAltText(""),                   // alternative text source for text-objects
    mAltCondition(NULL),            // condition when alternative sources are used
    mAction(""),                    // action (e.g. touchscreen action)
    mMultilineScrollPosition(0),
    mMultilineRelScroll(this, false),
    mObjects(NULL)
{
    mColor.SetColor(Parent->Skin()->Config().GetDriver()->GetForegroundColor());
    mBackgroundColor.SetColor(Parent->Skin()->Config().GetDriver()->GetBackgroundColor());
}

cSkinObject::cSkinObject(const cSkinObject & Src)
:   mDisplay(Src.mDisplay),
    mSkin(Src.mSkin),
    mType(Src.mType),
    //mPos1(Src.mPos1),
    //mPos2(Src.mPos2),
    mX1(Src.mX1),
    mY1(Src.mY1),
    mX2(Src.mX2),
    mY2(Src.mY2),
    mWidth(Src.mWidth),
    mHeight(Src.mHeight),
    mColor(Src.mColor),
    mBackgroundColor(Src.mBackgroundColor),
    mFilled(Src.mFilled),
    mRadius(Src.mRadius),
    mArc(Src.mArc),
    mDirection(Src.mDirection),
    mAlign(Src.mAlign),
    mVerticalAlign(Src.mVerticalAlign),
    mMultiline(Src.mMultiline),
    mPath(Src.mPath),
    mCurrent(Src.mCurrent),
    mTotal(Src.mTotal),
    mPeak(Src.mPeak),
    mFont(Src.mFont),
    mText(Src.mText),
    mCondition(Src.mCondition),
    mEffect(Src.mEffect),
    mEffectColor(Src.mEffectColor),
    mPeakGradientColor(Src.mPeakGradientColor),
    mGradient(Src.mGradient),
    mLastChange(0),
    mChangeDelay(-1),
    mStoredImagePath(Src.mStoredImagePath),
    mImageFrameId(0),
    mOpacity(Src.mOpacity),
    mScale(Src.mScale),
    mScrollLoopMode(Src.mScrollLoopMode),
    mScrollLoopReached(Src.mScrollLoopReached),
    mScrollSpeed(Src.mScrollSpeed),
    mScrollTime(Src.mScrollTime),
    mScrollOffset(Src.mScrollOffset),
    mCurrText(Src.mCurrText),
    mAltText(Src.mAltText),
    mAltCondition(Src.mAltCondition),
    mAction(Src.mAction),
    mMultilineScrollPosition(Src.mMultilineScrollPosition),
    mMultilineRelScroll(Src.mMultilineRelScroll),
    mObjects(NULL)
{
    if (Src.mObjects)
        mObjects = new cSkinObjects(*Src.mObjects);
}

cSkinObject::~cSkinObject()
{
    delete mObjects;
}

bool cSkinObject::ParseType(const std::string & Text)
{
    for (int i = 0; i < (int) __COUNT_OBJECT__; ++i)
    {
        if (ObjectNames[i] == Text)
        {
            mType = (eType) i;
            return true;
        }
    }
    return false;
}

bool cSkinObject::ParseColor(const std::string & Text, cSkinColor & ParamColor)
{
    std::string text = (std::string) Text;
    cColor color = cColor::ERRCOL;
    if (text[0] == '#') {
        cSkinVariable * variable = mSkin->GetVariable(text.substr(1));
        if (variable) {
            color = cColor::ParseColor(variable->Value().String());
            if (color == cColor::ERRCOL) {
                return false;
            }
            ParamColor.SetVarId(text.substr(1));
            return true;
        }
        return false;
    }
    color = cColor::ParseColor(text);
    if (color == cColor::ERRCOL) {
        return false;
    }
    ParamColor.SetColor(color);
    return true;
}

bool cSkinObject::ParseCondition(const std::string & Text)
{
    cSkinFunction *result = new cSkinFunction(this);
    if (result->Parse(Text))
    {
        delete mCondition;
        mCondition = result;
        return true;
    }
    return false;
}

bool cSkinObject::ParseAlignment(const std::string & Text)
{
    if (Text == "left")
        mAlign = taLeft;
    else if (Text == "right")
        mAlign = taRight;
    else if (Text == "center")
        mAlign = taCenter;
    else
        return false;
    return true;
}

bool cSkinObject::ParseVerticalAlignment(const std::string & Text)
{
    if (Text == "top")
        mVerticalAlign = tvaTop;
    else if (Text == "middle")
        mVerticalAlign = tvaMiddle;
    else if (Text == "bottom")
        mVerticalAlign = tvaBottom;
    else
        return false;
    return true;
}

bool cSkinObject::ParseEffect(const std::string & Text)
{
    if (Text == "none")
        mEffect = tfxNone;
    else if (Text == "shadow")
        mEffect = tfxShadow;
    else if (Text == "outline")
        mEffect = tfxOutline;
    else
        return false;
    return true;
}

bool cSkinObject::ParseScale(const std::string & Text)
{
    if (Text == "none")
        mScale = tscNone;
    else if (Text == "auto")
        mScale = tscAuto;
    else if (Text == "autox")
        mScale = tscAutoX;
    else if (Text == "autoy")
        mScale = tscAutoY;
    else if (Text == "fill")
        mScale = tscFill;
    else
        return false;
    return true;
}

bool cSkinObject::ParseGradient(const std::string & Text)
{
    if (Text == "none")
        mGradient = tgrdNone;
    else if (Text == "total" || Text == "default")
        mGradient = tgrdTotal;
    else if (Text == "current" || Text == "currentonly")
        mGradient = tgrdCurrent;
    else if (Text == "vertical")
        mGradient = tgrdVertical;
    else
        return false;
    return true;
}

bool cSkinObject::ParseIntParam(const std::string &Text, int & Param)
{
    if (isalpha(Text[0]) || Text[0] == '#')
    {
        cSkinFunction * func = new cSkinFunction(this);
        if (func->Parse(Text))
        {
            Param = (int) func->Evaluate();
            delete func;
            return true;
        }
        delete func;
    }
    char * e;
    const char * t = Text.c_str();
    long l = strtol(t, &e, 10);
    if (e ==t || *e != '\0')
    {
      return false;
    }
    Param = l;
    return true;
}

#if 0
bool cSkinObject::ParseWidth(const std::string &Text)
{
    int w;
    if (!ParseIntParam(Text, w))
        return false;
    if (w > 0)
    {
        mPos2.x = mPos1.x + w - 1;
        return true;
    }
    return false;
}

bool cSkinObject::ParseHeight(const std::string &Text)
{
    int h;
    if (!ParseIntParam(Text, h))
        return false;
    if (h > 0)
    {
        mPos2.y = mPos1.y + h - 1;
        return true;
    }
    return false;
}
#endif

bool cSkinObject::ParseFontFace(const std::string & Text)
{
    return mFont.Parse(Text);
}


bool cSkinObject::ParseScrollLoopMode(const std::string & Text)
{
    if (Text == "never")
        mScrollLoopMode = 0;
    else if (Text == "once")
        mScrollLoopMode = 1;
    else if (Text == "always")
        mScrollLoopMode = 2;
    else
        return false;
    return true;
}

bool cSkinObject::ParseScrollSpeed(const std::string & Text)
{
    int val;
    if (!ParseIntParam(Text, val))
        return false;

    if (val < 0 || val > 10)
        return false;

    mScrollSpeed = val;
    return true;
}

bool cSkinObject::ParseScrollTime(const std::string & Text)
{
    int val;
    if (!ParseIntParam(Text, val))
        return false;

    if (val < 0 || val > 2000)
        return false;

    if (val > 0 && val < 100)
        val = 100;

    mScrollTime = val;
    return true;
}


bool cSkinObject::ParseAltCondition(const std::string & Text)
{
    cSkinFunction *result = new cSkinFunction(this);
    if (result->Parse(Text))
    {
        delete mAltCondition;
        mAltCondition = result;
        return true;
    }
    return false;
}


void cSkinObject::SetListIndex(int MaxItems, int Index)
{
    mText.SetListIndex(MaxItems, Index);
    mPath.SetListIndex(MaxItems, Index);
    if (mCondition != NULL)
        mCondition->SetListIndex(MaxItems, Index);
}

const std::string & cSkinObject::TypeName(void) const
{
    return ObjectNames[mType];
}

tPoint cSkinObject::Pos(void) const
{
    int x1 = mX1.Evaluate();
    int y1 = mY1.Evaluate();
    return tPoint(x1 < 0 ? mSkin->BaseSize().w + x1 : x1,
                  y1 < 0 ? mSkin->BaseSize().h + y1 : y1);
}

tSize cSkinObject::Size(void) const
{
    int x1 = mX1.Evaluate();
    int y1 = mY1.Evaluate();
    tPoint p1(x1 < 0 ? mSkin->BaseSize().w + x1 : x1,
              y1 < 0 ? mSkin->BaseSize().h + y1 : y1);

    int w  = mWidth.Evaluate();
    int h  = mHeight.Evaluate();

    int x2 = mX2.Evaluate();
    if (w != 0 && x2 == -1) {
        x2 = x1 + w - 1;
    }
    int y2 = mY2.Evaluate();
    if (h != 0 && y2 == -1) {
        y2 = y1 + h - 1;
    }

    tPoint p2((x2 < 0) ? mSkin->BaseSize().w + x2 : x2, 
              (y2 < 0) ? mSkin->BaseSize().h + y2 : y2);
    return tSize(p2.x - p1.x + 1, p2.y - p1.y + 1);
}

void cSkinObject::Render(GLCD::cBitmap * screen)
{
    uint64_t timestamp;

    if (mCondition != NULL && !mCondition->Evaluate())
        return;

    timestamp = mSkin->Config().Now();

    switch (Type())
    {
        case cSkinObject::image:
        {
            cImageCache * cache = mSkin->ImageCache();
            int currScrollLoopMode = 2;  //default if not configured in the skin: always

            cType evalPath = mPath.Evaluate();
            std::string currPath = evalPath;

            if (currPath != mStoredImagePath) {
                mImageFrameId = 0;
                mStoredImagePath = currPath;
                mScrollLoopReached = false;
                mLastChange = timestamp;
                mChangeDelay = -1;
            }

            uint16_t scalew = 0;
            uint16_t scaleh = 0;
            
            switch (mScale) {
                case tscAuto:
                    {
                        uint16_t w_temp = 0;
                        uint16_t h_temp = 0;
                        // get dimensions of unscaled image
                        GLCD::cImage * image = cache->Get(evalPath, w_temp, h_temp);
                        if (image) {
                            w_temp = image->Width();
                            h_temp = image->Height();
                            if (w_temp != Size().w || h_temp != Size().h) {
                                double fw = (double)Size().w / (double)w_temp;
                                double fh = (double)Size().h / (double)h_temp;
                                if (fw < fh) {
                                    scalew = Size().w;
                                } else {
                                    scaleh = Size().h;
                                }
                            }
                        }
                    }
                    break;
                case tscAutoX:
                    scalew = Size().w;
                    break;
                case tscAutoY:
                    scaleh = Size().h;
                    break;
                case tscFill:
                    scalew = Size().w;
                    scaleh = Size().h;
                    break;
                default:
                    scalew = 0;
                    scaleh = 0;
            }
            
            GLCD::cImage * image = cache->Get(evalPath, scalew, scaleh);
            if (image)
            {
                int framecount = image->Count();

                const GLCD::cBitmap * bitmap = image->GetBitmap(mImageFrameId);

                if (bitmap)
                {
                    uint16_t xoff = 0;
                    uint16_t yoff = 0;
                    if (scalew || scaleh) {
                        if (image->Width() < (uint16_t)Size().w) {
                            xoff = (Size().w - image->Width() ) / 2;
                        } else if (image->Height() < (uint16_t)Size().h) {
                            yoff = (Size().h - image->Height() ) / 2;
                        }
                    }

                    if (mColor == cColor::ERRCOL)
                        screen->DrawBitmap(Pos().x + xoff, Pos().y + yoff, *bitmap);
                    else
                        screen->DrawBitmap(Pos().x + xoff, Pos().y + yoff, *bitmap, mColor, mBackgroundColor, mOpacity);
                }

                if (mScrollLoopMode != -1)  // if == -1: currScrollLoopMode already contains correct value
                  currScrollLoopMode = mScrollLoopMode;

                if (framecount > 1 && currScrollLoopMode > 0 && !mScrollLoopReached) {
                    mChangeDelay = image->Delay();

                    if ( (uint32_t)(timestamp - mLastChange) >= (uint32_t)mChangeDelay) {

                        if (currScrollLoopMode == 1 && mImageFrameId+1 == framecount) {
                            mScrollLoopReached = true;  // stop looping and switch to 1st frame
                        }

                        mImageFrameId = (mImageFrameId+1) % framecount;
                        mLastChange = timestamp;
                    }
                }

                if (mLastChange == 0)
                    mLastChange = timestamp;
            }
            break;
        }

        case cSkinObject::pixel:
            screen->DrawPixel(Pos().x, Pos().y, mColor);
            break;

        case cSkinObject::line:
        {
            int x1 = Pos().x;
            int x2 = Pos().x + Size().w - 1;
            int y1 = Pos().y;
            int y2 = Pos().y + Size().h - 1;
            if (x1 == x2)
                screen->DrawVLine(x1, y1, y2, mColor);
            else if (y1 == y2)
                screen->DrawHLine(x1, y1, x2, mColor);
            else
                screen->DrawLine(x1, y1, x2, y2, mColor);
            break;
        }

        case cSkinObject::rectangle:
            if (mRadius == 0)
                screen->DrawRectangle(Pos().x, Pos().y, Pos().x + Size().w - 1, Pos().y + Size().h - 1, mColor, mFilled);
            else
                screen->DrawRoundRectangle(Pos().x, Pos().y, Pos().x + Size().w - 1, Pos().y + Size().h - 1, mColor, mFilled, mRadius);
            break;

        case cSkinObject::ellipse:
            screen->DrawEllipse(Pos().x, Pos().y, Pos().x + Size().w - 1, Pos().y + Size().h - 1, mColor, mFilled, mArc);
            break;

        case cSkinObject::slope:
            screen->DrawSlope(Pos().x, Pos().y, Pos().x + Size().w - 1, Pos().y + Size().h - 1, mColor, mArc);
            break;

        case cSkinObject::progress:
        {
            int current = mCurrent.Evaluate();
            int total = mTotal.Evaluate();
            int peak = mPeak.Evaluate();
            if (total == 0)
                total = 1;
            if (current > total)
                current = total;
            if (peak > total)
                peak = total;
            
            int maxSize = ( (mDirection % 2) == 0 ) ? Size().w : Size().h;
            int currSize = maxSize * current / total;

            int peakSize = 0;
            int peakBarSize = 2;
            uint32_t peakGradientColor = (mPeakGradientColor == cColor::ERRCOL) ? mColor : mPeakGradientColor;
            
            bool gradient = false;
            
            if (peakGradientColor != mColor) {
                if (mGradient != tgrdNone) {
                    gradient = true;
                } else if (peak > 0) {
                    peakSize =  maxSize * peak / total;
                    if (mRadius <= 0) {
                        peakBarSize = maxSize / 20;
                        if (peakBarSize < 2)
                            peakBarSize = 2;
                    } else {
                        peakBarSize = mRadius;
                    }
                    // at least peakBarSize of empty space between normal progress bar and peak marker. if too small: don't show peak marker
                    if (currSize + peakBarSize + (peakBarSize / 2) >= peakSize)
                        peakSize = 0;  // don't show at all
                }
            }

            if (! gradient) {
                if (mDirection == 0)
                {
                    if (currSize > 0)
                        screen->DrawRectangle(Pos().x               , Pos().y,
                                              Pos().x + currSize - 1, Pos().y + Size().h - 1, mColor, true);
                    if (peakSize > 0)
                        screen->DrawRectangle(Pos().x + peakSize-1                , Pos().y,
                                              Pos().x + peakSize-1 + peakBarSize-1, Pos().y + Size().h - 1, peakGradientColor, true);
                }
                else if (mDirection == 1)
                {
                    if (currSize > 0)
                        screen->DrawRectangle(Pos().x               , Pos().y,
                                              Pos().x + Size().w - 1, Pos().y + currSize - 1, mColor, true);
                    if (peakSize > 0)
                        screen->DrawRectangle(Pos().x               , Pos().y + peakSize-1,
                                              Pos().x + Size().w - 1, Pos().y + peakSize-1 + peakBarSize-1, peakGradientColor, true);
                }
                else if (mDirection == 2)
                {
                    if (currSize > 0)
                        screen->DrawRectangle(Pos().x + Size().w - currSize, Pos().y,
                                              Pos().x + Size().w - 1       , Pos().y + Size().h - 1, mColor, true);
                    if (peakSize > 0)
                        screen->DrawRectangle(Pos().x + Size().w + maxSize - peakSize     , Pos().y,
                                              Pos().x + maxSize - peakSize + peakBarSize-1, Pos().y + Size().h - 1, peakGradientColor, true);
                }
                else if (mDirection == 3)
                {
                    if (currSize > 0)
                        screen->DrawRectangle(Pos().x               , Pos().y + Size().h - currSize,
                                              Pos().x + Size().w - 1, Pos().y + Size().h - 1       , mColor, true);
                    if (peakSize > 0)
                        screen->DrawRectangle(Pos().x               , Pos().y + maxSize - peakSize,
                                              Pos().x + Size().w - 1, Pos().y + maxSize - peakSize + peakBarSize-1, peakGradientColor, true);
                }
            } else {
                if (currSize > 0) {
                    int     s_a  = (mColor &  0xFF000000) >> 24;
                    int     s_r  = (mColor &  0x00FF0000) >> 16;
                    int     s_g  = (mColor &  0x0000FF00) >>  8;
                    int     s_b  = (mColor &  0x000000FF)      ;
                    int delta_a  = ((peakGradientColor & 0xFF000000) >> 24) - s_a;
                    int delta_r  = ((peakGradientColor & 0x00FF0000) >> 16) - s_r;
                    int delta_g  = ((peakGradientColor & 0x0000FF00) >>  8) - s_g;
                    int delta_b  = ((peakGradientColor & 0x000000FF)      ) - s_b;
                    int c_a, c_r, c_g, c_b;
                    double fact;
                    uint32_t currCol;
                    int gradSize = 0;
                    switch (mGradient) {
                        case tgrdCurrent:  gradSize = currSize; break;
                        case tgrdVertical: gradSize = (mDirection % 2 == 0) ? Size().h : Size().w ; break;
                        default:           gradSize = maxSize;  break;
                    }

                    for (int i = 0; i < ((mGradient == tgrdVertical) ? gradSize : currSize); i++) {
                        fact = (double)i / (double)(gradSize - 1);
                        c_a = s_a + int( double(delta_a) * fact );
                        c_r = s_r + int( double(delta_r) * fact );
                        c_g = s_g + int( double(delta_g) * fact );
                        c_b = s_b + int( double(delta_b) * fact );
                        currCol = (c_a << 24) | (c_r << 16) | (c_g << 8) | c_b;
                        //fprintf(stderr, "i: %d / %08x -> %08x / currCol: %08x\n", i, (uint32_t)mColor, peakGradientColor, currCol);
                        if (mGradient == tgrdVertical) {
                            if (mDirection == 0)
                                screen->DrawLine(Pos().x,                Pos().y + i,
                                                 Pos().x + currSize - 1, Pos().y + i, currCol);
                            else if (mDirection == 2)
                                screen->DrawLine(Pos().x + Size().w - currSize, Pos().y + i,
                                                 Pos().x + Size().w - 1,        Pos().y + i, currCol);
                            else if (mDirection == 1)
                                screen->DrawLine(Pos().x + Size().w - 1 - i, Pos().y,
                                                 Pos().x + Size().w - 1 - i, Pos().y + currSize - 1, currCol);
                            else if (mDirection == 3)
                                screen->DrawLine(Pos().x + i, Pos().y + Size().h - currSize,
                                                 Pos().x + i, Pos().y + Size().h - 1       , currCol);
                        } else {
                            if (mDirection == 0)
                                screen->DrawLine(Pos().x + i, Pos().y,
                                                 Pos().x + i, Pos().y + Size().h - 1, currCol);
                            else if (mDirection == 2)
                                screen->DrawLine(Pos().x + Size().w - 1 - i, Pos().y,
                                                 Pos().x + Size().w - 1 - i, Pos().y + Size().h - 1, currCol);
                            else if (mDirection == 1)
                                screen->DrawLine(Pos().x               , Pos().y + i,
                                                 Pos().x + Size().w - 1, Pos().y + i, currCol);
                            else if (mDirection == 3)
                                screen->DrawLine(Pos().x               , Pos().y + Size().h - 1 - i,
                                                 Pos().x + Size().w - 1, Pos().y + Size().h - 1 - i , currCol);
                        }
                    }
                }
            }
            break;
        }

        case cSkinObject::text:
        case cSkinObject::scrolltext:
        {
            cSkinFont * skinFont = mSkin->GetFont(mFont.Evaluate());
            int currScrollLoopMode = 1; // default values if no setup default values available
            int currScrollSpeed = 8;
            int currScrollTime = 500;

            // get default values from derived config-class if available
            tSkinToken token = tSkinToken();
            token.Id = mSkin->Config().GetTokenId("ScrollMode");
            if (token.Id >= 0) {
                cType t = mSkin->Config().GetToken(token);
                currScrollLoopMode = (int)(t);
            }
            token.Id = mSkin->Config().GetTokenId("ScrollSpeed");
            if (token.Id >= 0) {
                cType t = mSkin->Config().GetToken(token);
                currScrollSpeed = (int)(t);
            }
            token.Id = mSkin->Config().GetTokenId("ScrollTime");
            if (token.Id >= 0) {
                cType t = mSkin->Config().GetToken(token);
                currScrollTime = (int)(t);
            }

            // amount of loops for effects (no effect: 1 loop)
            int loop;
            int loops = 1;
            int varx[6]  = {0, 0, 0, 0, 0, 0};
            int vary[6]  = {0, 0, 0, 0, 0, 0};
            uint32_t varcol[6] = { mColor, mColor, mColor, mColor, mColor, mColor };

            int fxOff = 1;
            if (mRadius > 1)
                fxOff = 2;
            
            switch (mEffect) {
                case tfxShadow:
                    loops = 1;
                    for (int fxi = 0; fxi < fxOff; fxi++) {
                      varx[fxi] =  fxi + 1;  vary[fxi] =  fxi + 1;
                      varcol[loops-1] = mEffectColor;
                      loops++;
                    }
                    varcol[loops-1] = cColor::Transparent;
                    loops++;
                break;
                case tfxOutline:
                    loops = 6;
                    varx[0] = -fxOff;  vary[0] =      0;
                    varx[1] =  fxOff;  vary[1] =      0;
                    varx[2] =      0;  vary[2] = -fxOff;
                    varx[3] =      0;  vary[3] =  fxOff;
                    varcol[0] = varcol[1] = varcol[2] = varcol[3] = mEffectColor;
                    varcol[4] = cColor::Transparent;
                break;
                case tfxNone:  // no-one gets forgotten here, so make g++ happy
                default:
                    loops = 1;
            }

            if (skinFont)
            {

                cBitmap* pane = new cBitmap(Size().w, Size().h, cColor::Transparent);
                pane->SetProcessAlpha(false);

                const cFont * font = skinFont->Font();
                std::string text = "";

                // is an alternative text defined + alternative condition defined and true?
                if (mAltCondition != NULL && mAltCondition->Evaluate() && (mAltText.size() != 0)) {
                    cSkinString *result = new cSkinString(this, false);

                    if (result->Parse(mAltText)) {
                        text = (std::string) result->Evaluate();
                    }
                    delete result;
                } else { // nope: use the original text
                    text = (std::string) mText.Evaluate();
                }

                if (! (text == mCurrText) ) {
                    mScrollOffset = 0;
                    mCurrText = text;
                    mScrollLoopReached = false;
                    mLastChange = timestamp;
                    mMultilineScrollPosition = 0;
                }

                if (mMultiline)
                {
                    // scrolling in multiline not supported at the moment
                    mScrollLoopReached = true;  // avoid check in NeedsUpdate()

                    std::vector <std::string> lines;
                    font->WrapText(Size().w, 0/*Size().h*/, text, lines);

                    size_t amount_lines = Size().h / font->LineHeight();

                    if (amount_lines < lines.size()) {
                        int multilineRelScroll = mMultilineRelScroll.Evaluate();
                        if (multilineRelScroll != 0) {
                            if (multilineRelScroll < 0) {
                                mMultilineScrollPosition += multilineRelScroll;
                                if (mMultilineScrollPosition < 0)
                                    mMultilineScrollPosition = 0;
                            } else if (multilineRelScroll > 0) {
                                mMultilineScrollPosition += multilineRelScroll;
                                if (mMultilineScrollPosition > (int)((lines.size() - amount_lines)) )
                                    mMultilineScrollPosition = lines.size() - amount_lines;
                            }
                        }
                    }

                    // vertical alignment, calculate y offset
                    int yoff = 0;
                    int diff = Size().h - lines.size() * font->LineHeight();
                    switch (mVerticalAlign) {
                        case tvaMiddle:
                            yoff = (diff > 0) ? diff >> 1 : 0;
                        break;
                        case tvaBottom:
                            yoff = (diff > 0) ? diff : 0;
                        break;
                        default: yoff = 0;
                    }

                    int end_line = amount_lines;
                    if (amount_lines > lines.size() )
                        end_line = lines.size();

                    for (size_t i = 0; i < (size_t)end_line; i++)
                    {
                        int w = font->Width(lines[i + mMultilineScrollPosition]);
                        int x = 0;
                        if (w < Size().w)
                        {
                            if (mAlign == taRight)
                            {
                                x += Size().w - w;
                            }
                            else if (mAlign == taCenter)
                            {
                                x += (Size().w - w) / 2;
                            }
                        }
                        for (loop = 0; loop < loops; loop++) {
                            pane->DrawText(
                                varx[loop] + x, vary[loop] + yoff + i * font->LineHeight(), 
                                x + Size().w - 1, lines[i + mMultilineScrollPosition], font, varcol[loop], mBackgroundColor
                            );
                        }
                    }
                }
                else
                {
                    // vertical alignment, calculate y offset
                    int yoff = 0;
                    int diff = Size().h - font->LineHeight();
                    switch (mVerticalAlign) {
                        case tvaMiddle:
                            yoff = (diff > 0) ? diff >> 1 : 0;
                        break;
                        case tvaBottom:
                            yoff = (diff > 0) ? diff : 0;
                        break;
                        default: yoff = 0;
                    }

                    if (text.find('\t') != std::string::npos
                        && mSkin->Config().GetTabPosition(0, Size().w, *font) > 0)
                    {
                        // scrolling in texts with tabulators not supported at the moment
                        mScrollLoopReached = true;  // avoid check in NeedsUpdate()

                        std::string::size_type pos1;
                        std::string::size_type pos2;
                        std::string str;
                        int x = 0;
                        int w = Size().w;
                        int tab = 0;
                        int tabWidth;

                        pos1 = 0;
                        pos2 = text.find('\t');
                        while (pos1 != std::string::npos && pos2 != std::string::npos)
                        {
                            str = text.substr(pos1, pos2 - pos1);
                            tabWidth = mSkin->Config().GetTabPosition(tab, Size().w, *font);
                            for (loop = 0; loop < loops; loop++) {
                                pane->DrawText( varx[loop] + x, vary[loop] + yoff, x + tabWidth - 1, str, font, varcol[loop], mBackgroundColor );
                            }
                            pos1 = pos2 + 1;
                            pos2 = text.find('\t', pos1);
                            tabWidth += font->Width(' ');
                            x += tabWidth;
                            w -= tabWidth;
                            tab++;
                        }
                        str = text.substr(pos1);
                        for (loop = 0; loop < loops; loop++) {
                            pane->DrawText( varx[loop] + x, vary[loop] + yoff, x + w - 1, str, font, varcol[loop], mBackgroundColor );
                        }
                    }
                    else
                    {
                        int w = font->Width(text);
                        int x = 0;
                        bool updateScroll = false;

                        if (w < Size().w)
                        {
                            mScrollLoopReached = true;  // avoid check in NeedsUpdate()

                            if (mAlign == taRight)
                            {
                                x += Size().w - w;
                            }
                            else if (mAlign == taCenter)
                            {
                                x += (Size().w - w) / 2;
                            }
                        } else {

                           if (mScrollLoopMode != -1)  // if == -1: currScrollLoopMode already contains correct value
                               currScrollLoopMode = mScrollLoopMode;

                           if (mScrollSpeed > 0)
                               currScrollSpeed = mScrollSpeed;

                           if (mScrollTime > 0)
                               currScrollTime = mScrollTime;

                           if (currScrollLoopMode > 0 && (!mScrollLoopReached || mScrollOffset) && 
                               ((uint32_t)(timestamp-mLastChange) >= (uint32_t)currScrollTime)
                              )
                           {
                               if (mScrollLoopReached)
                                   mScrollOffset = 0;
                               else
                                   updateScroll = true;
                           }

                        }

                        if (mScrollOffset) {
                            int corr_scrolloffset = mScrollOffset;
                            /* object update before scrolltime? use previous offset to avoid 'stumbling' scrolling */
                            if ((uint32_t)(timestamp-mLastChange) < (uint32_t)currScrollTime) {
                                corr_scrolloffset -= currScrollSpeed;
                                if (corr_scrolloffset < 0)
                                    corr_scrolloffset = 0;
                            }
                            w += font->Width("     ");
                            std::string textdoubled = text + "     " + text;
                            for (loop = 0; loop < loops; loop++) {
                                pane->DrawText(
                                    varx[loop] + x, vary[loop] + yoff, x + Size().w - 1, textdoubled, font,
                                    varcol[loop], mBackgroundColor, true, corr_scrolloffset
                                );
                            }
                        } else {
                            for (loop = 0; loop < loops; loop++) {
                                pane->DrawText(
                                    varx[loop] + x, vary[loop] + yoff, x + Size().w - 1, text, font,
                                    varcol[loop], mBackgroundColor, true, mScrollOffset
                                );
                            }
                        }

                        if (updateScroll) {
                            mScrollOffset += currScrollSpeed;                            
                            if ( mScrollOffset >= w ) {
                                if (currScrollLoopMode == 1)
                                    // reset mScrollOffset in next step (else: string not redrawn when scroll done)
                                    mScrollLoopReached = true;
                                else
                                    mScrollOffset= 0;
                            }
                            updateScroll = false;
                            mLastChange = timestamp;
                        }
                    }
                }
                screen->DrawBitmap(Pos().x, Pos().y, *pane, cColor::White, cColor::Transparent);
                delete pane;
            }
            break;
        }

//        case cSkinObject::scrolltext:
            //DrawScrolltext(Object->Pos(), Object->Size(), Object->Fg(), Object->Text(), Object->Font(),
            //    Object->Align());
//            break;

        case cSkinObject::button:
        {
            cSkinFont * skinFont = mSkin->GetFont(mFont.Evaluate());

            if (mBackgroundColor == mColor || mBackgroundColor == cColor::Transparent)
                mBackgroundColor.SetColor( (cColor(mColor).Invert()) );

            if (mRadius == 0)
                screen->DrawRectangle(Pos().x, Pos().y, Pos().x + Size().w - 1, Pos().y + Size().h - 1, mBackgroundColor, true);
            else
                screen->DrawRoundRectangle(Pos().x, Pos().y, Pos().x + Size().w - 1, Pos().y + Size().h - 1, mBackgroundColor, true, mRadius);

            if (skinFont)
            {
                const cFont * font = skinFont->Font();
                std::string text = "";

                text = (std::string) mText.Evaluate();

                if (! (text == mCurrText) ) {
                    mCurrText = text;
                }
                std::vector <std::string> lines;
                font->WrapText(Size().w, Size().h, text, lines);

                // always use middle vertical alignment for buttons
                int diff = Size().h - lines.size() * font->LineHeight();
                int yoff = (diff > 0) ? diff >> 1 : 0;

                int w = font->Width(text);
                int x = Pos().x;
                if (w < Size().w) // always center alignment for buttons
                    x += (Size().w - w) / 2;
                screen->DrawText(x, yoff + Pos().y, x + Size().w - 1, text, font, mColor, mBackgroundColor);
            }
            break;
        }

        case cSkinObject::scrollbar:
            //DrawScrollbar(Object->Pos(), Object->Size(), Object->Bg(), Object->Fg());
            break;

        case cSkinObject::block:
            for (uint32_t i = 0; i < NumObjects(); i++)
                GetObject(i)->Render(screen);
            break;

        case cSkinObject::list:
        {
            const cSkinObject * item = GetObject(0);
            if (item && item->Type() == cSkinObject::item)
            {
                int itemheight = item->Size().h;
                int maxitems = Size().h / itemheight;
                int yoffset = 0;

                for (int i = 0; i < maxitems; i++)
                {
                    for (int j = 1; j < (int) NumObjects(); j++)
                    {
                        int px, py, ph,pw;
                        char buf[10];
                        const cSkinObject * o = GetObject(j);
                        cSkinObject obj(*o);
                        obj.SetListIndex(maxitems, i);
                        if (obj.Condition() != NULL && !obj.Condition()->Evaluate())
                            continue;
                        px = obj.Pos().x + Pos().x;                     // obj.mPos1.x += mPos1.x;
                        py = obj.Pos().y + Pos().y + yoffset;           // obj.mPos1.y += mPos1.y + yoffset;
                        ph = o->Size().h;                               // obj.mPos2.y += mPos1.y + yoffset;
                        pw = o->Size().w;
                        snprintf(buf, 9, "%d", px); obj.mX1.Parse((const char*)buf);
                        snprintf(buf, 9, "%d", py); obj.mY1.Parse((const char*)buf);
                        if (ph > 0)
                          snprintf(buf, 9, "%d", ph); obj.mHeight.Parse((const char*)buf);
                        snprintf(buf, 9, "%d", pw); obj.mWidth.Parse((const char*)buf);
                        obj.Render(screen);
                    }
                    yoffset += itemheight;
                }
            }
            break;
        }

        case cSkinObject::item:
            // ignore
            break;
    }
}

bool cSkinObject::NeedsUpdate(uint64_t CurrentTime)
{
    if (mCondition != NULL && !mCondition->Evaluate())
        return false;

    switch (Type())
    {
        case cSkinObject::image:
        {
            int currScrollLoopMode = 2;

            if (mScrollLoopMode != -1)
                currScrollLoopMode = mScrollLoopMode;

            if ( mChangeDelay > 0 && currScrollLoopMode > 0 && !mScrollLoopReached && 
                 ( (uint32_t)(CurrentTime-mLastChange) >= (uint32_t)mChangeDelay)
               )
            {
              return true;
            }
            return false;
            break;
        }
        case cSkinObject::text:
        case cSkinObject::scrolltext:
        //case cSkinObject::button:
        {
            int currScrollLoopMode = 1; // default values if no setup default values available
            int currScrollTime = 500;

            std::string text = "";

            // is an alternative text defined + alternative condition defined and true?
            if (mAltCondition != NULL && mAltCondition->Evaluate() && (mAltText.size() != 0)) {
                cSkinString *result = new cSkinString(this, false);

                if (result->Parse(mAltText)) {
                    text = (std::string) result->Evaluate();
                }
                delete result;
            } else { // nope: use the original text
                text = (std::string) mText.Evaluate();
            }

            // get default values from derived config-class if available
            tSkinToken token = tSkinToken();
            token.Id = mSkin->Config().GetTokenId("ScrollMode");
            if (token.Id >= 0) {
                cType t = mSkin->Config().GetToken(token);
                currScrollLoopMode = (int)(t);
            }
            token.Id = mSkin->Config().GetTokenId("ScrollTime");
            if (token.Id >= 0) {
                cType t = mSkin->Config().GetToken(token);
                currScrollTime = (int)(t);
            }

            if (mScrollLoopMode != -1)  // if == -1: currScrollLoopMode already contains correct value
                currScrollLoopMode = mScrollLoopMode;

            if (mScrollTime > 0)
                currScrollTime = mScrollTime;

            if ( (text != mCurrText) || 
                 ( (currScrollLoopMode > 0) && (!mScrollLoopReached || mScrollOffset) && 
                   ((uint32_t)(CurrentTime-mLastChange) >= (uint32_t)currScrollTime)
                 )
               )
            {
                return true;
            }
            return false;
            break;
        }
        case cSkinObject::progress:
            return false;
            break;
        case cSkinObject::block:
        {
            for (uint32_t i = 0; i < NumObjects(); i++) {
                if ( GetObject(i)->NeedsUpdate(CurrentTime) ) {
                    return true;
                }
            }
            return false;
            break;
        }
        default:  // all other elements are static ones
            return false;
    }
    return false;
}


std::string cSkinObject::CheckAction(cGLCDEvent * ev)
{
    if (mCondition != NULL && !mCondition->Evaluate())
        return "";

    switch (Type())
    {
        case cSkinObject::image:
        case cSkinObject::text:
        case cSkinObject::scrolltext:
        case cSkinObject::progress:
        case cSkinObject::rectangle:
        case cSkinObject::ellipse:
        case cSkinObject::slope:
        case cSkinObject::button:
        case cSkinObject::item:
        {
            if (mAction == "")
                return "";

            if (ev && (typeid(*ev) == typeid(cSimpleTouchEvent))) {
                cSimpleTouchEvent * stev = (cSimpleTouchEvent*)ev;
                // check if touch event is in bounding box of object
                // uses   >   and  <  -1  instead of  >=  and <  -0   for better results
                if ( (stev->x > Pos().x) && (stev->x < (Pos().x+Size().w -1)) && 
                     (stev->y > Pos().y) && (stev->y < (Pos().y+Size().h -1)) 
                   )
                {
                    return mAction;
                }
            }
            return "";
            break;
        }
        case cSkinObject::block:
        {
            std::string rv = "";
            for (uint32_t i = 0; i < NumObjects(); i++) {
                if ( (rv = GetObject(i)->CheckAction(ev)) != "" ) {
                    return rv;
                }
            }
            return "";
            break;
        }
        default:
            return "";
    }
    return "";
}



uint32_t cSkinColor::GetColor(void) {
    if (mVarId != "") {
        cSkinVariable * variable = mObject->Skin()->GetVariable(mVarId);
        if (variable) {
            return cColor::ParseColor(variable->Value().String());
        }
        return cColor::ERRCOL;
    }
    return (uint32_t) mColor;
}


cSkinObjects::cSkinObjects(void)
{
}

cSkinObjects::~cSkinObjects()
{
    for (uint32_t i = 0; i < size(); i++)
        delete operator[](i);
}

} // end of namespace

