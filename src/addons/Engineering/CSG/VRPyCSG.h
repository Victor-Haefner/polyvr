#ifndef VRPYCSG_H_INCLUDED
#define VRPYCSG_H_INCLUDED

#include "core/scripting/VRPyBase.h"
#include "CSGGeometry.h"

struct VRPyCSG : VRPyBaseT<OSG::CSGGeometry> {
    static PyMemberDef members[];
    static PyMethodDef methods[];

	static PyObject* getOperation(VRPyCSG* self);
    static PyObject* setOperation(VRPyCSG* self, PyObject* args);
    static PyObject* getEditMode(VRPyCSG* self);
    static PyObject* setEditMode(VRPyCSG* self, PyObject* args);
    static PyObject* markEdges(VRPyCSG* self, PyObject* args);
    static PyObject* setThreshold(VRPyCSG* self, PyObject* args);
};

#endif // VRPYCSG_H_INCLUDED
