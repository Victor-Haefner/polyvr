#ifndef VRPYMECHANISM_H_INCLUDED
#define VRPYMECHANISM_H_INCLUDED

#include "core/scripting/VRPyObject.h"
#include "VRMechanism.h"

class VRPyGeometry;

struct VRPyMechanism : VRPyBaseT<OSG::VRMechanism> {
    static PyMethodDef methods[];

    static PyObject* add(VRPyMechanism* self, PyObject* args);
    static PyObject* clear(VRPyMechanism* self);
    static PyObject* update(VRPyMechanism* self);
    static PyObject* addChain(VRPyMechanism* self, PyObject* args);
};

#endif // VRPYMECHANISM_H_INCLUDED
