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

template<> string typeName(const PyObjectPtr& o) {
    PyTypeObject* type = o->ob_type;
    if (!type) return "undefined";
    return type->tp_name;
}

template<> bool toValue(PyObject* o, int& v) {
    if (PyInt_Check(o)) { v = PyInt_AsLong(o); return 1; }
    if (PyString_Check(o)) { // check for enumerator constant
        int iOSG = VRPyBase::toOSGConst(o);
        if (iOSG != -1) { v = iOSG; return 1; }
        int iGL = VRPyBase::toGLConst(o);
        if (iGL != -1) { v = iGL; return 1; }
    }
    return 0;
}

template<> bool toValue(PyObject* o, void*& v) { v = o; Py_INCREF(o); return 1; }
template<> bool toValue(PyObject* o, bool& v) { if (!PyNumber_Check(o)) return 0; v = PyInt_AsLong(o); return 1; }
template<> bool toValue(PyObject* o, char& v) { if (!PyNumber_Check(o)) return 0; v = PyInt_AsLong(o); return 1; }
template<> bool toValue(PyObject* o, unsigned char& v) { if (!PyNumber_Check(o)) return 0; v = PyInt_AsLong(o); return 1; }
template<> bool toValue(PyObject* o, unsigned int& v) { if (!PyInt_Check(o)) return 0; v = PyInt_AsLong(o); return 1; }
template<> bool toValue(PyObject* o, size_t& v) { if (!PyInt_Check(o)) return 0; v = PyInt_AsLong(o); return 1; }
template<> bool toValue(PyObject* o, float& v) { if (!PyNumber_Check(o)) return 0; v = PyFloat_AsDouble(o); return 1; }
template<> bool toValue(PyObject* o, double& v) { if (!PyNumber_Check(o)) return 0; v = PyFloat_AsDouble(o); return 1; }

template<> bool toValue(PyObject* o, string& v) {
    if (!PyString_Check(o) && !PyUnicode_Check(o)) o = PyObject_Repr(o);
    auto vc = PyString_AsString(o);
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
            for (int i=0; i<N; i++) if (!PyInt_Check( PyList_GetItem(o,i) )) return false;
            break;
    }
    return true;
}

template<> bool toValue(PyObject* o, Color3f& v) { if (!PyVec_Check(o, 3, 'f')) return 0; v = Vec3f(VRPyBase::parseVec3dList(o)); return 1; }
template<> bool toValue(PyObject* o, Color4f& v) {
    if (PyVec_Check(o, 4, 'f')) { v = Vec4f(VRPyBase::parseVec4dList(o)); return 1; }
    if (PyVec_Check(o, 3, 'f')) { v = Vec4f(VRPyBase::parseVec3dList(o)); return 1; }
    return 0;
}

template<> bool toValue(PyObject* o, Vec2d& v) { if (!PyVec_Check(o, 2, 'f')) return 0; v = VRPyBase::parseVec2dList(o); return 1; }
template<> bool toValue(PyObject* o, Vec3d& v) { if (!PyVec_Check(o, 3, 'f')) return 0; v = VRPyBase::parseVec3dList(o); return 1; }
template<> bool toValue(PyObject* o, Vec4d& v) { if (!PyVec_Check(o, 4, 'f')) return 0; v = VRPyBase::parseVec4dList(o); return 1; }
template<> bool toValue(PyObject* o, Pnt2d& v) { if (!PyVec_Check(o, 2, 'f')) return 0; v = Pnt2d( VRPyBase::parseVec2dList(o) ); return 1; }
template<> bool toValue(PyObject* o, Pnt3d& v) { if (!PyVec_Check(o, 3, 'f')) return 0; v = Pnt3d( VRPyBase::parseVec3dList(o) ); return 1; }
template<> bool toValue(PyObject* o, Pnt4d& v) { if (!PyVec_Check(o, 4, 'f')) return 0; v = Pnt4d( VRPyBase::parseVec4dList(o) ); return 1; }
template<> bool toValue(PyObject* o, Vec2i& v) { if (!PyVec_Check(o, 2, 'i')) return 0; v = VRPyBase::parseVec2iList(o); return 1; }
template<> bool toValue(PyObject* o, Vec3i& v) { if (!PyVec_Check(o, 3, 'i')) return 0; v = VRPyBase::parseVec3iList(o); return 1; }
template<> bool toValue(PyObject* o, Vec4i& v) { if (!PyVec_Check(o, 4, 'i')) return 0; v = VRPyBase::parseVec4iList(o); return 1; }
template<> bool toValue(PyObject* o, Line& l) { if (!PyVec_Check(o, 6, 'f')) return 0; l = VRPyBase::PyToLine(o); return 1; }
template<> bool toValue(PyObject* o, Matrix4d& m) { if (!PyVec_Check(o, 16, 'f')) return 0; m = VRPyBase::parseMatrixList(o); return 1; }

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
        v = VRFunction<void>::create( "pyExecCall", bind(VRPyBase::execPyCallVoidVoid, o, PyTuple_New(0)) );
    }
    return 1;
}





