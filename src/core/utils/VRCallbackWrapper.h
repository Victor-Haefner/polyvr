#ifndef VRCALLBACKWRAPPER_H_INCLUDED
#define VRCALLBACKWRAPPER_H_INCLUDED

#include "VRUtilsFwd.h"
#include "toString.h"

OSG_BEGIN_NAMESPACE;

template<typename Params>
struct VRCallbackWrapper {
    string format;
    VRCallbackWrapper() {}
    virtual ~VRCallbackWrapper() {}

    virtual bool execute(void* obj, const Params& params) = 0;
};

OSG_END_NAMESPACE;

#endif // VRCALLBACKWRAPPER_H_INCLUDED
