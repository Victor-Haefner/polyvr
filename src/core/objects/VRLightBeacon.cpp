#include "VRLightBeacon.h"
#include "VRLight.h"
#include "core/objects/OSGObject.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/OSGMaterial.h"
#include "core/utils/VRStorage_template.h"
#include <OpenSG/OSGMultiPassMaterial.h>
#include <OpenSG/OSGSimpleMaterial.h>
#include <OpenSG/OSGSimpleGeometry.h>        // Methods to create simple geos.

using namespace OSG;

VRMaterialPtr getLightGeoMat() {
    VRMaterialPtr mat = VRMaterial::create("light_geo_mat");
    mat->setAmbient(Color3f(0.7, 0.7, 0.7));
    mat->setDiffuse(Color3f(0.9, 0.9, 0.9));
    mat->setSpecular(Color3f(0.4, 0.4, 0.4));
    mat->setTransparency(0.3);
    mat->setLit(false);
    return mat;
}

VRLightBeacon::VRLightBeacon(string name) : VRTransform(name) {
    type = "LightBeacon";
    lightGeo = 0;

    GeometryMTRecPtr lightGeo_ = makeSphereGeo(2,0.1);
    lightGeo_->setMaterial(getLightGeoMat()->getMaterial()->mat);

    lightGeo = OSGObject::create( makeNodeFor(lightGeo_) );
    lightGeo->node->setTravMask(0);
    addChild(lightGeo);

    storeObjName("light", &light, &light_name);
}

VRLightBeacon::~VRLightBeacon() {}

VRLightBeaconPtr VRLightBeacon::ptr() { return static_pointer_cast<VRLightBeacon>( shared_from_this() ); }
VRLightBeaconPtr VRLightBeacon::create(string name) {
    auto p = shared_ptr<VRLightBeacon>(new VRLightBeacon(name) );
    getAll().push_back( p );
    return p;
}

VRObjectPtr VRLightBeacon::copy(vector<VRObjectPtr> children) {
    VRLightBeaconPtr beacon = VRLightBeacon::create(getBaseName());
    //for (auto c : children) // TODO: connect to light, light may be duplicated?
    beacon->setVisible(isVisible());
    beacon->setPickable(isPickable());
    beacon->setMatrix(getMatrix());
    return beacon;
}

void VRLightBeacon::showLightGeo(bool b) {
    if (b) lightGeo->node->setTravMask(0xffffffff);
    else lightGeo->node->setTravMask(0);
}

VRLightWeakPtr VRLightBeacon::getLight() { return light; }
void VRLightBeacon::setLight(VRLightPtr l) { light = l; }

vector<VRLightBeaconWeakPtr>& VRLightBeacon::getAll() {
    static vector<VRLightBeaconWeakPtr> objs;
    return objs;
}
