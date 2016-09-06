#ifndef VRPYGRAPHLAYOUT_H_INCLUDED
#define VRPYGRAPHLAYOUT_H_INCLUDED

#include "core/scripting/VRPyBase.h"
#include "VRGraphLayout.h"

struct VRPyGraphLayout : VRPyBaseT<OSG::VRGraphLayout> {
    static PyMethodDef methods[];
    static PyObject* setGraph(VRPyGraphLayout* self, PyObject* args);
};

#endif // VRPYGRAPHLAYOUT_H_INCLUDED
