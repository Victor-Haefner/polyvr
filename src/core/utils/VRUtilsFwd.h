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
    ptrFwd(VRTimer);
    ptrFwd(VRVisualLayer);
    ptrFwd(VRProgress);
    ptrFwd(VREncryption);
    ptrFwd(XMLElement);
    ptrFwd(XML);
    ptrFwd(VRSpreadsheet);
    ptrFwd(Table);
    ptrFwd(VRMutex);
    ptrFwd(VRLock);
    ptrFwd(VRScheduler);

    ptrFwd(VRPDF);
    ptrFwd(VRPDFData);
    ptrFwd(VRPDFPage);
    ptrFwd(VRPDF2DModel);
    ptrFwd(VRPDF3DModel);
}

#endif // VRUTILSFWD_H_INCLUDED
