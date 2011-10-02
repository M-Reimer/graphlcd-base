/*
 * GraphLCD skin library
 *
 * type.h  -  skin type class
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * based on text2skin
 *
 */

#ifndef _GLCDSKIN_TYPE_H_
#define _GLCDSKIN_TYPE_H_

#include <string>
#include <stdio.h>

#include <stdint.h>
#include <stdlib.h>

namespace GLCD
{

class cType
{
public:
    enum eType
    {
        string,
        number,
        boolean
    };

    //friend class cxFunction;

private:
    eType       mType;
    std::string mString;
    int         mNumber;
    uint32_t    mUpdateIn;

public:
    cType(void): mType(boolean), mNumber(0), mUpdateIn(0) {}
    cType(const char *String): mType(string), mString(String ?: ""), mUpdateIn(0) {}
    cType(std::string String): mType(string), mString(String), mUpdateIn(0) {}
    cType(int Number): mType(number), mNumber(Number), mUpdateIn(0) {}
    cType(time_t Number): mType(number), mNumber(Number), mUpdateIn(0) {}
    cType(bool Value): mType(boolean), mNumber(Value ? 1 : 0), mUpdateIn(0) {}

    std::string String(void) const;
    int Number(void) const { return mType == number ? mNumber : atoi(mString.c_str()); }

    void SetUpdate(uint32_t UpdateIn) { mUpdateIn = UpdateIn; }
    uint32_t UpdateIn(void) const { return mUpdateIn; }

    bool IsString(void)  const { return (mType == string); }
    bool IsNumber(void)  const { return (mType == number); }
    bool IsBoolean(void) const { return (mType == boolean); }

    operator std::string () const { return String(); }
    operator int         () const { return Number(); }
    operator bool        () const;

    friend bool operator== (const cType &a, const cType &b);
    friend bool operator!= (const cType &a, const cType &b);
    friend bool operator<  (const cType &a, const cType &b);
    friend bool operator>  (const cType &a, const cType &b);
    friend bool operator<= (const cType &a, const cType &b);
    friend bool operator>= (const cType &a, const cType &b);
};

inline std::string cType::String(void) const
{
    char str[16];

    switch (mType)
    {
        case number:
            sprintf(str, "%d", mNumber);
            return str;
        case boolean:
            return mNumber != 0 ? "1" : "";
        default:
            return mString;
    }
}

inline cType::operator bool () const
{
    switch (mType)
    {
        case string:
            return mString != "";
        default:
            return mNumber != 0;
    }
}

inline bool operator== (const cType &a, const cType &b)
{
    if (a.mType == cType::string || b.mType == cType::string)
        return a.String() == b.String();
    return a.mNumber == b.mNumber;
}

inline bool operator!= (const cType &a, const cType &b)
{
    if (a.mType == cType::string || b.mType == cType::string)
        return a.String() != b.String();
    return a.mNumber != b.mNumber;
}

inline bool operator< (const cType &a, const cType &b)
{
    if (a.mType == cType::string || b.mType == cType::string)
        return a.String() < b.String();
    return a.mNumber < b.mNumber;
}

inline bool operator> (const cType &a, const cType &b)
{
    if (a.mType == cType::string || b.mType == cType::string)
        return a.String() > b.String();
    return a.mNumber > b.mNumber;
}

inline bool operator<= (const cType &a, const cType &b)
{
    if (a.mType == cType::string || b.mType == cType::string)
        return a.String() <= b.String();
    return a.mNumber <= b.mNumber;
}

inline bool operator>= (const cType &a, const cType &b)
{
    if (a.mType == cType::string || b.mType == cType::string)
        return a.String() >= b.String();
    return a.mNumber >= b.mNumber;
}

} // end of namespace

#endif
