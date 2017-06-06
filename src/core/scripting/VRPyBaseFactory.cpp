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

template<> void toValue<bool>(PyObject* o, bool& b) { b = PyInt_AsLong(o); }
template<> void toValue<int>(PyObject* o, int& i) { i = PyInt_AsLong(o); }
template<> void toValue<float>(PyObject* o, float& f) { f = PyFloat_AsDouble(o); }
template<> void toValue<string>(PyObject* o, string& s) { s = PyString_AsString(o); }
template<> void toValue<Color4f>(PyObject* o, Color4f& c) { c = VRPyBase::parseVec4fList(o); }
template<> void toValue<Vec3f>(PyObject* o, Vec3f& v) { v = VRPyBase::parseVec3fList(o); }
template<> void toValue<VRLightBeaconPtr>(PyObject* o, VRLightBeaconPtr& b) { b = ((VRPyLightBeacon*)o)->objPtr; }
template<> void toValue<VREntityPtr>(PyObject* o, VREntityPtr& e) { e = ((VRPyEntity*)o)->objPtr; }


