#include "VRLight.h"
#include "core/utils/toString.h"
#include "core/utils/VRStorage_template.h"
#include "core/scene/VRScene.h"
#include "core/scene/VRSceneManager.h"
#include "core/objects/OSGObject.h"
#include "core/objects/object/OSGCore.h"
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
    DirectionalLightMTRecPtr d_light = DirectionalLight::create();
    PointLightMTRecPtr p_light = PointLight::create();
    SpotLightMTRecPtr s_light = SpotLight::create();

    this->d_light = OSGCore::create(d_light);
    this->p_light = OSGCore::create(p_light);
    this->s_light = OSGCore::create(s_light);

    d_light->setDirection(Vec3f(0,0,1));

    s_light->setDirection(Vec3f(0,0,-1));
    s_light->setSpotCutOff(Pi/6.f);
    s_light->setSpotExponent(3.f);

    ssme = SimpleShadowMapEngine::create();
    setShadowColor(Color4f(0.1f, 0.1f, 0.1f, 1.0f));
    shadowType = "4096";

    setCore(OSGCore::create(p_light), "Light");
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
    regStorageSetupFkt( VRFunction<int>::create("light setup", boost::bind(&VRLight::setup, this)) );
    regStorageSetupAfterFkt( VRFunction<int>::create("light setup after", boost::bind(&VRLight::setup_after, this)) );

    // test scene
    //shadow_test_scene* sts = new shadow_test_scene();
    //addChild(sts->rootNode);
}

VRLight::~VRLight() {}

VRLightPtr VRLight::ptr() { return static_pointer_cast<VRLight>( shared_from_this() ); }
VRLightPtr VRLight::create(string name) {
    auto l = shared_ptr<VRLight>(new VRLight(name) );
    VRSceneManager::getCurrent()->addLight(l);
    return l;
}

void VRLight::setup() {
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

    setup_after(); // TODO: deffered shading needs to have the light beacon before adding the node!?!
}

void VRLight::setup_after() {
    auto root = VRSceneManager::getCurrent()->getRoot();
    VRObjectPtr tmp = getRoot()->find(beacon_name);
    if (tmp) setBeacon( static_pointer_cast<VRLightBeacon>(tmp) );
    else cout << "  !! could not find light beacon: " << root << " " << this << endl;
}

void VRLight::setType(string type) {
    lightType = type;
    if (type == "point") setPointlight();
    if (type == "directional") setDirectionallight();
    if (type == "spot") setSpotlight();
}

void VRLight::setBeacon(VRLightBeaconPtr b) {
    beacon = b;
    b->setLight( ptr() );
    dynamic_pointer_cast<Light>(d_light->core)->setBeacon(b->getNode()->node);
    dynamic_pointer_cast<Light>(p_light->core)->setBeacon(b->getNode()->node);
    dynamic_pointer_cast<Light>(s_light->core)->setBeacon(b->getNode()->node);
}

void VRLight::setLightDiffColor(Color4f c) {
    lightDiffuse = c;
    dynamic_pointer_cast<Light>(d_light->core)->setDiffuse(c);
    dynamic_pointer_cast<Light>(p_light->core)->setDiffuse(c);
    dynamic_pointer_cast<Light>(s_light->core)->setDiffuse(c);
}

Color4f VRLight::getLightDiffColor() { return lightDiffuse; }

void VRLight::setLightAmbColor(Color4f c) {
    lightAmbient = c;
    dynamic_pointer_cast<Light>(d_light->core)->setAmbient(c);
    dynamic_pointer_cast<Light>(p_light->core)->setAmbient(c);
    dynamic_pointer_cast<Light>(s_light->core)->setAmbient(c);
}

Color4f VRLight::getLightAmbColor() { return lightAmbient; }

void VRLight::setLightSpecColor(Color4f c) {
    lightSpecular = c;
    dynamic_pointer_cast<Light>(d_light->core)->setSpecular(c);
    dynamic_pointer_cast<Light>(p_light->core)->setSpecular(c);
    dynamic_pointer_cast<Light>(s_light->core)->setSpecular(c);
}

Color4f VRLight::getLightSpecColor() { return lightSpecular; }

void VRLight::setShadows(bool b) {
    shadows = b;
    if (b) {
        dynamic_pointer_cast<Light>(d_light->core)->setLightEngine(ssme);
        dynamic_pointer_cast<Light>(p_light->core)->setLightEngine(ssme);
        dynamic_pointer_cast<Light>(s_light->core)->setLightEngine(ssme);
        auto bb = getBoundingBox(); // update osg volume
    } else {
        dynamic_pointer_cast<Light>(d_light->core)->setLightEngine(0);
        dynamic_pointer_cast<Light>(p_light->core)->setLightEngine(0);
        dynamic_pointer_cast<Light>(s_light->core)->setLightEngine(0);
    }
}

bool VRLight::getShadows() { return shadows; }

void VRLight::setShadowColor(Color4f c) {
    shadowColor = c;
    ssme->setShadowColor(c);
}

Color4f VRLight::getShadowColor() { return shadowColor; }

void VRLight::setOn(bool b) {
    on = b;
    dynamic_pointer_cast<Light>(d_light->core)->setOn(b);
    dynamic_pointer_cast<Light>(p_light->core)->setOn(b);
    dynamic_pointer_cast<Light>(s_light->core)->setOn(b);
}

bool VRLight::isOn() { return on; }

void VRLight::setAttenuation(Vec3f a) {
    attenuation = a;
    dynamic_pointer_cast<Light>(d_light->core)->setConstantAttenuation(a[0]);
    dynamic_pointer_cast<Light>(d_light->core)->setLinearAttenuation(a[1]);
    dynamic_pointer_cast<Light>(d_light->core)->setQuadraticAttenuation(a[2]);
    dynamic_pointer_cast<PointLight>(p_light->core)->setAttenuation(a[0], a[1], a[2]);
    dynamic_pointer_cast<SpotLight>(s_light->core)->setAttenuation(a[0], a[1], a[2]);
}

Vec3f VRLight::getAttenuation() { return attenuation; }

void VRLight::setShadowType(string t) {
    shadowType = t;
    setup();
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

VRLightBeaconWeakPtr VRLight::getBeacon() { return beacon; }

void VRLight::setPointlight() { switchCore(p_light); }
void VRLight::setSpotlight() { switchCore(s_light); }
void VRLight::setDirectionallight() { switchCore(d_light); }

LightMTRecPtr VRLight::getLightCore() { return dynamic_pointer_cast<Light>(getCore()->core); }
string VRLight::getLightType() { return lightType; };

OSG_END_NAMESPACE;
