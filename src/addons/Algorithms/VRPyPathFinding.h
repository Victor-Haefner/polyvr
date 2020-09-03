#ifndef VRPYPATHFINDING_H_INCLUDED
#define VRPYPATHFINDING_H_INCLUDED

#include "core/scripting/VRPyBase.h"
#include "VRPathFinding.h"

struct VRPyPathFinding : VRPyBaseT<OSG::VRPathFinding> {
    static PyMethodDef methods[];
    static PyObject* computePath(VRPyPathFinding* self, PyObject* args);
};

#endif // VRPYPATHFINDING_H_INCLUDED
