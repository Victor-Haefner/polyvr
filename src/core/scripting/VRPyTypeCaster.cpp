#include "VRPyTypeCaster.h"

#include "VRPyBoundingbox.h"
#include "VRPyGeometry.h"
#include "VRPySprite.h"
#include "VRPyMaterial.h"
#include "VRPyLod.h"
#include "VRPyClipPlane.h"
#include "VRPyLight.h"
#include "VRPyCamera.h"
#include "VRPyMenu.h"
#include "VRPyLightBeacon.h"
#include "VRPyTextureRenderer.h"
#include "VRPyWaypoint.h"
#include "VRPyGeoPrimitive.h"
#include "VRPyJointTool.h"
#include "VRPyBaseT.h"

#include "VRPyDevice.h"
#include "VRPyMobile.h"
#include "VRPyMouse.h"
#include "VRPyHaptic.h"
#include "VRPyMath.h"

#include "core/objects/object/VRObject.h"
#include "core/setup/devices/VRDevice.h"
#include "addons/Semantics/Reasoning/VRPyOntology.h"
#include "addons/Semantics/VRSemanticsFwd.h"
#include "addons/Engineering/CSG/VRPyCSG.h"

using namespace OSG;

VRPyTypeCaster::VRPyTypeCaster() {;}

template<> PyObject* VRPyTypeCaster::cast(const VRObjectPtr& obj) {
    if (obj == 0) Py_RETURN_NONE;

    string type = obj->getType();
    if (type == "Geometry") return VRPyGeometry::fromSharedPtr( static_pointer_cast<VRGeometry>(obj) );
    else if (type == "Transform") return VRPyTransform::fromSharedPtr( static_pointer_cast<VRTransform>(obj) );
    else if (type == "Object") return VRPyObject::fromSharedPtr( static_pointer_cast<VRObject>(obj) );
    else if (type == "Sprite") return VRPySprite::fromSharedPtr( static_pointer_cast<VRSprite>(obj) );
#ifndef _WIN32
    else if (type == "CSGGeometry") return VRPyCSG::fromSharedPtr( static_pointer_cast<CSGGeometry>(obj) );
#endif
    else if (type == "Sprite") return VRPySprite::fromSharedPtr( static_pointer_cast<VRSprite>(obj) );
    else if (type == "Material") return VRPyMaterial::fromSharedPtr( static_pointer_cast<VRMaterial>(obj) );
    else if (type == "Lod") return VRPyLod::fromSharedPtr( static_pointer_cast<VRLod>(obj) );
    else if (type == "ClipPlane") return VRPyClipPlane::fromSharedPtr( static_pointer_cast<VRClipPlane>(obj) );
    else if (type == "Light") return VRPyLight::fromSharedPtr( static_pointer_cast<VRLight>(obj) );
    else if (type == "Camera") return VRPyCamera::fromSharedPtr( static_pointer_cast<VRCamera>(obj) );
    else if (type == "Menu") return VRPyMenu::fromSharedPtr( static_pointer_cast<VRMenu>(obj) );
    else if (type == "LightBeacon") return VRPyLightBeacon::fromSharedPtr( static_pointer_cast<VRLightBeacon>(obj) );
    else if (type == "TextureRenderer") return VRPyTextureRenderer::fromSharedPtr( static_pointer_cast<VRTextureRenderer>(obj) );
    else if (type == "Waypoint") return VRPyWaypoint::fromSharedPtr( static_pointer_cast<VRWaypoint>(obj) );
    else if (type == "JointTool") return VRPyJointTool::fromSharedPtr( static_pointer_cast<VRJointTool>(obj) );
    else if (type == "Handle") return VRPyGeometry::fromSharedPtr( static_pointer_cast<VRGeometry>(obj) ); // TODO
    else if (type == "GeoPrimitive") return VRPyGeoPrimitive::fromSharedPtr( static_pointer_cast<VRGeoPrimitive>(obj) );
    cout << "\nERROR in VRPyTypeCaster::cast object: " << type << " not handled!\n";

    return VRPyObject::fromSharedPtr(obj);
}

template<> PyObject* VRPyTypeCaster::cast(const VRDevicePtr& dev) {
    if (!dev) Py_RETURN_NONE;

    string type = dev->getType();
    if (type == "mouse") return VRPyMouse::fromSharedPtr( static_pointer_cast<VRMouse>(dev) );
    else if (type == "server") return VRPyMobile::fromSharedPtr( static_pointer_cast<VRServer>(dev) );
    else if (type == "haptic") return VRPyHaptic::fromSharedPtr( static_pointer_cast<VRHaptic>(dev) );
    else if (type == "keyboard") return VRPyDevice::fromSharedPtr( dev );
    cout << "\nERROR in VRPyTypeCaster::cast device: " << type << " not handled!\n";
    return VRPyDevice::fromSharedPtr(dev);
}

template<> PyObject* VRPyTypeCaster::cast(const VRTransformPtr& e) { return VRPyTypeCaster::cast(dynamic_pointer_cast<VRObject>(e)); }
template<> PyObject* VRPyTypeCaster::cast(const VRGeometryPtr& e) { return VRPyTypeCaster::cast(dynamic_pointer_cast<VRObject>(e)); }
template<> PyObject* VRPyTypeCaster::cast(const VREntityPtr& e) { return VRPyEntity::fromSharedPtr(e); }
template<> PyObject* VRPyTypeCaster::cast(const int& i) { return PyInt_FromLong(i); }
template<> PyObject* VRPyTypeCaster::cast(const float& i) { return PyFloat_FromDouble(i); }
template<> PyObject* VRPyTypeCaster::cast(const string& s) { return PyString_FromString(s.c_str()); }
template<> PyObject* VRPyTypeCaster::cast(const bool& b) { if (b) Py_RETURN_TRUE; else Py_RETURN_FALSE; }
template<> PyObject* VRPyTypeCaster::cast(const Vec2d& b) { return toPyObject(b); }
template<> PyObject* VRPyTypeCaster::cast(const Vec3d& b) { return toPyObject(b); }
template<> PyObject* VRPyTypeCaster::cast(const Vec4d& b) { return VRPyBase::toPyTuple(b); }
template<> PyObject* VRPyTypeCaster::cast(const Boundingbox& b) { return VRPyBoundingbox::fromObject(b); }
template<> PyObject* VRPyTypeCaster::cast(const BoundingboxPtr& b) { return VRPyBoundingbox::fromSharedPtr(b); }

PyObject* VRPyTypeCaster::pack(const vector<PyObject*>& v) {
    auto l = PyList_New(v.size());
    for (int i=0; i<v.size(); i++) PyList_SetItem(l,i,v[i]);
    return l;
}




