#include "VRPyBaseT.h"
#include "VRPyBaseFactory.h"
#include "VRPyMath.h"
#include "VRPyPose.h"
#include "core/utils/VRCallbackWrapper.h"
#include "core/utils/VRFunction.h"

#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGColor.h>

using namespace OSG;

typedef PyObject* PyObjectPtr;

template<> string typeName(const PyObjectPtr* o) {
    if (!o) return "PyObject";
    if (!*o) return "PyObject";
    PyTypeObject* type = (*o)->ob_type;
    if (!type) return "undefined";
    return type->tp_name;
}

template<> bool toValue(PyObject* o, int& v) {
    if (PyLong_Check(o)) { v = PyLong_AsLong(o); return 1; }
    if (PyUnicode_Check(o)) { // check for enumerator constant
        int iOSG = VRPyBase::toOSGConst(o);
        if (iOSG != -1) { v = iOSG; return 1; }
        int iGL = VRPyBase::toGLConst(o);
        if (iGL != -1) { v = iGL; return 1; }
    }
    return 0;
}

template<> bool toValue(PyObject* o, void*& v) { v = o; Py_INCREF(o); return 1; }
template<> bool toValue(PyObject* o, PyObject*& v) { v = o; Py_INCREF(o); return 1; }
template<> bool toValue(PyObject* o, bool& v) { if (!PyNumber_Check(o)) return 0; v = PyLong_AsLong(o); return 1; }
template<> bool toValue(PyObject* o, char& v) { if (!PyNumber_Check(o)) return 0; v = PyLong_AsLong(o); return 1; }
template<> bool toValue(PyObject* o, unsigned char& v) { if (!PyNumber_Check(o)) return 0; v = PyLong_AsLong(o); return 1; }
template<> bool toValue(PyObject* o, unsigned int& v) { if (!PyLong_Check(o)) return 0; v = PyLong_AsLong(o); return 1; }
template<> bool toValue(PyObject* o, short& v) { if (!PyLong_Check(o)) return 0; v = PyLong_AsLong(o); return 1; }
template<> bool toValue(PyObject* o, size_t& v) { if (!PyLong_Check(o)) return 0; v = PyLong_AsLong(o); return 1; }
template<> bool toValue(PyObject* o, float& v) { if (!PyNumber_Check(o)) return 0; v = PyFloat_AsDouble(o); return 1; }
template<> bool toValue(PyObject* o, double& v) { if (!PyNumber_Check(o)) return 0; v = PyFloat_AsDouble(o); return 1; }

template<> bool toValue(PyObject* o, string& v) {
    if (o == 0) return 1;

    string oType = string(o->ob_type->tp_name);

    //cout << "toValue->string " << bool(o == Py_None) << " " << o << endl;
    //cout << " - - - toValue->string " << o->ob_type->tp_name << endl;
    if (oType == "tuple") {
        v = "(";
        for (int i=0; i<PyTuple_GET_SIZE(o); i++) {
            auto c = PyTuple_GET_ITEM(o, i);
            if (i>0) v += ", ";
            v += PyUnicode_AsUTF8(c);
        }
        v += ")";
        return 1;
    }

    if (oType == "bytes") {
        char* buffer = nullptr;
        Py_ssize_t length = 0;
        if (PyBytes_AsStringAndSize(o, &buffer, &length) == 0) {
            v.assign(buffer, length);
            return 1;
        }
    }

    if (VRPyBase::isNone(o)) return 1;
    if (!PyUnicode_Check(o) && !PyUnicode_Check(o)) o = PyObject_Repr(o); // may segfault with tuple!
    auto vc = PyUnicode_AsUTF8(o);
    v = vc?vc:"";
    return 1;
}

bool PyVec_Check(PyObject* o, int N, char type) {
    if (N == 2 && type == 'f') if (VRPyVec2f::check(o)) return true;
    if (N == 3 && type == 'f') if (VRPyVec3f::check(o)) return true;
    if (!PyList_Check(o)) return false;
    if (PyList_GET_SIZE(o) != N) return false;
    switch(type) {
        case 'f':
            for (int i=0; i<N; i++) if (!PyNumber_Check( PyList_GetItem(o,i) )) return false;
            break;
        case 'i':
            for (int i=0; i<N; i++) if (!PyLong_Check( PyList_GetItem(o,i) )) return false;
            break;
    }
    return true;
}

bool toColorValue(PyObject* o, Color4f& v) {
    if (PyVec_Check(o, 4, 'f')) { v = Vec4f(VRPyBase::parseVec4dList(o)); return 1; }
    if (PyVec_Check(o, 3, 'f')) { v = Vec4f(VRPyBase::parseVec3dList(o)); return 1; }
    if (PyVec_Check(o, 4, 'i')) { v = Vec4f(VRPyBase::parseVec4iList(o)); return 1; }
    if (PyVec_Check(o, 3, 'i')) { v = Vec4f(VRPyBase::parseVec3iList(o)); return 1; }

    auto hex = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return 0;
    };

    auto c1 = [&](char a) { return hex(a) / 15.f; };
    auto c2 = [&](char a, char b) { return (hex(a) * 16 + hex(b)) / 255.f; };

    string s; toValue(o, s);
    if (s.length() == 4 && s[0] == '#') { v = Color4f(c1(s[1]), c1(s[2]), c1(s[3]), 1.0); return 1; }
    if (s.length() == 5 && s[0] == '#') { v = Color4f(c1(s[1]), c1(s[2]), c1(s[3]), c1(s[4])); return 1; }
    if (s.length() == 7 && s[0] == '#') { v = Color4f(c2(s[1],s[2]), c2(s[3],s[4]), c2(s[5],s[6]),1.0); return 1; }
    if (s.length() == 9 && s[0] == '#') { v = Color4f(c2(s[1],s[2]), c2(s[3],s[4]), c2(s[5],s[6]), c2(s[7],s[8])); return 1; }

    return 0;
}

template<> bool toValue(PyObject* o, Color3f& v) {
    Color4f col;
    if (toColorValue(o, col)) { v = Color3f(col[0], col[1], col[2]); return 1; }
    return 0;
}

template<> bool toValue(PyObject* o, Color4f& v) {
    Color4f col;
    if (toColorValue(o, col)) { v = col; return 1; }
    return 0;
}

template<> bool toValue(PyObject* o, Color3ub& v) {
    Color4f col;
    if (toColorValue(o, col)) { v = Color3ub(col[0], col[1], col[2]); return 1; }
    return 0;
}

template<> bool toValue(PyObject* o, Color4ub& v) {
    Color4f col;
    if (toColorValue(o, col)) { v = Color4ub(col[0], col[1], col[2], col[3]); return 1; }
    return 0;
}

template<> bool toValue(PyObject* o, Vec2f& v) { if (!PyVec_Check(o, 2, 'f')) return 0; v = Vec2f(VRPyBase::parseVec2dList(o)); return 1; }
template<> bool toValue(PyObject* o, Vec3f& v) { if (!PyVec_Check(o, 3, 'f')) return 0; v = Vec3f(VRPyBase::parseVec3dList(o)); return 1; }
template<> bool toValue(PyObject* o, Vec4f& v) { if (!PyVec_Check(o, 4, 'f')) return 0; v = Vec4f(VRPyBase::parseVec4dList(o)); return 1; }
template<> bool toValue(PyObject* o, Vec2d& v) { if (!PyVec_Check(o, 2, 'f')) return 0; v = VRPyBase::parseVec2dList(o); return 1; }
template<> bool toValue(PyObject* o, Vec3d& v) { if (!PyVec_Check(o, 3, 'f')) return 0; v = VRPyBase::parseVec3dList(o); return 1; }
template<> bool toValue(PyObject* o, Vec4d& v) { if (!PyVec_Check(o, 4, 'f')) return 0; v = VRPyBase::parseVec4dList(o); return 1; }
template<> bool toValue(PyObject* o, Pnt2d& v) { if (!PyVec_Check(o, 2, 'f')) return 0; v = Pnt2d( VRPyBase::parseVec2dList(o) ); return 1; }
template<> bool toValue(PyObject* o, Pnt3d& v) { if (!PyVec_Check(o, 3, 'f')) return 0; v = Pnt3d( VRPyBase::parseVec3dList(o) ); return 1; }
template<> bool toValue(PyObject* o, Pnt4d& v) { if (!PyVec_Check(o, 4, 'f')) return 0; v = Pnt4d( VRPyBase::parseVec4dList(o) ); return 1; }
template<> bool toValue(PyObject* o, Vec2ub& v) { if (!PyVec_Check(o, 2, 'i')) return 0; v = Vec2ub( VRPyBase::parseVec2iList(o) ); return 1; }
template<> bool toValue(PyObject* o, Vec2i& v) { if (!PyVec_Check(o, 2, 'i')) return 0; v = VRPyBase::parseVec2iList(o); return 1; }
template<> bool toValue(PyObject* o, Vec3i& v) { if (!PyVec_Check(o, 3, 'i')) return 0; v = VRPyBase::parseVec3iList(o); return 1; }
template<> bool toValue(PyObject* o, Vec4i& v) { if (!PyVec_Check(o, 4, 'i')) return 0; v = VRPyBase::parseVec4iList(o); return 1; }
template<> bool toValue(PyObject* o, Matrix4d& m) { if (!PyVec_Check(o, 16, 'f')) return 0; m = VRPyBase::parseMatrixList(o); return 1; }

template<> bool toValue(PyObject* o, Line& l) {
    if (VRPyLine::check(o)) {
        l = ((VRPyLine*)o)->l;
        return 1;
    }

    if (PyVec_Check(o, 6, 'f')) {
        l = VRPyBase::PyToLine(o);
        return 1;
    }

    if (PyTuple_Check(o)) {
        int N = PyTuple_GET_SIZE(o);
        if (N == 2) {
            PyObject* p = PyTuple_GetItem(o, 0);
            PyObject* d = PyTuple_GetItem(o, 1);
            if (VRPyVec3f::check(p) && VRPyVec3f::check(d)) {
                l = VRPyBase::PyToLine(o);
                return 1;
            }
        }
    }

    return 0;
}

template<> bool toValue(PyObject* o, Pose& m) {
    if (!VRPyPose::check(o)) return 0;
    auto p = (VRPyPose*)(o);
    m = *p->objPtr.get();
    return 1;
}

template<> bool toValue(PyObject* o, std::shared_ptr<VRFunction<void>>& v) {
    //if (!VRPyEntity::check(o)) return 0; // TODO: add checks!
    if (VRPyBase::isNone(o)) v = 0;
    else {
        Py_IncRef(o);
        addPyCallback(o);
        v = VRFunction<void>::create( "pyExecCall", bind(VRPyBase::execPyCallVoidVoid, o, PyTuple_New(0)) );
    }
    return 1;
}





