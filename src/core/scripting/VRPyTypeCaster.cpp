#include "VRPyTypeCaster.h"

#include "VRPyGeometry.h"
#include "VRPySprite.h"
#include "VRPyMaterial.h"
#include "VRPyLod.h"
#include "VRPyClipPlane.h"
#include "VRPyLight.h"
#include "VRPyMenu.h"
#include "VRPyBaseT.h"
#include "core/objects/object/VRObject.h"
#ifndef _WIN32
#include "addons/Engineering/CSG/VRPyCSG.h"
#endif

VRPyTypeCaster::VRPyTypeCaster() {;}

PyObject* VRPyTypeCaster::cast(OSG::VRObject* obj) {
    if (obj == 0) Py_RETURN_NONE;

    string type = obj->getType();
    if (type == "Geometry") return VRPyGeometry::fromPtr((OSG::VRGeometry*)obj);
    else if (type == "Transform") return VRPyTransform::fromPtr((OSG::VRTransform*)obj);
    else if (type == "Object") return VRPyObject::fromPtr(obj);
    else if (type == "Sprite") return VRPySprite::fromPtr((OSG::VRSprite*)obj);
#ifndef _WIN32
    else if (type == "CSGGeometry") return VRPyCSG::fromPtr((OSG::CSGGeometry*)obj);
#endif
    else if (type == "Sprite") return VRPySprite::fromPtr((OSG::VRSprite*)obj);
    else if (type == "Material") return VRPyMaterial::fromPtr((OSG::VRMaterial*)obj);
    else if (type == "Lod") return VRPyLod::fromPtr((OSG::VRLod*)obj);
    else if (type == "ClipPlane") return VRPyClipPlane::fromPtr((OSG::VRClipPlane*)obj);
    else if (type == "Light") return VRPyLight::fromPtr((OSG::VRLight*)obj);
    else if (type == "Camera") return VRPyTransform::fromPtr((OSG::VRTransform*)obj); // TODO: add camera py type
    else if (type == "Menu") return VRPyMenu::fromPtr((OSG::VRMenu*)obj); // TODO: add camera py type
    cout << "\nERROR in VRPyTypeCaster::cast: " << type << " not handled!\n";
    Py_RETURN_NONE;
}
