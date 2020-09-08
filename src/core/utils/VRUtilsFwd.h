#ifndef VRUTILSFWD_H_INCLUDED
#define VRUTILSFWD_H_INCLUDED

#include <memory>
#include <vector>
#include <string>
#include <Python.h>
#include "VRFwdDeclTemplate.h"


ptrFwd(XMLElement);
ptrFwd(XML);

ptrTemplateFwd(VRCallbackWrapper, VRCallbackStrWrapper, std::string);
ptrTemplateFwd(VRCallbackWrapper, VRCallbackPyWrapper, PyObject*);

namespace OSG {
    ptrFwd(VRVisualLayer);
    ptrFwd(VRProgress);
    ptrFwd(VREncryption);
    ptrFwd(VRPDF);
}

#endif // VRUTILSFWD_H_INCLUDED
