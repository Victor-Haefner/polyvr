#include "VRPyBaseT.h"
#include "VRPyBaseFactory.h"
#include "VRPyLightBeacon.h"
#include "addons/Semantics/Reasoning/VRPyOntology.h"
#include "core/utils/VRCallbackWrapper.h"

#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGColor.h>

OSG_BEGIN_NAMESPACE;
template<>
struct VRCallbackWrapper<PyObject*> {
    VRCallbackWrapper() {}
    virtual ~VRCallbackWrapper() {}

    template<typename T>
    PyObject* convert(const T& t) { return VRPyTypeCaster::cast<T>(t); }

    virtual bool execute(void* obj, const vector<PyObject*>& params, PyObject*& result) = 0;
};

template<> PyObject* VRCallbackWrapper<PyObject*>::convert(const int& t) { return VRPyTypeCaster::cast(t); }
template<> PyObject* VRCallbackWrapper<PyObject*>::convert(const string& t) { return VRPyTypeCaster::cast(t); }
template<> PyObject* VRCallbackWrapper<PyObject*>::convert(const bool& t) { return VRPyTypeCaster::cast(t); }
template<> PyObject* VRCallbackWrapper<PyObject*>::convert(const VREntityPtr& t) { return VRPyTypeCaster::cast(t); }
OSG_END_NAMESPACE;

using namespace OSG;

typedef PyObject* PyObjectPtr;
template<> string typeName(const PyObjectPtr& o) {
    PyTypeObject* type = o->ob_type;
    if (!type) return "undefined";
    return type->tp_name;
}

template<> bool toValue<bool>(PyObject* o, bool& v) { if (!PyBool_Check(o)) return 0; v = PyInt_AsLong(o); return 1; }
template<> bool toValue<int>(PyObject* o, int& v) { if (!PyInt_Check(o)) return 0; v = PyInt_AsLong(o); return 1; }
template<> bool toValue<float>(PyObject* o, float& v) { if (!PyFloat_Check(o)) return 0; v = PyFloat_AsDouble(o); return 1; }
template<> bool toValue<string>(PyObject* o, string& v) { if (!PyString_Check(o)) return 0; v = PyString_AsString(o); return 1; }
template<> bool toValue<VRLightBeaconPtr>(PyObject* o, VRLightBeaconPtr& v) { if (!VRPyLightBeacon::check(o)) return 0; v = ((VRPyLightBeacon*)o)->objPtr; return 1; }
template<> bool toValue<VREntityPtr>(PyObject* o, VREntityPtr& v) { if (!VRPyEntity::check(o)) return 0; v = ((VRPyEntity*)o)->objPtr; return 1; }

bool PyVec_Check(PyObject* o, int N, char type) {
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

template<> bool toValue<Color4f>(PyObject* o, Color4f& v) { if (!PyVec_Check(o, 4, 'f')) return 0; v = VRPyBase::parseVec4fList(o); return 1; }
template<> bool toValue<Vec3f>(PyObject* o, Vec3f& v) { if (!PyVec_Check(o, 3, 'f')) return 0; v = VRPyBase::parseVec3fList(o); return 1; }


