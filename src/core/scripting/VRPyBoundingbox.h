#ifndef VRPYBOUNDINGBOX_H_INCLUDED
#define VRPYBOUNDINGBOX_H_INCLUDED

#include "core/math/boundingbox.h"
#include "core/scripting/VRPyBase.h"

struct VRPyBoundingbox : VRPyBaseT<OSG::boundingbox> {
    static PyMethodDef methods[];

    static PyObject* min(VRPyBoundingbox* self);
    static PyObject* max(VRPyBoundingbox* self);
};

#endif // VRPYBOUNDINGBOX_H_INCLUDED
