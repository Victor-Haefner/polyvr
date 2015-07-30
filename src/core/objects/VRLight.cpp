#include "VRLight.h"
#include "core/utils/toString.h"
#include "core/utils/VRStorage_template.h"
#include "VRLightBeacon.h"

#include <OpenSG/OSGShadowStage.h>
#include <OpenSG/OSGImage.h>
#include <OpenSG/OSGTextureObjChunk.h>
#include <OpenSG/OSGTextureEnvChunk.h>
#include <OpenSG/OSGSimpleMaterial.h>
#include <OpenSG/OSGPointLight.h>
#include <OpenSG/OSGDirectionalLight.h>
#include <OpenSG/OSGSpotLight.h>
#include <OpenSG/OSGSimpleShadowMapEngine.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

VRLight::VRLight(string name) : VRObject(name) {
    d_light = DirectionalLight::create();
    p_light = PointLight::create();
    s_light = SpotLight::create();

    d_light->setDirection(Vec3f(0,0,1));

    s_light->setDirection(Vec3f(0,0,-1));
    s_light->setSpotCutOff(Pi/6.f);
    s_light->setSpotExponent(3.f);

    ssme = SimpleShadowMapEngine::create();
    setShadowColor(Color4f(0.1f, 0.1f, 0.1f, 1.0f));
    setShadowType("4096");

    setCore(p_light, "Light");
    lightType = "point";
    attenuation = Vec3f(p_light->getConstantAttenuation(), p_light->getLinearAttenuation(), p_light->getQuadraticAttenuation());

    setLightDiffColor(Color4f(1,1,1,1));
    setLightAmbColor(Color4f(.3,.3,.3,1));
    setLightSpecColor(Color4f(.1,.1,.1,1));

    store("on", &on);
    store("lightType", &lightType);
    store("shadow", &shadows);
    store("shadowType", &shadowType);
    store("diffuse", &lightDiffuse);
    store("ambient", &lightAmbient);
    store("specular", &lightSpecular);
    store("shadowColor", &shadowColor);
    storeObjName("beacon", &beacon, &beacon_name);

    // test scene
    //shadow_test_scene* sts = new shadow_test_scene();
    //addChild(sts->rootNode);
}

VRLight::~VRLight() {
    d_light = 0;
    p_light = 0;
    s_light = 0;
    //if (beacon) delete beacon; // should be deleted system
}

void VRLight::update() {
    ssme = SimpleShadowMapEngine::create();
    setType(lightType);
    setShadows(shadows);
    setShadowColor(shadowColor);
    ssme->setWidth (toInt(shadowType));
    ssme->setHeight(toInt(shadowType));
    setLightDiffColor(lightDiffuse);
    setLightAmbColor(lightAmbient);
    setLightSpecColor(lightSpecular);

    setOn(on);

    VRObject* tmp = getRoot()->find(beacon_name);
    if (tmp) setBeacon((VRLightBeacon*)tmp);
}

void VRLight::setType(string type) {
    lightType = type;
    if (type == "point") setPointlight();
    if (type == "directional") setDirectionallight();
    if (type == "spot") setSpotlight();
}

void VRLight::setBeacon(VRLightBeacon* b) {
    if (beacon && b != beacon) delete beacon;
    beacon = b;

    b->setLight(this);
    d_light->setBeacon(beacon->getNode());
    p_light->setBeacon(beacon->getNode());
    s_light->setBeacon(beacon->getNode());
}

void VRLight::setLightDiffColor(Color4f c) {
    lightDiffuse = c;
    d_light->setDiffuse(c);
    p_light->setDiffuse(c);
    s_light->setDiffuse(c);
}

Color4f VRLight::getLightDiffColor() { return lightDiffuse; }

void VRLight::setLightAmbColor(Color4f c) {
    lightAmbient = c;
    d_light->setAmbient(c);
    p_light->setAmbient(c);
    s_light->setAmbient(c);
}

Color4f VRLight::getLightAmbColor() { return lightAmbient; }

void VRLight::setLightSpecColor(Color4f c) {
    lightSpecular = c;
    d_light->setSpecular(c);
    p_light->setSpecular(c);
    s_light->setSpecular(c);
}

Color4f VRLight::getLightSpecColor() { return lightSpecular; }

void VRLight::setShadows(bool b) {
    shadows = b;
    if (b) {
        p_light->setLightEngine(ssme);
        d_light->setLightEngine(ssme);
        s_light->setLightEngine(ssme);

        OSG::Vec3f mi,ma; // update osg volume
        getNode()->updateVolume();
        getNode()->getVolume   ().getBounds( mi, ma );
    } else {
        p_light->setLightEngine(0);
        d_light->setLightEngine(0);
        s_light->setLightEngine(0);
    }
}

bool VRLight::getShadows() { return shadows; }

void VRLight::setShadowColor(Color4f c) {
    shadowColor = c;
    ssme->setShadowColor(c);
}

Color4f VRLight::getShadowColor() { return shadowColor; }

void VRLight::saveContent(xmlpp::Element* e) {
    VRObject::saveContent(e);
    VRStorage::save(e);
}

void VRLight::loadContent(xmlpp::Element* e) {
    VRObject::loadContent(e);
    VRStorage::load(e);
    update();
}

void VRLight::setOn(bool b) {
    on = b;
    d_light->setOn(b);
    p_light->setOn(b);
    s_light->setOn(b);
}

bool VRLight::isOn() { return on; }

void VRLight::setAttenuation(Vec3f a) {
    attenuation = a;
    d_light->setConstantAttenuation(a[0]);
    d_light->setLinearAttenuation(a[1]);
    d_light->setQuadraticAttenuation(a[2]);
    p_light->setAttenuation(a[0], a[1], a[2]);
    s_light->setAttenuation(a[0], a[1], a[2]);
}

Vec3f VRLight::getAttenuation() { return attenuation; }

void VRLight::setShadowType(string t) {
    shadowType = t;
    update();
}

string VRLight::getShadowType() { return shadowType; }

vector<string> VRLight::getTypes() {
    vector<string> s;
    s.push_back("point");
    s.push_back("directional");
    s.push_back("spot");
    return s;
}

vector<string> VRLight::getShadowTypes() {
    vector<string> s;
    s.push_back("128");
    s.push_back("256");
    s.push_back("512");
    s.push_back("1024");
    s.push_back("2048");
    s.push_back("4096");
    s.push_back("8192");
    s.push_back("16384");
    s.push_back("32768");
    s.push_back("65536");
    return s;
}

vector<string> VRLight::getTypeParameter(string type) {
    static bool init = false;
    static map<string, vector<string> > params;

    if (!init) {
        init = true;
        params["Point light"] = vector<string>();
        params["Directional light"] = vector<string>();
        params["Spot light"] = vector<string>();

        params["Spot light"].push_back("Spot Cut Off");
        params["Spot light"].push_back("Spot Cut Off Deg");
        params["Spot light"].push_back("Spot Exponent");
    }

    if (params.count(type)) return params[type];
    else return vector<string>();
}

// IDEE: licht sucht ob beacon schon da ist, danach sucht beacon ob licht schon da ist.. je nachdem wer wann erstellt wird..

VRLightBeacon* VRLight::getBeacon() { return beacon; }

void VRLight::setPointlight() { switchCore(p_light); }
void VRLight::setSpotlight() { switchCore(s_light); }
void VRLight::setDirectionallight() { switchCore(d_light); }

LightRecPtr VRLight::getLightCore() { return dynamic_pointer_cast<Light>(getCore()); }
string VRLight::getLightType() { return lightType; };

OSG_END_NAMESPACE;
