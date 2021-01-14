#ifndef VRUTILSFWD_H_INCLUDED
#define VRUTILSFWD_H_INCLUDED

#include <memory>
#include <vector>
#include <string>
#include <Python.h>
#include "VRFwdDeclTemplate.h"


ptrTemplateFwd(VRCallbackWrapper, VRCallbackStrWrapper, std::string);
ptrTemplateFwd(VRCallbackWrapper, VRCallbackPyWrapper, PyObject*);

namespace OSG {
    ptrFwd(VRVisualLayer);
    ptrFwd(VRProgress);
    ptrFwd(VREncryption);
    ptrFwd(VRPDF);
    ptrFwd(XMLElement);
    ptrFwd(XML);
    ptrFwd(VRSpreadsheet);
}

#endif // VRUTILSFWD_H_INCLUDED
