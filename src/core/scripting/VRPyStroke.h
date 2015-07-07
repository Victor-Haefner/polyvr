#ifndef VRPyStroke_H_INCLUDED
#define VRPyStroke_H_INCLUDED

#include "VRPyObject.h"
#include "core/objects/geometry/VRStroke.h"

struct VRPyStroke : VRPyBaseT<OSG::VRStroke> {
    static PyMemberDef members[];
    static PyMethodDef methods[];

    static PyObject* setPath(VRPyStroke* self, PyObject* args);
    static PyObject* addPath(VRPyStroke* self, PyObject* args);
    static PyObject* setPaths(VRPyStroke* self, PyObject* args);
    static PyObject* getPaths(VRPyStroke* self);

    static PyObject* strokeProfile(VRPyStroke* self, PyObject* args);
    static PyObject* strokeStrew(VRPyStroke* self, PyObject* args);
    static PyObject* getGeometry(VRPyStroke* self);

    static PyObject* update(VRPyStroke* self);

    static PyObject* convertToRope(VRPyStroke* self);


};

#endif // VRPyStroke_H_INCLUDED
