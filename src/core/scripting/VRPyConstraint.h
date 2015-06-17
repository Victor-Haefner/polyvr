#ifndef VRPYCONSTRAINT_H_INCLUDED
#define VRPYCONSTRAINT_H_INCLUDED

#include "VRPyObject.h"
#include "core/objects/geometry/VRConstraint.h"
#include <OpenSG/OSGQuaternion.h>
struct VRPyConstraint : VRPyBaseT<OSG::VRConstraint> {
    static PyMemberDef members[];
    static PyMethodDef methods[];

    static PyObject* setDOFRange(VRPyConstraint* self, PyObject* args);
    static PyObject* setLocal(VRPyConstraint* self, PyObject* args);
    static PyObject* free(VRPyConstraint* self, PyObject* args);
    static PyObject* lock(VRPyConstraint* self, PyObject* args);
    static PyObject* setLocalOffsetB(VRPyConstraint* self, PyObject* args);
    static PyObject* setLocalOffsetA(VRPyConstraint* self, PyObject* args);
};

#endif // VRPYCONSTRAINT_H_INCLUDED
