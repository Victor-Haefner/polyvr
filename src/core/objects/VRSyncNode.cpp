#include "VRSyncNode.h"
#include "VRLight.h"
#include "core/objects/OSGObject.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/OSGMaterial.h"
#include "core/utils/VRStorage_template.h"
#include <OpenSG/OSGMultiPassMaterial.h>
#include <OpenSG/OSGSimpleMaterial.h>
#include <OpenSG/OSGSimpleGeometry.h>        // Methods to create simple geos.

using namespace OSG;

template<> string typeName(const VRSyncNode& o) { return "SyncNode"; }

//VRMaterialPtr getLightGeoMat() {
//    VRMaterialPtr mat = VRMaterial::create("light_geo_mat");
//    mat->setAmbient(Color3f(0.7, 0.7, 0.7));
//    mat->setDiffuse(Color3f(0.9, 0.9, 0.9));
//    mat->setSpecular(Color3f(0.4, 0.4, 0.4));
//    mat->setTransparency(0.3);
//    mat->setLit(false);
//    return mat;
//}

VRSyncNode::VRSyncNode(string name) : VRTransform(name) {
    type = "SyncNode";
    lightGeo = 0;
//
//    GeometryMTRecPtr lightGeo_ = makeSphereGeo(2,0.1);
//    lightGeo_->setMaterial(getLightGeoMat()->getMaterial()->mat);
//
//    lightGeo = OSGObject::create( makeNodeFor(lightGeo_) );
//    lightGeo->node->setTravMask(0);
//    addChild(lightGeo);
//
//    storeObjName("light", &light, &light_name);
}

VRSyncNode::~VRSyncNode() {}

VRSyncNodePtr VRSyncNode::ptr() { return static_pointer_cast<VRSyncNode>( shared_from_this() ); }
VRSyncNodePtr VRSyncNode::create(string name) {
    auto p = shared_ptr<VRSyncNode>(new VRSyncNode(name) );
    getAll().push_back( p );
    return p;
}

VRObjectPtr VRSyncNode::copy(vector<VRObjectPtr> children) {
    VRSyncNodePtr beacon = VRSyncNode::create(getBaseName());
    //for (auto c : children) // TODO: connect to light, light may be duplicated?
    beacon->setVisible(isVisible());
    beacon->setPickable(isPickable());
    beacon->setMatrix(getMatrix());
    return beacon;
}

void VRSyncNode::showLightGeo(bool b) {
    if (b) lightGeo->node->setTravMask(0xffffffff);
    else lightGeo->node->setTravMask(0);
}

VRLightWeakPtr VRSyncNode::getLight() { return light; }
void VRSyncNode::setLight(VRLightPtr l) { light = l; }

vector<VRSyncNodeWeakPtr>& VRSyncNode::getAll() {
    static vector<VRSyncNodeWeakPtr> objs;
    return objs;
}
