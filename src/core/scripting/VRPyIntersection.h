#ifndef VRPYINTERSECTION_H_INCLUDED
#define VRPYINTERSECTION_H_INCLUDED

#include "core/setup/devices/VRIntersect.h"
#include "core/scripting/VRPyBase.h"

struct VRPyIntersection : VRPyBaseT<OSG::VRIntersection> {
    static PyMethodDef methods[];

    static PyObject* getIntersected(VRPyIntersection* self);
    static PyObject* getIntersection(VRPyIntersection* self);
};

#endif // VRPYINTERSECTION_H_INCLUDED
