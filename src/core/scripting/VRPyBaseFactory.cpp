#include "VRPyBaseT.h"
#include "VRPyBaseFactory.h"
#include "VRPyMath.h"
#include "core/utils/VRCallbackWrapper.h"

#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGColor.h>

using namespace OSG;

typedef PyObject* PyObjectPtr;

template<> string typeName(const PyObjectPtr& o) {
    PyTypeObject* type = o->ob_type;
    if (!type) return "undefined";
    return type->tp_name;
}

template<> bool toValue(PyObject* o, bool& v) { if (!PyBool_Check(o)) return 0; v = PyInt_AsLong(o); return 1; }
template<> bool toValue(PyObject* o, int& v) { if (!PyInt_Check(o)) return 0; v = PyInt_AsLong(o); return 1; }
template<> bool toValue(PyObject* o, float& v) { if (!PyNumber_Check(o)) return 0; v = PyFloat_AsDouble(o); return 1; }
template<> bool toValue(PyObject* o, double& v) { if (!PyNumber_Check(o)) return 0; v = PyFloat_AsDouble(o); return 1; }
template<> bool toValue(PyObject* o, string& v) { if (!PyString_Check(o)) return 0; v = PyString_AsString(o); return 1; }

template<> bool toValue(PyObject* o, VRAnimCbPtr& v) {
    //if (!VRPyEntity::check(o)) return 0; // TODO: add checks!
    Py_IncRef(o);
    v = VRAnimCb::create( "pyExecCall", boost::bind(VRPyBase::execPyCall<float>, o, PyTuple_New(1), _1) );
    return 1;
}

bool PyVec_Check(PyObject* o, int N, char type) {
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
template<> bool toValue(PyObject* o, Color4f& v) { if (!PyVec_Check(o, 4, 'f')) return 0; v = Vec4f(VRPyBase::parseVec4dList(o)); return 1; }
template<> bool toValue(PyObject* o, Vec2d& v) { if (!PyVec_Check(o, 2, 'f')) return 0; v = VRPyBase::parseVec2dList(o); return 1; }
template<> bool toValue(PyObject* o, Vec3d& v) { if (!PyVec_Check(o, 3, 'f')) return 0; v = VRPyBase::parseVec3dList(o); return 1; }
template<> bool toValue(PyObject* o, Vec4d& v) { if (!PyVec_Check(o, 4, 'f')) return 0; v = VRPyBase::parseVec4dList(o); return 1; }
template<> bool toValue(PyObject* o, Vec3i& v) { if (!PyVec_Check(o, 3, 'i')) return 0; v = VRPyBase::parseVec3iList(o); return 1; }
template<> bool toValue(PyObject* o, Vec4i& v) { if (!PyVec_Check(o, 4, 'i')) return 0; v = VRPyBase::parseVec4iList(o); return 1; }







