#include "VRPyTypeCaster.h"

#include "VRPyBoundingbox.h"
#include "VRPyGeometry.h"
#include "VRPySprite.h"
#include "VRPyStroke.h"
#include "VRPyMaterial.h"
#include "VRPyLod.h"
#include "VRPyClipPlane.h"
#include "VRPyLight.h"
#include "VRPyCamera.h"
#include "VRPyMenu.h"
#include "VRPyLightBeacon.h"
#include "VRPyAnnotationEngine.h"
#include "VRPyTextureRenderer.h"
#include "VRPyWaypoint.h"
#include "VRPyGeoPrimitive.h"
#include "VRPyPointCloud.h"
#include "VRPyJointTool.h"
#include "VRPyBaseT.h"
#include "VRPyMath.h"
#include "VRPySyncNode.h"

#include "core/objects/object/VRObject.h"
#include "core/setup/devices/VRDevice.h"
#include "addons/Semantics/Reasoning/VRPyOntology.h"
#include "addons/Semantics/VRSemanticsFwd.h"
#ifndef WITHOUT_CGAL
#include "addons/Engineering/CSG/VRPyCSG.h"
#endif

using namespace OSG;

VRPyTypeCaster::VRPyTypeCaster() {;}

template<> PyObject* VRPyTypeCaster::cast(const VRObjectPtr& obj) {
    if (obj == 0) Py_RETURN_NONE;

    string type = obj->getType();
    if (type == "Geometry") return VRPyBaseT<VRGeometry>::fromSharedPtr( static_pointer_cast<VRGeometry>(obj) );
    else if (type == "Transform") return VRPyBaseT<VRTransform>::fromSharedPtr( static_pointer_cast<VRTransform>(obj) );
    else if (type == "Object") return VRPyObject::fromSharedPtr( static_pointer_cast<VRObject>(obj) );
    else if (type == "Sprite") return VRPySprite::fromSharedPtr( static_pointer_cast<VRSprite>(obj) );
#ifndef WITHOUT_CGAL
    else if (type == "CSGGeometry") return VRPyCSGGeometry::fromSharedPtr( static_pointer_cast<CSGGeometry>(obj) );
#endif
    else if (type == "Stroke") return VRPyStroke::fromSharedPtr( static_pointer_cast<VRStroke>(obj) );
    else if (type == "Material") return VRPyMaterial::fromSharedPtr( static_pointer_cast<VRMaterial>(obj) );
    else if (type == "Lod") return VRPyLod::fromSharedPtr( static_pointer_cast<VRLod>(obj) );
    else if (type == "ClipPlane") return VRPyClipPlane::fromSharedPtr( static_pointer_cast<VRClipPlane>(obj) );
    else if (type == "Light") return VRPyLight::fromSharedPtr( static_pointer_cast<VRLight>(obj) );
    else if (type == "Camera") return VRPyCamera::fromSharedPtr( static_pointer_cast<VRCamera>(obj) );
    else if (type == "Menu") return VRPyMenu::fromSharedPtr( static_pointer_cast<VRMenu>(obj) );
    else if (type == "LightBeacon") return VRPyLightBeacon::fromSharedPtr( static_pointer_cast<VRLightBeacon>(obj) );
    else if (type == "TextureRenderer") return VRPyTextureRenderer::fromSharedPtr( static_pointer_cast<VRTextureRenderer>(obj) );
    else if (type == "AnnotationEngine") return VRPyAnnotationEngine::fromSharedPtr( static_pointer_cast<VRAnnotationEngine>(obj) );
    else if (type == "Waypoint") return VRPyWaypoint::fromSharedPtr( static_pointer_cast<VRWaypoint>(obj) );
    else if (type == "JointTool") return VRPyJointTool::fromSharedPtr( static_pointer_cast<VRJointTool>(obj) );
    else if (type == "Handle") return VRPyBaseT<VRGeometry>::fromSharedPtr( static_pointer_cast<VRGeometry>(obj) ); // TODO
    else if (type == "GeoPrimitive") return VRPyGeoPrimitive::fromSharedPtr( static_pointer_cast<VRGeoPrimitive>(obj) );
    else if (type == "PointCloud") return VRPyPointCloud::fromSharedPtr( static_pointer_cast<VRPointCloud>(obj) );
#ifndef WITHOUT_TCP
    else if (type == "SyncNode") return VRPySyncNode::fromSharedPtr( static_pointer_cast<VRSyncNode>(obj) );
#endif
    cout << "\nERROR in VRPyTypeCaster::cast object: " << type << " not handled!\n";

    return VRPyObject::fromSharedPtr(obj);
}

typedef void* voidPtr;
template<> PyObject* VRPyTypeCaster::cast(const voidPtr& v) { if (v) { PyObject* o = (PyObject*)v; Py_INCREF(o); return o; } else Py_RETURN_NONE; }
template<> PyObject* VRPyTypeCaster::cast(const int& i) { return PyInt_FromLong(i); }
template<> PyObject* VRPyTypeCaster::cast(const unsigned int& i) { return PyInt_FromLong(i); }
template<> PyObject* VRPyTypeCaster::cast(const short& s) { return PyInt_FromLong(s); }
template<> PyObject* VRPyTypeCaster::cast(const char& i) { return PyInt_FromLong(i); }
template<> PyObject* VRPyTypeCaster::cast(const size_t& i) { return PyInt_FromLong(i); }
template<> PyObject* VRPyTypeCaster::cast(const unsigned char& i) { return PyInt_FromLong(i); }
template<> PyObject* VRPyTypeCaster::cast(const double& d) { return PyFloat_FromDouble(d); }
template<> PyObject* VRPyTypeCaster::cast(const float& f) { return PyFloat_FromDouble(f); }
template<> PyObject* VRPyTypeCaster::cast(const string& s) { return PyString_FromString(s.c_str()); }
template<> PyObject* VRPyTypeCaster::cast(const bool& b) { if (b) Py_RETURN_TRUE; else Py_RETURN_FALSE; }
template<> PyObject* VRPyTypeCaster::cast(const Vec2d& b) { return toPyObject(b); }
template<> PyObject* VRPyTypeCaster::cast(const Vec3d& b) { return toPyObject(b); }
template<> PyObject* VRPyTypeCaster::cast(const Vec4d& b) { return VRPyBase::toPyTuple(b); }
template<> PyObject* VRPyTypeCaster::cast(const Pnt3d& b) { return toPyObject(Vec3d(b)); }
template<> PyObject* VRPyTypeCaster::cast(const Vec2i& b) { return VRPyBase::toPyTuple(b); }
template<> PyObject* VRPyTypeCaster::cast(const Vec3i& b) { return VRPyBase::toPyTuple(b); }
template<> PyObject* VRPyTypeCaster::cast(const Vec4i& b) { return VRPyBase::toPyTuple(b); }
template<> PyObject* VRPyTypeCaster::cast(const Color3f& b) { return VRPyBase::toPyTuple(Vec3d(b)); }
template<> PyObject* VRPyTypeCaster::cast(const Color4f& b) { return VRPyBase::toPyTuple(Vec4d(b)); }
//template<> PyObject* VRPyTypeCaster::cast(const Line& b) {}
template<> PyObject* VRPyTypeCaster::cast(const Boundingbox& b) { return VRPyBoundingbox::fromObject(b); }

template<> PyObject* VRPyTypeCaster::cast(const Line& l) {
    auto L = PyTuple_New(2);
    PyTuple_SetItem(L,0,VRPyBase::toPyTuple(Vec3d(l.getPosition())));
    PyTuple_SetItem(L,1,VRPyBase::toPyTuple(Vec3d(l.getDirection())));
    return L;
}

PyObject* VRPyTypeCaster::pack(const vector<PyObject*>& v) {
    auto l = PyList_New(v.size());
    for (unsigned int i=0; i<v.size(); i++) PyList_SetItem(l,i,v[i]);
    return l;
}

PyObject* VRPyTypeCaster::pack(const vector<vector<PyObject*>>& v) {
    auto l = PyList_New(v.size());
    for (unsigned int i=0; i<v.size(); i++) PyList_SetItem(l,i,pack(v[i]));
    return l;
}

PyObject* VRPyTypeCaster::pack(const vector< pair<PyObject*,PyObject*> >& v) {
    auto l = PyDict_New();
    for (auto i : v) PyDict_SetItem(l, i.first, i.second);
    return l;
}




