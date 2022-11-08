#include <syslog.h>

#include "variable.h"
#include "skin.h"
#include "function.h"

namespace GLCD
{

cSkinVariable::cSkinVariable(cSkin * Parent)
:   mSkin(Parent),
    mValue(0),
    mCondition(NULL),
    mFunction(NULL),
    mDummyDisplay(mSkin),
    mDummyObject(&mDummyDisplay),
    mEvalMode(tevmTick),
    mEvalInterval(0),
    mTimestamp(0)
{
}

bool cSkinVariable::ParseEvalMode(const std::string & Text)
{
  
    if (Text == "always") {
        mEvalMode = tevmAlways;
    } else if (Text == "tick") {
        mEvalMode = tevmTick;
    } else if (Text == "switch") {
        mEvalMode = tevmSwitch;
    } else if (Text == "once") {
        mEvalMode = tevmOnce;
    } else if (Text.length() > 9 && Text.substr(0,9) == "interval:") {
        char * e;
        const char * t = Text.substr(9).c_str();
        long l = strtol(t, &e, 10);
        if ( ! (e == t || *e != '\0') && (l >= 100))
        {
            mEvalInterval = (int) l;
            mEvalMode = tevmInterval;
            return true;
        }
        return false;
    } else {
        return false;
    } 
    return true;
}


bool cSkinVariable::ParseValue(const std::string & Text)
{
    if (isalpha(Text[0]) || Text[0] == '#' || Text[0] == '{')
    {
        //delete mFunction;
        mFunction = new cSkinFunction(&mDummyObject);
        if (mFunction->Parse(Text))
        {
            if (mEvalMode == tevmOnce) {
                mValue = mFunction->Evaluate();
                delete mFunction;
                mFunction = NULL;
            }
            //mValue = func->Evaluate();
            //delete func;
            return true;
        }
        delete mFunction;
        mFunction = NULL;
    }
    else if (Text[0] == '\'')
    {
        mValue = Text.substr(1, Text.length() - 2);
        return true;
    }
    char * e;
    const char * t = Text.c_str();
    long l = strtol(t, &e, 10);
    if (e == t || *e != '\0')
    {
      return false;
    }
    mValue = l;
    return true;
}

bool cSkinVariable::ParseCondition(const std::string & Text)
{
    cSkinFunction *result = new cSkinFunction(&mDummyObject);
    if (result->Parse(Text))
    {
        delete mCondition;
        mCondition = result;
        return true;
    }
    return false;
}


const cType & cSkinVariable::Value(void)
{
    if ( mTimestamp > 0 &&
         ( ( mEvalMode == tevmTick && mTimestamp >= mSkin->GetTSEvalTick() )     ||
           ( mEvalMode == tevmSwitch && mTimestamp >= mSkin->GetTSEvalSwitch() ) ||
           ( mEvalMode == tevmInterval &&  (mTimestamp + (uint64_t)mEvalInterval) > mSkin->Config().Now()) 
         )
       )
    {
        return mValue;
    }

    if (mFunction != NULL) {
        mValue = mFunction->Evaluate();
        // should've been solved in ParseValue already, just to be sure ...
        if (mEvalMode == tevmOnce) {
            delete mFunction;
            mFunction = NULL;
        }
    }
    if (mEvalMode == tevmTick || mEvalMode == tevmSwitch || mEvalMode == tevmInterval) {
        mTimestamp = mSkin->Config().Now();
    }
    return mValue;
}


cSkinVariables::cSkinVariables(void)
{
}

cSkinVariables::~cSkinVariables()
{
    iterator it = begin();
    while (it != end())
    {
        delete (*it);
        it++;
    }
}

} // end of namespace
