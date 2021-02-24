#ifndef VRPYCSG_H_INCLUDED
#define VRPYCSG_H_INCLUDED

#include "core/scripting/VRPyBase.h"
#include "CSGGeometry.h"

struct VRPyCSGGeometry : VRPyBaseT<OSG::CSGGeometry> {
    static PyMemberDef members[];
    static PyMethodDef methods[];

	static PyObject* getOperation(VRPyCSGGeometry* self);
    static PyObject* setOperation(VRPyCSGGeometry* self, PyObject* args);
    static PyObject* getEditMode(VRPyCSGGeometry* self);
    static PyObject* setEditMode(VRPyCSGGeometry* self, PyObject* args);
    static PyObject* markEdges(VRPyCSGGeometry* self, PyObject* args);
    static PyObject* setThreshold(VRPyCSGGeometry* self, PyObject* args);
};

#endif // VRPYCSG_H_INCLUDED
