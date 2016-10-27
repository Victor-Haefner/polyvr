#include "VRPyTypeCaster.h"

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

#include "core/objects/object/VRObject.h"
#include "core/setup/devices/VRDevice.h"

#ifndef _WIN32
#include "addons/Engineering/CSG/VRPyCSG.h"
#endif

VRPyTypeCaster::VRPyTypeCaster() {;}

PyObject* VRPyTypeCaster::cast(OSG::VRObjectPtr obj) {
    if (obj == 0) Py_RETURN_NONE;

    string type = obj->getType();
    if (type == "Geometry") return VRPyGeometry::fromSharedPtr( static_pointer_cast<OSG::VRGeometry>(obj) );
    else if (type == "Transform") return VRPyTransform::fromSharedPtr( static_pointer_cast<OSG::VRTransform>(obj) );
    else if (type == "Object") return VRPyObject::fromSharedPtr( static_pointer_cast<OSG::VRObject>(obj) );
    else if (type == "Sprite") return VRPySprite::fromSharedPtr( static_pointer_cast<OSG::VRSprite>(obj) );
#ifndef _WIN32
    else if (type == "CSGGeometry") return VRPyCSG::fromSharedPtr( static_pointer_cast<OSG::CSGGeometry>(obj) );
#endif
    else if (type == "Sprite") return VRPySprite::fromSharedPtr( static_pointer_cast<OSG::VRSprite>(obj) );
    else if (type == "Material") return VRPyMaterial::fromSharedPtr( static_pointer_cast<OSG::VRMaterial>(obj) );
    else if (type == "Lod") return VRPyLod::fromSharedPtr( static_pointer_cast<OSG::VRLod>(obj) );
    else if (type == "ClipPlane") return VRPyClipPlane::fromSharedPtr( static_pointer_cast<OSG::VRClipPlane>(obj) );
    else if (type == "Light") return VRPyLight::fromSharedPtr( static_pointer_cast<OSG::VRLight>(obj) );
    else if (type == "Camera") return VRPyCamera::fromSharedPtr( static_pointer_cast<OSG::VRCamera>(obj) );
    else if (type == "Menu") return VRPyMenu::fromSharedPtr( static_pointer_cast<OSG::VRMenu>(obj) );
    else if (type == "LightBeacon") return VRPyLightBeacon::fromSharedPtr( static_pointer_cast<OSG::VRLightBeacon>(obj) );
    else if (type == "TextureRenderer") return VRPyTextureRenderer::fromSharedPtr( static_pointer_cast<OSG::VRTextureRenderer>(obj) );
    else if (type == "Waypoint") return VRPyWaypoint::fromSharedPtr( static_pointer_cast<OSG::VRWaypoint>(obj) );
    else if (type == "JointTool") return VRPyJointTool::fromSharedPtr( static_pointer_cast<OSG::VRJointTool>(obj) );
    else if (type == "Handle") return VRPyGeometry::fromSharedPtr( static_pointer_cast<OSG::VRGeometry>(obj) ); // TODO
    else if (type == "GeoPrimitive") return VRPyGeoPrimitive::fromSharedPtr( static_pointer_cast<OSG::VRGeoPrimitive>(obj) );
    cout << "\nERROR in VRPyTypeCaster::cast object: " << type << " not handled!\n";

    return VRPyObject::fromSharedPtr(obj);
}

PyObject* VRPyTypeCaster::cast(OSG::VRDevicePtr dev) {
    if (!dev) Py_RETURN_NONE;

    string type = dev->getType();
    if (type == "mouse") return VRPyMouse::fromSharedPtr( static_pointer_cast<OSG::VRMouse>(dev) );
    else if (type == "mobile") return VRPyMobile::fromSharedPtr( static_pointer_cast<OSG::VRMobile>(dev) );
    else if (type == "haptic") return VRPyHaptic::fromSharedPtr( static_pointer_cast<OSG::VRHaptic>(dev) );
    else if (type == "keyboard") return VRPyDevice::fromSharedPtr( dev );
    cout << "\nERROR in VRPyTypeCaster::cast device: " << type << " not handled!\n";
    return VRPyDevice::fromSharedPtr(dev);
}
