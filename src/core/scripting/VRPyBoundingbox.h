#ifndef VRPYBOUNDINGBOX_H_INCLUDED
#define VRPYBOUNDINGBOX_H_INCLUDED

#include "core/math/boundingbox.h"
#include "core/scripting/VRPyBase.h"

struct VRPyBoundingbox : VRPyBaseT<OSG::Boundingbox> {
    static PyMethodDef methods[];

    static PyObject* min(VRPyBoundingbox* self);
    static PyObject* max(VRPyBoundingbox* self);
    static PyObject* update(VRPyBoundingbox* self, PyObject* args);
};

#endif // VRPYBOUNDINGBOX_H_INCLUDED
