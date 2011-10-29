/*
 * GraphLCD skin library
 *
 * variable.h  -  variable class
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * based on text2skin
 *
 */

#ifndef _GLCDSKIN_VARIABLE_H_
#define _GLCDSKIN_VARIABLE_H_

#include <string>
#include <vector>

#include "display.h"
#include "object.h"
#include "function.h"

namespace GLCD
{

enum eEvalMode
{
    tevmAlways,
    tevmTick,
    tevmSwitch,
    tevmOnce,
    tevmInterval
};


class cSkin;

class cSkinVariable
{
    friend bool StartElem(const std::string &name, std::map<std::string,std::string> &attrs);
    friend bool EndElem(const std::string &name);

private:
    cSkin * mSkin;
    std::string mId;
    cType mValue;
    cSkinFunction * mCondition;
    cSkinFunction * mFunction;
    cSkinDisplay mDummyDisplay;
    cSkinObject mDummyObject;
    eEvalMode mEvalMode;
    int mEvalInterval;
    uint64_t mTimestamp;

public:
    cSkinVariable(cSkin * Parent);

    bool ParseEvalMode(const std::string & Text);
    bool ParseValue(const std::string & Text);
    bool ParseCondition(const std::string & Text);

    cSkin * Skin(void) const { return mSkin; }
    const std::string & Id(void) const { return mId; }
//    const cType & Value(void) const { return mValue; }
    const cType & Value(void);
    cSkinFunction * Condition(void) const { return mCondition; }
};

class cSkinVariables: public std::vector <cSkinVariable *>
{
public:
    cSkinVariables(void);
    ~cSkinVariables(void);
};

} // end of namespace

#endif
