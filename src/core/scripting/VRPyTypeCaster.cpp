#include "VRPyTypeCaster.h"

#include "VRPyGeometry.h"
#include "VRPySprite.h"
#include "VRPyMaterial.h"
#include "VRPyLod.h"
#include "VRPyBaseT.h"
#include "core/objects/object/VRObject.h"
#include "addons/CSG/VRPyCSG.h"

VRPyTypeCaster::VRPyTypeCaster() {;}

PyObject* VRPyTypeCaster::cast(OSG::VRObject* obj) {
    if (obj == 0) Py_RETURN_NONE;

    string type = obj->getType();
    if (type == "Geometry") return VRPyGeometry::fromPtr((OSG::VRGeometry*)obj);
    else if (type == "Transform") return VRPyTransform::fromPtr((OSG::VRTransform*)obj);
    else if (type == "Object") return VRPyObject::fromPtr(obj);
    else if (type == "Sprite") return VRPySprite::fromPtr((OSG::VRSprite*)obj);
    else if (type == "CSGGeometry") return VRPyCSG::fromPtr((OSG::CSGApp::CSGGeometry*)obj);
    else if (type == "Sprite") return VRPySprite::fromPtr((OSG::VRSprite*)obj);
    else if (type == "Material") return VRPyMaterial::fromPtr((OSG::VRMaterial*)obj);
    else if (type == "Lod") return VRPyLod::fromPtr((OSG::VRLod*)obj);
    cout << "\nERROR in VRPyTypeCaster::cast: " << type << " not handled!\n";
    Py_RETURN_NONE;
}
