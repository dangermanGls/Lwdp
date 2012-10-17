// Copyright 2008-2011 Zhang Yun Gui, rhcad@hotmail.com
// http://sourceforge.net/projects/x3c/

#ifndef _X3_UTILITY_GUIDGENERATOR_H
#define _X3_UTILITY_GUIDGENERATOR_H

#include <Utility/Ix_GuidGenerator.h>

class Cx_GuidGenerator : public Ix_GuidGenerator
{
    X3BEGIN_CLASS_DECLARE(Cx_GuidGenerator)
        X3DEFINE_INTERFACE_ENTRY(Ix_GuidGenerator)
    X3END_CLASS_DECLARE()
protected:
    Cx_GuidGenerator() {}
    virtual ~Cx_GuidGenerator() {}

private:
    // From Ix_GuidGenerator
    virtual tstring CreateGuid(bool withBrackets = false);
    virtual tstring RemoveGuidBrackets(const tstring& uid);
    virtual tstring GetCurrentTimeString(bool hasYear = true);
    virtual ULONG CreateID(long_ type = 0);
};

#endif // _X3_UTILITY_GUIDGENERATOR_H
