/*
 * GraphLCD skin library
 *
 * function.h  -  skin functions class
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * based on text2skin
 *
 */

#ifndef _GLCDSKIN_FUNCTION_H_
#define _GLCDSKIN_FUNCTION_H_

#include <stdint.h>

#include <string>

#include "type.h"
#include "string.h"

namespace GLCD
{

#define STRING    0x01000000
#define NUMBER    0x02000000
#define INTERNAL  0x04000000
#define VARIABLE  0x08000000

#define MAXPARAMETERS 512

class cSkinObject;
class cSkin;

class cSkinFunction
{
public:
    enum eType
    {
        undefined_function,

        string = STRING,
        number = NUMBER,
        variable = VARIABLE,

        fun_not = INTERNAL,
        fun_and,
        fun_or,
        fun_equal,
        fun_eq,
        fun_gt,
        fun_lt,
        fun_ge,
        fun_le,
        fun_ne,
        fun_file,
        fun_trans,

        funAdd,
        funSub,
        funMul,
        funDiv,

        funFontTotalWidth,
        funFontTotalHeight,
        funFontTotalAscent,
        funFontSpaceBetween,
        funFontLineHeight,
        funFontTextWidth,
        funFontTextHeight,

        funImageWidth,
        funImageHeight,

        funQueryFeature
    };

private:
    cSkinObject   * mObject;
    cSkin         * mSkin;
    eType           mType;
    cSkinString     mString;
    int             mNumber;
    std::string     mVariableId;
    cSkinFunction * mParams[MAXPARAMETERS];
    uint32_t        mNumParams;

protected:
    cType FunFile  (const cType &Param) const;
    cType FunPlugin(const cType &Param) const;
    cType FunFont  (eType Function, const cType &Param1, const cType &Param2) const;
    cType FunImage (eType Function, const cType &Param) const;
public:
    cSkinFunction(cSkinObject *Parent);
    cSkinFunction(const cSkinString &String);
    cSkinFunction(const cSkinFunction &Src);
    ~cSkinFunction();

    bool Parse(const std::string &Text, bool reparse = false);
    cType Evaluate(void) const;

    void SetListIndex(int MaxItems, int Index);
};

inline void cSkinFunction::SetListIndex(int MaxItems, int Index)
{
    mString.SetListIndex(MaxItems, Index);
    for (uint32_t i = 0; i < mNumParams; i++)
        mParams[i]->SetListIndex(MaxItems, Index);
}

} // end of namespace

#endif
