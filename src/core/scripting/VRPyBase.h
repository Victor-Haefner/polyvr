#ifndef VRPYBASE_H_INCLUDED
#define VRPYBASE_H_INCLUDED

#undef _XOPEN_SOURCE
#undef _POSIX_C_SOURCE
#include <Python.h>
#include <structmember.h>
#include <string>
#include <iostream>
#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>

using namespace std;

struct VRPyBase {
    PyObject_HEAD;
    static PyObject* err;

    static PyObject* parseObject(PyObject *args);

    static vector<PyObject*> pyListToVector(PyObject *v);
    static vector<PyObject*> parseList(PyObject *args);
    static OSG::Vec2f parseVec2f(PyObject *args);
    static OSG::Vec3f parseVec3f(PyObject *args);
    static OSG::Vec4f parseVec4f(PyObject *args);
    static float parseFloat(PyObject *args);
    static bool parseBool(PyObject *args);
    static int parseInt(PyObject *args);
    static string parseString(PyObject *args);
    static OSG::Vec3f parseVec3fList(PyObject *li);

    static PyObject* toPyTuple(OSG::Vec3f v);
    static PyObject* toPyTuple(OSG::Vec3i v);
    static PyObject* toPyTuple(OSG::Vec2f v);
};

template<class T>
struct VRPyBaseT : public VRPyBase {
    T* obj;
    bool owner;
    static PyTypeObject type;
    static PyTypeObject* typeRef;

    VRPyBaseT();

    static PyObject* fromPtr(T* obj);
    static PyObject* New(PyTypeObject *type, PyObject *args, PyObject *kwds);
    static PyObject* New_toZero(PyTypeObject *type, PyObject *args, PyObject *kwds);
    static PyObject* New_VRObjects(PyTypeObject *type, PyObject *args, PyObject *kwds);
    static PyObject* New_VRObjects_unnamed(PyTypeObject *type, PyObject *args, PyObject *kwds);

    static void dealloc(VRPyBaseT<T>* self);
    static int init(VRPyBaseT<T> *self, PyObject *args, PyObject *kwds);
    static void registerModule(string name, PyObject* mod, PyTypeObject* tp_base = 0);
};

#endif // VRPYBASE_H_INCLUDED
