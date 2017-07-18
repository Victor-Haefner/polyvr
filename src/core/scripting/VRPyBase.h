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
#include <OpenSG/OSGLine.h>
#include "core/utils/VRFunction.h"

using namespace std;

struct VRPyBase {
    PyObject_HEAD;
    static PyObject* err;

    static PyObject* setErr(string e);

    static PyObject* parseObject(PyObject *args);
    template <typename T>
    static void parseObject(PyObject *args, T& t) {
        t = 0;
        if (! PyArg_ParseTuple(args, "O", &t)) return;
        if ((PyObject*)t == Py_None) t = 0;
    }

    template <typename T>
    static void execPyCall(PyObject* pyFkt, PyObject* pArgs, T t);
    template <typename T>
    static VRFunction<T>* parseCallback(PyObject *args);

    static vector<PyObject*> parseList(PyObject *args);
    static OSG::Vec2f parseVec2f(PyObject *args);
    static OSG::Vec3f parseVec3f(PyObject *args);
    static OSG::Vec4f parseVec4f(PyObject *args);
    static OSG::Vec3i parseVec3i(PyObject *args);
    static float parseFloat(PyObject *args);
    static bool parseBool(PyObject *args);
    static int parseInt(PyObject *args);
    static string parseString(PyObject *args);

    static bool isList(PyObject* o);
    static bool isTuple(PyObject* o);
    static int pySize(PyObject* v);
    static PyObject* getItem(PyObject* v, int i);
    static vector<PyObject*> pyListToVector(PyObject* v);
    template <class T, class t>
    static bool pyListToVector(PyObject* data, T& vec);
    static OSG::Vec2f parseVec2fList(PyObject *li);
    static OSG::Vec3f parseVec3fList(PyObject *li);
    static OSG::Vec4f parseVec4fList(PyObject *li);
    static OSG::Vec2d parseVec2dList(PyObject *li);
    static OSG::Vec3d parseVec3dList(PyObject *li);
    static OSG::Vec4d parseVec4dList(PyObject *li);
    static OSG::Vec2i parseVec2iList(PyObject *li);
    static OSG::Vec3i parseVec3iList(PyObject *li);
    static OSG::Vec4i parseVec4iList(PyObject *li);
    static OSG::Line PyToLine(PyObject *li);

    static PyObject* toPyTuple(const OSG::Vec2f& v);
    static PyObject* toPyTuple(const OSG::Vec3f& v);
    static PyObject* toPyTuple(const OSG::Vec4f& v);
    static PyObject* toPyTuple(const OSG::Vec2i& v);
    static PyObject* toPyTuple(const OSG::Vec3i& v);
    static PyObject* toPyTuple(const OSG::Vec4i& v);
    static PyObject* toPyTuple(const OSG::Vec2d& v);
    static PyObject* toPyTuple(const OSG::Vec3d& v);
    static PyObject* toPyTuple(const OSG::Vec4d& v);
    static PyObject* toPyTuple(const vector<string>& v);
    static PyObject* toPyTuple(const vector<PyObject*>& v);

    static int toOSGConst(string cst);
    static int toOSGConst(PyObject* o);
    static int toGLConst(string cst);
    static int toGLConst(PyObject* o);
    static bool isNone(PyObject* o);
};

template<class T>
struct VRPyBaseT : public VRPyBase {
    //T* obj = 0;
    std::shared_ptr<T> objPtr = 0;
    bool owner = true;
    static PyTypeObject type;
    static PyTypeObject* typeRef;

    VRPyBaseT();
    //virtual ~VRPyBaseT();

    bool valid();

    static bool check(PyObject* o);

    static PyObject* fromObject(T obj);
    static PyObject* fromSharedPtr(std::shared_ptr<T> obj);
    static bool      parse(PyObject *args, T** obj);
    static bool      parse(PyObject *args, std::shared_ptr<T>* obj);
    static PyObject* New_ptr(PyTypeObject *type, PyObject *args, PyObject *kwds);
    static PyObject* New_named_ptr(PyTypeObject *type, PyObject *args, PyObject *kwds);
    static PyObject* New_toZero(PyTypeObject *type, PyObject *args, PyObject *kwds);
    static PyObject* New_VRObjects_ptr(PyTypeObject *type, PyObject *args, PyObject *kwds);

    static PyObject* allocPtr(PyTypeObject* type, std::shared_ptr<T> t);
    static void dealloc(VRPyBaseT<T>* self);
    static int init(VRPyBaseT<T> *self, PyObject *args, PyObject *kwds);
    static void registerModule(string name, PyObject* mod, PyTypeObject* tp_base = 0);
};

#endif // VRPYBASE_H_INCLUDED
