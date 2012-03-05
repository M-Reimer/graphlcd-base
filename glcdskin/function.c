/*
 * GraphLCD skin library
 *
 * function.c  -  skin functions class
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * based on text2skin
 *
 */

#include <syslog.h>

#include <string.h>

#include "skin.h"
#include "function.h"

namespace GLCD
{

static const char * Internals[] =
{
    "not", "and", "or", "equal", "eq", "gt",  "lt", "ge", "le", "ne", "file", "trans",
    "add", "sub", "mul", "div",
    "FontTotalWidth",
    "FontTotalHeight",
    "FontTotalAscent",
    "FontSpaceBetween",
    "FontLineHeight",
    "FontTextWidth",
    "FontTextHeight",
    "ImageWidth",
    "ImageHeight",
    "QueryFeature",
    NULL
};

cSkinFunction::cSkinFunction(cSkinObject *Parent)
:   mObject(Parent),
    mSkin(Parent->Skin()),
    mType(string),
    mString(mObject, false),
    mNumber(0),
    mNumParams(0)
{
}

cSkinFunction::cSkinFunction(const cSkinString & String)
:   mObject(String.Object()),
    mSkin(String.Skin()),
    mType(string),
    mString(String),
    mNumber(0),
    mNumParams(0)
{
}

cSkinFunction::cSkinFunction(const cSkinFunction & Src)
:   mObject(Src.mObject),
    mSkin(Src.mSkin),
    mType(Src.mType),
    mString(Src.mString),
    mNumber(Src.mNumber),
    mNumParams(Src.mNumParams)
{
    for (uint32_t i = 0; i < mNumParams; ++i)
        mParams[i] = new cSkinFunction(*Src.mParams[i]);
}

cSkinFunction::~cSkinFunction()
{
    for (uint32_t i = 0; i < mNumParams; ++i)
        delete mParams[i];
}

bool cSkinFunction::Parse(const std::string & Text, bool reparse)
{
    const char *text = Text.c_str();
    const char *ptr = text, *last = text;
    eType type = undefined_function;
    int inExpr = 0;
    cSkinFunction *expr;

    if (*ptr == '\'' || *ptr == '{')
    {
        // must be string
        if (strlen(ptr) < 2
            || (*ptr == '\'' && *(ptr + strlen(ptr) - 1) != '\'')
            || (*ptr == '{' && *(ptr + strlen(ptr) - 1) != '}'))
        {
            if (!reparse) // only log this error when not reparsing
                syslog(LOG_ERR, "ERROR: graphlcd/skin/function: Unmatched string end\n");
            return false;
        }

        std::string temp;
        if (*ptr == '\'')
            temp.assign(ptr + 1, strlen(ptr) - 2);
        else
            temp.assign(ptr);

        int pos = -1;
        while ((pos = temp.find("\\'", pos + 1)) != -1)
            temp.replace(pos, 2, "'");

        if (!mString.Parse(temp))
            return false;

        mType = string;
    }
    if (*ptr == '#')
    {
        // must be a variable id
        if (strlen(ptr) < 2)
        {
            if (!reparse) // only log this error when not reparsing
                syslog(LOG_ERR, "ERROR: graphlcd/skin/function: No variable id given\n");
            return false;
        }

        mVariableId.assign(ptr + 1);
        mType = variable;
    }
    else if ((*ptr >= '0' && *ptr <= '9') || *ptr == '-' || *ptr == '+')
    {
        // must be number
        char *end;
        int num = strtol(ptr, &end, 10);
        if (end == ptr || *end != '\0')
        {
            // don't log this because when parsing a string starting with a digit (eg: 0%) this may result in a load of false positives
            //syslog(LOG_ERR, "ERROR: Invalid numeric value (%s)\n", Text.c_str());
            return false;
        }

        mNumber = num;
        mType = number;
    }
    else
    {
        bool inToken = false;

        // expression
        for (; *ptr; ++ptr)
        {

            if (*ptr == '{')
                inToken = true;
            else if (inToken && *ptr == '}')
                inToken = false;

            if (*ptr == '(')
            {
                if (inExpr++ == 0)
                {
                    int i;
                    for (i = 0; Internals[i] != NULL; ++i)
                    {
                        if ((size_t)(ptr - last) == strlen(Internals[i])
                            && memcmp(last, Internals[i], ptr - last) == 0)
                        {
                            type = (eType)(INTERNAL + i);
                            break;
                        }
                    }

                    if (Internals[i] == NULL)
                    {
                        if (!reparse) // only log this error when not reparsing
                            syslog(LOG_ERR, "ERROR: graphlcd/skin/function: Unknown function %.*s", (int)(ptr - last), last);
                        return false;
                    }
                    last = ptr + 1;
                }
            }
            else if ( ( (!inToken) && (*ptr == ',') ) || *ptr == ')')
            {
                if (inExpr == 0)
                {
                    if (!reparse) // only log this error when not reparsing
                        syslog(LOG_ERR, "ERROR: graphlcd/skin/function: Unmatched '%c' in expression (%s)", *ptr, Text.c_str());
                    return false;
                }

                if (inExpr == 1)
                {
                    expr = new cSkinFunction(mObject);
                    if (!expr->Parse(std::string(last, ptr - last)))
                    {
                        delete expr;
                        return false;
                    }

                    if (mNumParams == MAXPARAMETERS)
                    {
                        if (!reparse) // only log this error when not reparsing
                            syslog(LOG_ERR, "ERROR: graphlcd/skin/function: Too many parameters to function, maximum is %d",
                                   MAXPARAMETERS);
                        return false;
                    }

                    mType = type;
                    mParams[mNumParams++] = expr;
                    last = ptr + 1;
                }

                if (*ptr == ')')
                {
                    if (inExpr == 1)
                    {
                        int params = 0;

                        switch (mType)
                        {
                            case fun_and:
                            case fun_or:
                                params = -1;
                                break;

                            case fun_equal:
                            case fun_eq:
                            case fun_ne:
                            case fun_gt:
                            case fun_lt:
                            case fun_ge:
                            case fun_le:
                                params = 2;
                                break;

                            case fun_not:
                            case fun_trans:
                            case fun_file:
                                params = 1;
                                break;

                            case funAdd:
                            case funSub:
                            case funMul:
                                params = -1;
                                break;

                            case funDiv:
                                params = 2;
                                break;

                            case funFontTotalWidth:
                            case funFontTotalHeight:
                            case funFontTotalAscent:
                            case funFontSpaceBetween:
                            case funFontLineHeight:
                                params = 1;
                                break;

                            case funFontTextWidth:
                            case funFontTextHeight:
                                params = 2;
                                break;

                            case funImageWidth:
                            case funImageHeight:
                                params = 1;
                                break;

                            case funQueryFeature:
                                params = 1;
                                break;

                            default:
                                break;
                        }

                        if (params != -1 && mNumParams != (uint32_t) params)
                        {
                            syslog(LOG_ERR, "ERROR: graphlcd/skin/function: Wrong number of parameters to %s, "
                                    "expecting %d", Internals[mType - INTERNAL], params);
                            return false;
                        }
                    }

                    --inExpr;
                }
            }
        }

        if (inExpr > 0)
        {
            // only log this error when not reparsing
            if (!reparse)
                syslog(LOG_ERR, "ERROR: Expecting ')' in expression");
            return false;
        }
    }

    return true;
}

cType cSkinFunction::FunFile(const cType & Param) const
{
    cImageCache * cache = mSkin->ImageCache();
    GLCD::cImage * image = cache->Get(Param);
    return image ? (cType) Param : (cType) false;
}

cType cSkinFunction::FunFont(eType Function, const cType &Param1, const cType &Param2) const
{
    cSkinFont * skinFont = mSkin->GetFont(Param1);
    if (!skinFont)
        return false;

    const cFont * font = skinFont->Font();
    if (!font)
        return false;
    switch (Function)
    {
        case funFontTotalWidth:
            return font->TotalWidth();
        case funFontTotalHeight:
            return font->TotalHeight();
        case funFontTotalAscent:
            return font->TotalAscent();
        case funFontSpaceBetween:
            return font->SpaceBetween();
        case funFontLineHeight:
            return font->LineHeight();
        case funFontTextWidth:
            return font->Width((const std::string) Param2);
        case funFontTextHeight:
            return font->Height((const std::string) Param2);
        default:
            break;
    }
    return false;
}

cType cSkinFunction::FunImage(eType Function, const cType &Param) const
{
    cImageCache * cache = mSkin->ImageCache();
    GLCD::cImage * image = cache->Get(Param);

    if (!image)
        return false;

    switch (Function)
    {
        case funImageWidth:
            return (int) image->Width();
        case funImageHeight:
            return (int) image->Height();
        default:
            break;
    }
    return false;
}

cType cSkinFunction::Evaluate(void) const
{
    switch (mType)
    {
        case string:
            return mString.Evaluate();

        case number:
            return mNumber;

        case variable:
        {
            cSkinVariable * variable = mSkin->GetVariable(mVariableId);
            if (variable) {
                cType rv = variable->Value();
                if (rv.IsString()) {
                    std::string val = rv;
                    if (val.find("{") != std::string::npos || val.find("#") != std::string::npos) {
                        cSkinString *result = new cSkinString(mObject, false);
                        if (result->Parse(val)) {
                            val = (std::string) result->Evaluate();
                            rv = cType(val);
                        }
                        delete result;
                    }
                }
                return rv;
            }
            return false;
        }

        case fun_not:
            return !mParams[0]->Evaluate();

        case fun_and:
            for (uint32_t i = 0; i < mNumParams; ++i)
            {
                if (!mParams[i]->Evaluate())
                    return false;
            }
            return true;

        case fun_or:
            for (uint32_t i = 0; i < mNumParams; ++i)
            {
                if (mParams[i]->Evaluate())
                    return true;
            }
            return false;

        case fun_equal:
        case fun_eq:
            return mParams[0]->Evaluate() == mParams[1]->Evaluate();

        case fun_ne:
            return mParams[0]->Evaluate() != mParams[1]->Evaluate();

        case fun_gt:
            return mParams[0]->Evaluate() >  mParams[1]->Evaluate();

        case fun_lt:
            return mParams[0]->Evaluate() <  mParams[1]->Evaluate();

        case fun_ge:
            return mParams[0]->Evaluate() >= mParams[1]->Evaluate();

        case fun_le:
            return mParams[0]->Evaluate() <= mParams[1]->Evaluate();

        case fun_file:
            return FunFile(mParams[0]->Evaluate());

        case fun_trans:
            return mSkin->Config().Translate(mParams[0]->Evaluate());

        case funAdd:
        {
            int result = 0;
            for (uint32_t i = 0; i < mNumParams; ++i)
            {
                result += (int) mParams[i]->Evaluate();
            }
            return result;
        }

        case funSub:
            if (mNumParams > 0)
            {
                int result = (int) mParams[0]->Evaluate();
                for (uint32_t i = 1; i < mNumParams; ++i)
                {
                    result -= (int) mParams[i]->Evaluate();
                }
                return result;
            }
            return 0;

        case funMul:
        {
            int result = 1;
            for (uint32_t i = 0; i < mNumParams; ++i)
            {
                result *= (int) mParams[i]->Evaluate();
            }
            return result;
        }

        case funDiv:
            return ((int) mParams[0]->Evaluate() / (int) mParams[1]->Evaluate());

        case funFontTotalWidth:
        case funFontTotalHeight:
        case funFontTotalAscent:
        case funFontSpaceBetween:
        case funFontLineHeight:
            return FunFont(mType, mParams[0]->Evaluate(), "");

        case funFontTextWidth:
        case funFontTextHeight:
            return FunFont(mType, mParams[0]->Evaluate(), mParams[1]->Evaluate());

        case funImageWidth:
        case funImageHeight:
            return FunImage(mType, mParams[0]->Evaluate());

        case funQueryFeature: {
            int value;
            if (mSkin->Config().GetDriver()->GetFeature((const std::string)(mParams[0]->Evaluate()), value)) {
                return (value) ? true : false;
            } else {
                return false;
            }
        }

        default:
            //Dprintf("unknown function code\n");
            syslog(LOG_ERR, "ERROR: graphlcd/skin/function: Unknown function code called (this shouldn't happen)");
            break;
    }
    return false;
}

} // end of namespace
