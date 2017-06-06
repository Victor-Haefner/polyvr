#ifndef VRUTILSFWD_H_INCLUDED
#define VRUTILSFWD_H_INCLUDED

#include <memory>
#include <vector>
#include <string>
#include <Python.h>
#include "VRFwdDeclTemplate.h"

ptrFwd(VRProgress);

namespace OSG {
    ptrTemplateFwd(VRCallbackWrapper, VRCallbackStrWrapper, std::vector<std::string>);
    ptrTemplateFwd(VRCallbackWrapper, VRCallbackPyWrapper, std::vector<PyObject*>);
    ptrFwd(VRVisualLayer);
}

#endif // VRUTILSFWD_H_INCLUDED
