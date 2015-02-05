#include "VRLightBeacon.h"
#include "VRLight.h"
#include "core/objects/material/VRMaterial.h"
#include <OpenSG/OSGSimpleMaterial.h>
#include <OpenSG/OSGSimpleGeometry.h>        // Methods to create simple geos.
#include <libxml++/nodes/element.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

VRMaterial* getLightGeoMat() {
    static VRMaterial* mat = 0;
    if (mat == 0) {
        mat = new VRMaterial("light_geo_mat");
        mat->setAmbient(Color3f(0.7, 0.7, 0.7));
        mat->setDiffuse(Color3f(0.9, 0.9, 0.9));
        mat->setSpecular(Color3f(0.4, 0.4, 0.4));
        mat->setTransparency(0.3);
        mat->setLit(false);
        //mat->setDiffuse(Color4f(0,0,1,1));
    }
    return mat;
}

VRLightBeacon::VRLightBeacon(string name) : VRTransform(name) {
    type = "LightBeacon";
    light = 0;
    lightGeo = 0;

    GeometryRecPtr lightGeo_ = makeSphereGeo(2,0.1);
    lightGeo = makeNodeFor(lightGeo_);
    lightGeo->setTravMask(0);
    lightGeo_->setMaterial(getLightGeoMat()->getMaterial());
    addChild(lightGeo);

    getAll().push_back(this);
}

VRLightBeacon::~VRLightBeacon() {
    ;//TODO: remove from getAll vector
}

void VRLightBeacon::showLightGeo(bool b) {
    if (b) lightGeo->setTravMask(0xffffffff);
    else lightGeo->setTravMask(0);
}

VRLight* VRLightBeacon::getLight() { return light; }
void VRLightBeacon::setLight(VRLight* l) { light = l; }

void VRLightBeacon::saveContent(xmlpp::Element* e) {
    VRTransform::saveContent(e);
    e->set_attribute("light", light->getName());
}

void VRLightBeacon::loadContent(xmlpp::Element* e) {
    VRTransform::loadContent(e);
    string lightName = e->get_attribute("light")->get_value();

    // try to find light!
    VRObject* tmp = this;
    while(tmp->getParent()) tmp = tmp->getParent();
    if (tmp) light = (VRLight*)tmp->find(lightName);
    if (light) light->setBeacon(this);
}

vector<VRLightBeacon*>& VRLightBeacon::getAll() {
    static vector<VRLightBeacon*> objs;
    return objs;
}

OSG_END_NAMESPACE;
