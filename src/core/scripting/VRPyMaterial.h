#ifndef VRPYMATERIAL_H_INCLUDED
#define VRPYMATERIAL_H_INCLUDED

#include "core/objects/material/VRMaterial.h"
#ifndef WITHOUT_AV
#include "core/objects/material/VRVideo.h"
#endif
#include "VRPyBase.h"

struct VRPyMaterial : VRPyBaseT<OSG::VRMaterial> {
    static PyMethodDef methods[];
    
    static PyObject* setTexture(VRPyMaterial* self, PyObject* args);
    static PyObject* setShaderParameter(VRPyMaterial* self, PyObject* args);
};

#ifndef WITHOUT_AV
struct VRPyVideo : VRPyBaseT<OSG::VRVideo> {
    static PyMethodDef methods[];
};
#endif

#endif // VRPYMATERIAL_H_INCLUDED
