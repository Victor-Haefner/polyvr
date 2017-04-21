#ifndef VRPYMATH_H_INCLUDED
#define VRPYMATH_H_INCLUDED

#include "VRPyObject.h"
#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGMatrix.h>
#include <OpenSG/OSGLine.h>

struct VRPyVec3f : VRPyBaseT<OSG::Vec3f> {
    OSG::Vec3f v;

    static PyMethodDef methods[];
    static PyNumberMethods nMethods;
    static PySequenceMethods sMethods;

    static PyObject* New(PyTypeObject *type, PyObject *args, PyObject *kwds);
    static PyObject* Print(PyObject* self);

    static PyObject* normalize(VRPyVec3f* self);
    static PyObject* length(VRPyVec3f* self);
    static PyObject* dot(VRPyVec3f* self, PyObject* args);
    static PyObject* cross(VRPyVec3f* self, PyObject* args);
    static PyObject* asList(VRPyVec3f* self);

    static PyObject* add(PyObject* self, PyObject* v);
    static PyObject* sub(PyObject* self, PyObject* v);
    static PyObject* mul(PyObject* self, PyObject* f);
    static PyObject* div(PyObject* self, PyObject* f);
    static PyObject* neg(PyObject* self);
    static PyObject* abs(PyObject* self);

    static Py_ssize_t len(PyObject* self);
    static PyObject* getItem(PyObject* self, Py_ssize_t i);
    static PyObject* setItem(PyObject* self, Py_ssize_t i, PyObject* val);
};

struct VRPyLine : VRPyBaseT<OSG::Line> {
    OSG::Line l;

    static PyMethodDef methods[];
    static PyNumberMethods nMethods;

    static PyObject* New(PyTypeObject *type, PyObject *args, PyObject *kwds);
    static PyObject* Print(PyObject* self);

    static PyObject* intersect(VRPyLine* self, PyObject *args);
    static PyObject* pos(VRPyLine* self);
    static PyObject* dir(VRPyLine* self);
};

#endif // VRPYMATH_H_INCLUDED
