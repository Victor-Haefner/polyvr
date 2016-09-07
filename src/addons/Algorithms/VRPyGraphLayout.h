#ifndef VRPYGRAPHLAYOUT_H_INCLUDED
#define VRPYGRAPHLAYOUT_H_INCLUDED

#include "core/scripting/VRPyBase.h"
#include "VRGraphLayout.h"

struct VRPyGraphLayout : VRPyBaseT<OSG::VRGraphLayout> {
    static PyMethodDef methods[];
    static PyObject* setGraph(VRPyGraphLayout* self, PyObject* args);
    static PyObject* setAlgorithm(VRPyGraphLayout* self, PyObject* args);
    static PyObject* setParameters(VRPyGraphLayout* self, PyObject* args);
    static PyObject* fixNode(VRPyGraphLayout* self, PyObject* args);
    static PyObject* compute(VRPyGraphLayout* self, PyObject* args);
};

#endif // VRPYGRAPHLAYOUT_H_INCLUDED
