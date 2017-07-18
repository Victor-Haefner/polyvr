#include "VRPyBaseT.h"
#include "VRPyBaseFactory.h"
#include "VRPyLightBeacon.h"
#include "VRPyBoundingbox.h"
#include "VRPyGeometry.h"
#include "VRPyMath.h"
#include "VRPyImage.h"
#include "addons/Semantics/Reasoning/VRPyOntology.h"
#include "addons/WorldGenerator/VRWorldGeneratorFwd.h"
#include "core/utils/VRCallbackWrapper.h"

#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGColor.h>

OSG_BEGIN_NAMESPACE;
template<>
struct VRCallbackWrapper<PyObject*> : VRCallbackWrapperBase {
    VRCallbackWrapper() {}
    virtual ~VRCallbackWrapper() {}

    template<typename T>
    PyObject* convert(const T& t) { return VRPyTypeCaster::cast<T>(t); }

    virtual bool execute(void* obj, const vector<PyObject*>& params, PyObject*& result) = 0;
};

template<> PyObject* VRCallbackWrapper<PyObject*>::convert(const int& t) { return VRPyTypeCaster::cast(t); }
template<> PyObject* VRCallbackWrapper<PyObject*>::convert(const float& t) { return VRPyTypeCaster::cast(t); }
template<> PyObject* VRCallbackWrapper<PyObject*>::convert(const string& t) { return VRPyTypeCaster::cast(t); }
template<> PyObject* VRCallbackWrapper<PyObject*>::convert(const bool& t) { return VRPyTypeCaster::cast(t); }
template<> PyObject* VRCallbackWrapper<PyObject*>::convert(const VREntityPtr& t) { return VRPyTypeCaster::cast(t); }
template<> PyObject* VRCallbackWrapper<PyObject*>::convert(const Vec2f& t) { return VRPyTypeCaster::cast(t); }
template<> PyObject* VRCallbackWrapper<PyObject*>::convert(const Vec3f& t) { return VRPyTypeCaster::cast(t); }
template<> PyObject* VRCallbackWrapper<PyObject*>::convert(const Vec3d& t) { return VRPyTypeCaster::cast(t); }
template<> PyObject* VRCallbackWrapper<PyObject*>::convert(const VRRoadPtr& t) { return VRPyTypeCaster::cast(t); }
template<> PyObject* VRCallbackWrapper<PyObject*>::convert(const VRRoadNetworkPtr& t) { return VRPyTypeCaster::cast(t); }
template<> PyObject* VRCallbackWrapper<PyObject*>::convert(const VRTerrainPtr& t) { return VRPyTypeCaster::cast(t); }
template<> PyObject* VRCallbackWrapper<PyObject*>::convert(const VRMaterialPtr& t) { return VRPyTypeCaster::cast(t); }
template<> PyObject* VRCallbackWrapper<PyObject*>::convert(const posePtr& t) { return VRPyTypeCaster::cast(t); }
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
template<> bool toValue<float>(PyObject* o, float& v) { if (!PyNumber_Check(o)) return 0; v = PyFloat_AsDouble(o); return 1; }
template<> bool toValue<double>(PyObject* o, double& v) { if (!PyNumber_Check(o)) return 0; v = PyFloat_AsDouble(o); return 1; }
template<> bool toValue<string>(PyObject* o, string& v) { if (!PyString_Check(o)) return 0; v = PyString_AsString(o); return 1; }
template<> bool toValue<VRLightBeaconPtr>(PyObject* o, VRLightBeaconPtr& v) { if (!VRPyLightBeacon::check(o)) return 0; v = ((VRPyLightBeacon*)o)->objPtr; return 1; }
template<> bool toValue<VREntityPtr>(PyObject* o, VREntityPtr& v) { if (!VRPyEntity::check(o)) return 0; v = ((VRPyEntity*)o)->objPtr; return 1; }
template<> bool toValue<VROntologyPtr>(PyObject* o, VROntologyPtr& v) { if (!VRPyOntology::check(o)) return 0; v = ((VRPyOntology*)o)->objPtr; return 1; }
template<> bool toValue<Boundingbox>(PyObject* o, Boundingbox& v) { if (!VRPyBoundingbox::check(o)) return 0; v = *((VRPyBoundingbox*)o)->objPtr; return 1; }
template<> bool toValue<VRObjectPtr>(PyObject* o, VRObjectPtr& v) { if (!VRPyObject::check(o)) return 0; v = ((VRPyObject*)o)->objPtr; return 1; }
template<> bool toValue<VRGeometryPtr>(PyObject* o, VRGeometryPtr& v) { if (!VRPyGeometry::check(o)) return 0; v = ((VRPyGeometry*)o)->objPtr; return 1; }
template<> bool toValue<VRTransformPtr>(PyObject* o, VRTransformPtr& v) { if (!VRPyTransform::check(o)) return 0; v = ((VRPyTransform*)o)->objPtr; return 1; }
template<> bool toValue<VRTexturePtr>(PyObject* o, VRTexturePtr& v) { if (!VRPyImage::check(o)) return 0; v = ((VRPyImage*)o)->objPtr; return 1; }

template<> bool toValue<VRAnimCbPtr>(PyObject* o, VRAnimCbPtr& v) {
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

template<> bool toValue<Color3f>(PyObject* o, Color3f& v) { if (!PyVec_Check(o, 3, 'f')) return 0; v = VRPyBase::parseVec3fList(o); return 1; }
template<> bool toValue<Color4f>(PyObject* o, Color4f& v) { if (!PyVec_Check(o, 4, 'f')) return 0; v = VRPyBase::parseVec4fList(o); return 1; }
template<> bool toValue<Vec2f>(PyObject* o, Vec2f& v) { if (!PyVec_Check(o, 2, 'f')) return 0; v = VRPyBase::parseVec2fList(o); return 1; }
template<> bool toValue<Vec3f>(PyObject* o, Vec3f& v) { if (!PyVec_Check(o, 3, 'f')) return 0; v = VRPyBase::parseVec3fList(o); return 1; }
template<> bool toValue<Vec4f>(PyObject* o, Vec4f& v) { if (!PyVec_Check(o, 4, 'f')) return 0; v = VRPyBase::parseVec4fList(o); return 1; }
template<> bool toValue<Vec3i>(PyObject* o, Vec3i& v) { if (!PyVec_Check(o, 3, 'i')) return 0; v = VRPyBase::parseVec3iList(o); return 1; }
template<> bool toValue<Vec4i>(PyObject* o, Vec4i& v) { if (!PyVec_Check(o, 4, 'i')) return 0; v = VRPyBase::parseVec4iList(o); return 1; }
template<> bool toValue<Vec2d>(PyObject* o, Vec2d& v) { if (!PyVec_Check(o, 2, 'f')) return 0; v = VRPyBase::parseVec2dList(o); return 1; }
template<> bool toValue<Vec3d>(PyObject* o, Vec3d& v) { if (!PyVec_Check(o, 3, 'f')) return 0; v = VRPyBase::parseVec3dList(o); return 1; }
template<> bool toValue<Vec4d>(PyObject* o, Vec4d& v) { if (!PyVec_Check(o, 4, 'f')) return 0; v = VRPyBase::parseVec4dList(o); return 1; }







