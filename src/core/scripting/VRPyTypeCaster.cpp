#include "VRPyTypeCaster.h"

#include "VRPyGeometry.h"
#include "VRPySprite.h"
#include "VRPyMaterial.h"
#include "VRPyLod.h"
#include "VRPyClipPlane.h"
#include "VRPyLight.h"
#include "VRPyCamera.h"
#include "VRPyMenu.h"
#include "VRPyBaseT.h"
#include "core/objects/object/VRObject.h"
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
    cout << "\nERROR in VRPyTypeCaster::cast: " << type << " not handled!\n";

    return VRPyObject::fromSharedPtr(obj);
    //Py_RETURN_NONE;
}
