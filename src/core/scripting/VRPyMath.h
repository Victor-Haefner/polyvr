#ifndef VRPYMATH_H_INCLUDED
#define VRPYMATH_H_INCLUDED

#include "VRPyObject.h"
#include "core/math/Expression.h"
#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGMatrix.h>
#include <OpenSG/OSGLine.h>

PyObject* toPyObject(const OSG::Vec2d& v);
PyObject* toPyObject(const OSG::Vec3d& v);

struct VRPyVec2f : VRPyBaseT<OSG::Vec2d> {
    OSG::Vec2d v;

    static PyMethodDef methods[];
    static PyNumberMethods nMethods;
    static PySequenceMethods sMethods;

    static PyObject* New(PyTypeObject *type, PyObject *args, PyObject *kwds);
    static PyObject* Print(PyObject* self);

    static PyObject* normalize(VRPyVec2f* self);
    static PyObject* length(VRPyVec2f* self);
    static PyObject* dot(VRPyVec2f* self, PyObject* args);
    static PyObject* cross(VRPyVec2f* self, PyObject* args);
    static PyObject* asList(VRPyVec2f* self);
    static PyObject* distance(VRPyVec2f* self, PyObject* args);

    static PyObject* add(PyObject* self, PyObject* v);
    static PyObject* sub(PyObject* self, PyObject* v);
    static PyObject* mul(PyObject* self, PyObject* f);
    static PyObject* div(PyObject* self, PyObject* f);
    static PyObject* neg(PyObject* self);
    static PyObject* abs(PyObject* self);

    static Py_ssize_t len(PyObject* self);
    static PyObject* getItem(PyObject* self, Py_ssize_t i);
    static int setItem(PyObject* self, Py_ssize_t i, PyObject* val);
    static PyObject* getSlice(PyObject* self, long i0, long i1);
};

struct VRPyVec3f : VRPyBaseT<OSG::Vec3d> {
    OSG::Vec3d v;

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
    static PyObject* distance(VRPyVec3f* self, PyObject* args);

    static PyObject* add(PyObject* self, PyObject* v);
    static PyObject* sub(PyObject* self, PyObject* v);
    static PyObject* mul(PyObject* self, PyObject* f);
    static PyObject* div(PyObject* self, PyObject* f);
    static PyObject* neg(PyObject* self);
    static PyObject* abs(PyObject* self);

    static Py_ssize_t len(PyObject* self);
    static PyObject* getItem(PyObject* self, Py_ssize_t i);
    static int setItem(PyObject* self, Py_ssize_t i, PyObject* val);
    static PyObject* getSlice(PyObject* self, long i0, long i1);
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

struct VRPyMathExpression : VRPyBaseT<OSG::MathExpression> {
    static PyMethodDef methods[];
};

#endif // VRPYMATH_H_INCLUDED
