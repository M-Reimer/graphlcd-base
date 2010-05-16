#include "display.h"
#include "object.h"
#include "skin.h"
#include "cache.h"
#include "function.h"
#include <sys/time.h>

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
    "block",
    "list",
    "item"
};

cSkinObject::cSkinObject(cSkinDisplay * Parent)
:   mDisplay(Parent),
    mSkin(Parent->Skin()),
    mType((eType) __COUNT_OBJECT__),
    mPos1(0, 0),
    mPos2(-1, -1),
    mColor(GLCD::clrBlack),
    mFilled(false),
    mRadius(0),
    mArc(0),
    mDirection(0),
    mAlign(taLeft),
    mMultiline(false),
    mPath(this, false),
    mCurrent(this, false),
    mTotal(this, false),
    mFont(this, false),
    mText(this, false),
    mCondition(NULL),
    mLastChange(0),
    mChangeDelay(-1),               // delay between two images frames: -1: not animated / don't care
    mStoredImagePath(""),
    mImageFrameId(0),               // start with 1st frame
    mScrollLoopMode(-1),            // scroll (text) or loop (image) mode: default (-1)
    mScrollLoopReached(false),      // if scroll/loop == once: already looped once?
    mScrollSpeed(0),                // scroll speed: default (0)
    mScrollTime(0),                 // scroll time interval: default (0)
    mScrollOffset(0),               // scroll offset (pixels)
    mCurrText(""),
    mObjects(NULL)
{
}

cSkinObject::cSkinObject(const cSkinObject & Src)
:   mDisplay(Src.mDisplay),
    mSkin(Src.mSkin),
    mType(Src.mType),
    mPos1(Src.mPos1),
    mPos2(Src.mPos2),
    mColor(Src.mColor),
    mFilled(Src.mFilled),
    mRadius(Src.mRadius),
    mArc(Src.mArc),
    mDirection(Src.mDirection),
    mAlign(Src.mAlign),
    mMultiline(Src.mMultiline),
    mPath(Src.mPath),
    mCurrent(Src.mCurrent),
    mTotal(Src.mTotal),
    mFont(Src.mFont),
    mText(Src.mText),
    mCondition(Src.mCondition),
    mLastChange(0),
    mChangeDelay(-1),
    mStoredImagePath(""),
    mImageFrameId(0),
    mScrollLoopMode(Src.mScrollLoopMode),
    mScrollLoopReached(Src.mScrollLoopReached),
    mScrollSpeed(Src.mScrollSpeed),
    mScrollTime(Src.mScrollTime),
    mScrollOffset(Src.mScrollOffset),
    mCurrText(Src.mCurrText),
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

bool cSkinObject::ParseColor(const std::string & Text)
{
    if (Text == "white")
        mColor = GLCD::clrWhite;
    else if (Text == "black")
        mColor = GLCD::clrBlack;
    else
        return false;
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
    return tPoint(mPos1.x < 0 ? mSkin->BaseSize().w + mPos1.x : mPos1.x,
                  mPos1.y < 0 ? mSkin->BaseSize().h + mPos1.y : mPos1.y);
}

tSize cSkinObject::Size(void) const
{
    tPoint p1(mPos1.x < 0 ? mSkin->BaseSize().w + mPos1.x : mPos1.x,
              mPos1.y < 0 ? mSkin->BaseSize().h + mPos1.y : mPos1.y);
    tPoint p2(mPos2.x < 0 ? mSkin->BaseSize().w + mPos2.x : mPos2.x,
              mPos2.y < 0 ? mSkin->BaseSize().h + mPos2.y : mPos2.y);
    return tSize(p2.x - p1.x + 1, p2.y - p1.y + 1);
}

void cSkinObject::Render(GLCD::cBitmap * screen)
{
    struct timeval tv;
    uint64_t timestamp;

    if (mCondition != NULL && !mCondition->Evaluate())
        return;

    gettimeofday(&tv, 0);
    timestamp = tv.tv_sec * 1000 + tv.tv_usec / 1000;

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

            GLCD::cImage * image = cache->Get(evalPath);
            if (image)
            {
                int framecount = image->Count();

                const GLCD::cBitmap * bitmap = image->GetBitmap(mImageFrameId);

                if (bitmap)
                {
                    screen->DrawBitmap(Pos().x, Pos().y, *bitmap, mColor);
                }

                if (mScrollLoopMode != -1)  // if == -1: currScrollLoopMode already contains correct value
                  currScrollLoopMode = mScrollLoopMode;

                if (framecount > 1 && currScrollLoopMode > 0 && !mScrollLoopReached) {
                    mChangeDelay = image->Delay();

                    if ( (int)(timestamp - mLastChange) >= mChangeDelay) {

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
            if (total == 0)
                total = 1;
            if (current > total)
                current = total;
            if (mDirection == 0)
            {
                int w = Size().w * current / total;
                if (w > 0)
                    screen->DrawRectangle(Pos().x, Pos().y, Pos().x + w - 1, Pos().y + Size().h - 1, mColor, true);
            }
            else if (mDirection == 1)
            {
                int h = Size().h * current / total;
                if (h > 0)
                    screen->DrawRectangle(Pos().x, Pos().y, Pos().x + Size().w - 1, Pos().y + h - 1, mColor, true);
            }
            else if (mDirection == 2)
            {
                int w = Size().w * current / total;
                if (w > 0)
                    screen->DrawRectangle(Pos().x + Size().w - w, Pos().y, Pos().x + Size().w - 1, Pos().y + Size().h - 1, mColor, true);
            }
            else if (mDirection == 3)
            {
                int h = Size().h * current / total;
                if (h > 0)
                    screen->DrawRectangle(Pos().x, Pos().y + Size().h - h, Pos().x + Size().w - 1, Pos().y + Size().h - 1, mColor, true);
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

            if (skinFont)
            {
                const cFont * font = skinFont->Font();
                std::string text = mText.Evaluate();

                if (! (text == mCurrText) ) {
                    mScrollOffset = 0;
                    mCurrText = text;
                    mScrollLoopReached = false;
                    mLastChange = timestamp;
                }

                if (mMultiline)
                {

                    // scrolling in multiline not supported at the moment
                    mScrollLoopReached = true;  // avoid check in NeedsUpdate()

                    std::vector <std::string> lines;
                    font->WrapText(Size().w, Size().h, text, lines);
                    for (size_t i = 0; i < lines.size(); i++)
                    {
                        int w = font->Width(lines[i]);
                        int x = Pos().x;
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
                        screen->DrawText(x, Pos().y + i * font->LineHeight(), x + Size().w - 1, lines[i], font, mColor);
                    }
                }
                else
                {
                    if (text.find('\t') != std::string::npos
                        && mSkin->Config().GetTabPosition(0, Size().w, *font) > 0)
                    {
                        // scrolling in texts with tabulators not supported at the moment
                        mScrollLoopReached = true;  // avoid check in NeedsUpdate()

                        std::string::size_type pos1;
                        std::string::size_type pos2;
                        std::string str;
                        int x = Pos().x;
                        int w = Size().w;
                        int tab = 0;
                        int tabWidth;

                        pos1 = 0;
                        pos2 = text.find('\t');
                        while (pos1 != std::string::npos && pos2 != std::string::npos)
                        {
                            str = text.substr(pos1, pos2 - pos1);
                            tabWidth = mSkin->Config().GetTabPosition(tab, Size().w, *font);
                            screen->DrawText(x, Pos().y, x + tabWidth - 1, str, font, mColor);
                            pos1 = pos2 + 1;
                            pos2 = text.find('\t', pos1);
                            tabWidth += font->Width(' ');
                            x += tabWidth;
                            w -= tabWidth;
                            tab++;
                        }
                        str = text.substr(pos1);
                        screen->DrawText(x, Pos().y, x + w - 1, str, font, mColor);
                    }
                    else
                    {
                        int w = font->Width(text);
                        int x = Pos().x;
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
                               ((int)(timestamp-mLastChange) >= currScrollTime)
                              )
                           {
                               if (mScrollLoopReached)
                                   mScrollOffset = 0;
                               else
                                   updateScroll = true;
                           }

                        }

                        if (mScrollOffset) {
                            w += font->Width("     ");
                            std::string textdoubled = text + "     " + text;
                            screen->DrawText(x, Pos().y, x + Size().w - 1, textdoubled, font, mColor, true, mScrollOffset);
                        } else {
                            screen->DrawText(x, Pos().y, x + Size().w - 1, text, font, mColor, true, mScrollOffset);
                        }

                        if (updateScroll) {
                            mScrollOffset += currScrollSpeed;

                            if ( x + Size().w + mScrollOffset > w+Size().w) {
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
            }
            break;
        }

//        case cSkinObject::scrolltext:
            //DrawScrolltext(Object->Pos(), Object->Size(), Object->Fg(), Object->Text(), Object->Font(),
            //    Object->Align());
//            break;

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
                        const cSkinObject * o = GetObject(j);
                        cSkinObject obj(*o);
                        obj.SetListIndex(maxitems, i);
                        if (obj.Condition() != NULL && !obj.Condition()->Evaluate())
                            continue;
                        obj.mPos1.x += mPos1.x;
                        obj.mPos1.y += mPos1.y + yoffset;
                        obj.mPos2.y += mPos1.y + yoffset;
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
                 ( (int)(CurrentTime-mLastChange) >= mChangeDelay)
               )
            {
              return true;
            }
            return false;
            break;
        }
        case cSkinObject::text:
        case cSkinObject::scrolltext:
        {
            int currScrollLoopMode = 1; // default values if no setup default values available
            int currScrollTime = 500;

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

            if (currScrollLoopMode > 0 && (!mScrollLoopReached || mScrollOffset) && 
                (int)(CurrentTime-mLastChange) >= currScrollTime
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


cSkinObjects::cSkinObjects(void)
{
}

cSkinObjects::~cSkinObjects()
{
    for (uint32_t i = 0; i < size(); i++)
        delete operator[](i);
}

} // end of namespace

