#ifndef VRPYLOD_H
#define VRPYLOD_H

#include "VRPyBase.h"
#include "core/objects/VRLod.h"

struct VRPyLod : VRPyBaseT<OSG::VRLod> {
    static PyMethodDef methods[];

    static PyObject* setCenter(VRPyLod* self, PyObject* args);
    static PyObject* setDistance(VRPyLod* self, PyObject* args);
};

#endif // VRPYLOD_H
