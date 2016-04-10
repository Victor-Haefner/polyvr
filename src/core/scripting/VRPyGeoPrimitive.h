#ifndef VRPYGEOPRIMITIVE_H_INCLUDED
#define VRPYGEOPRIMITIVE_H_INCLUDED

#include "VRPyObject.h"
#include "core/tools/VRGeoPrimitive.h"

struct VRPyGeoPrimitive : VRPyBaseT<OSG::VRGeoPrimitive> {
    static PyMethodDef methods[];

    static PyObject* select(VRPyGeoPrimitive* self, PyObject* args);
    static PyObject* getHandles(VRPyGeoPrimitive* self);
};

#endif // VRPYGEOPRIMITIVE_H_INCLUDED
