#include "VRCallbackWrapper.h"
#include "core/scripting/VRPyBase.h"
#include "core/utils/toString.h"

string VRCallbackWrapperBase::err = "";

template<> int convertDefaultParam(string s, int& t) {
    int iOSG = VRPyBase::toOSGConst(s);
    if (iOSG != -1) { t = iOSG; return 1; }
    int iGL = VRPyBase::toGLConst(s);
    if (iGL != -1) { t = iGL; return 1; }
    return toValue<int>(s, t);
}
