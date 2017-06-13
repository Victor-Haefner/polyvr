#ifndef VRUTILSFWD_H_INCLUDED
#define VRUTILSFWD_H_INCLUDED

#include <memory>
#include <vector>
#include <string>
#include <Python.h>
#include "VRFwdDeclTemplate.h"


namespace OSG {
    ptrTemplateFwd(VRCallbackWrapper, VRCallbackStrWrapper, std::string);
    ptrTemplateFwd(VRCallbackWrapper, VRCallbackPyWrapper, PyObject*);
    ptrFwd(VRVisualLayer);
    ptrFwd(VRProgress);
}

#endif // VRUTILSFWD_H_INCLUDED
