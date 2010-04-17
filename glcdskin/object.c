#include "display.h"
#include "object.h"
#include "skin.h"
#include "cache.h"
#include "function.h"

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
    if (mCondition != NULL && !mCondition->Evaluate())
        return;

    switch (Type())
    {
        case cSkinObject::image:
        {
            cImageCache * cache = mSkin->ImageCache();
            GLCD::cImage * image = cache->Get(mPath.Evaluate());
            if (image)
            {
                const GLCD::cBitmap * bitmap = image->GetBitmap();
                if (bitmap)
                {
                    screen->DrawBitmap(Pos().x, Pos().y, *bitmap, mColor);
                }
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
        {
            cSkinFont * skinFont = mSkin->GetFont(mFont.Evaluate());
            if (skinFont)
            {
                const cFont * font = skinFont->Font();
                std::string text = mText.Evaluate();
                if (mMultiline)
                {
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
                        screen->DrawText(x, Pos().y, x + Size().w - 1, text, font, mColor);
                    }
                }
            }
            break;
        }

        case cSkinObject::scrolltext:
            //DrawScrolltext(Object->Pos(), Object->Size(), Object->Fg(), Object->Text(), Object->Font(),
            //    Object->Align());
            break;

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

cSkinObjects::cSkinObjects(void)
{
}

cSkinObjects::~cSkinObjects()
{
    for (uint32_t i = 0; i < size(); i++)
        delete operator[](i);
}

} // end of namespace

