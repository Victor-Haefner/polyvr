#include "VRLight.h"
#include "core/utils/toString.h"
#include "core/utils/VRStorage_template.h"
#include "core/objects/material/VRTexture.h"
#include "core/scene/VRScene.h"
#ifndef WITHOUT_IES
#include "core/scene/import/VRIES.h"
#endif
#include "core/objects/OSGObject.h"
#include "core/objects/object/OSGCore.h"
#include "VRLightBeacon.h"

#include <OpenSG/OSGNode.h>
#include <OpenSG/OSGShadowStage.h>
#include <OpenSG/OSGImage.h>
#include <OpenSG/OSGTextureObjChunk.h>
#include <OpenSG/OSGTextureEnvChunk.h>
#include <OpenSG/OSGSimpleMaterial.h>
#include <OpenSG/OSGPointLight.h>
#include <OpenSG/OSGDirectionalLight.h>
#include <OpenSG/OSGSpotLight.h>
#ifndef OSG_OGL_ES2
#include <OpenSG/OSGSimpleShadowMapEngine.h>
#include <OpenSG/OSGShaderShadowMapEngine.h>
#include <OpenSG/OSGTrapezoidalShadowMapEngine.h>
#endif
#include <OpenSG/OSGBoxVolume.h>

using namespace OSG;


VRLight::VRLight(string name) : VRObject(name) {
    DirectionalLightMTRecPtr d_light = DirectionalLight::create();
    PointLightMTRecPtr p_light = PointLight::create();
    SpotLightMTRecPtr s_light = SpotLight::create();
    PointLightMTRecPtr ph_light = PointLight::create();

    this->d_light = OSGCore::create(d_light);
    this->p_light = OSGCore::create(p_light);
    this->s_light = OSGCore::create(s_light);
    this->ph_light = OSGCore::create(ph_light);

    d_light->setDirection(Vec3f(0,0,1));

    s_light->setDirection(Vec3f(0,0,-1));
    s_light->setSpotCutOff(Pi/6.f);
    s_light->setSpotExponent(3.f);

    setCore(OSGCore::create(p_light), "Light");
    attenuation = Vec3d(p_light->getConstantAttenuation(), p_light->getLinearAttenuation(), p_light->getQuadraticAttenuation());

    setDiffuse(Color4f(1,1,1,1));
    setAmbient(Color4f(.3,.3,.3,1));
    setSpecular(Color4f(.1,.1,.1,1));
    setShadowColor(Color4f(0.1,0.1,0.1,1));

    store("on", &on);
    store("lightType", &lightType);
    store("shadow", &shadows);
    store("shadowMapRes", &shadowMapRes);
    store("diffuse", &lightDiffuse);
    store("ambient", &lightAmbient);
    store("specular", &lightSpecular);
    store("shadowColor", &shadowColor);
    store("photometricMap", &photometricMapPath);
    store("shadowVolume", &shadowVolume);
    storeObjName("beacon", &beacon, &beacon_name);
    regStorageSetupFkt( VRStorageCb::create("light setup", bind(&VRLight::setup, this, _1)) );
    setupAfterCb = VRUpdateCb::create("light setup after", bind(&VRLight::setup_after, this));
    regStorageSetupAfterFkt( setupAfterCb );

    // test scene
    //shadow_test_scene* sts = new shadow_test_scene();
    //addChild(sts->rootNode);
}

VRLight::~VRLight() {
    VRScene::getCurrent()->subLight( getID() );
}

VRLightPtr VRLight::ptr() { return static_pointer_cast<VRLight>( shared_from_this() ); }
VRLightPtr VRLight::create(string name) {
    auto l = VRLightPtr(new VRLight(name) );
    VRScene::getCurrent()->addLight(l);
    //cout << "VRLight::create " << l << " " << l->getName() << " deferred " << l->deferred << endl;
    return l;
}

void VRLight::setup(VRStorageContextPtr context) {
    setType(lightType);
    setShadows(shadows);
    setShadowColor(shadowColor);
    setShadowVolume(shadowVolume);
    setDiffuse(lightDiffuse);
    setAmbient(lightAmbient);
    setSpecular(lightSpecular);
    setOn(on);
    loadPhotometricMap(photometricMapPath);

    setup_after(); // TODO: deferred shading needs to have the light beacon before adding the node!?!
}

void VRLight::setup_after() {
    //auto root = VRScene::getCurrent()->getRoot();
    auto root = getRoot();
    if (!root) { cout << "  !! could not find root for light beacon: " << root << " " << this << endl; return; }
    else cout << "VRLight::setup_after, root: " << root.get() << ", light: " << this << endl;
    VRObjectPtr tmp = root->find(beacon_name);
    if (tmp) setBeacon( static_pointer_cast<VRLightBeacon>(tmp) );
    else cout << "  !! could not find light beacon: " << root << " " << this << endl;
    cout << " done" << endl;
}

VRObjectPtr VRLight::copy(vector<VRObjectPtr> children) {
    VRLightPtr light = VRLight::create(getBaseName());
    if (auto b = getBeacon()) {
        for (auto c : children) {
            auto tmpv = c->findAll(b->getBaseName());
            if (tmpv.size() > 0) {
                auto tmp = tmpv[0];
                if (tmp) {
                    light->setBeacon( static_pointer_cast<VRLightBeacon>(tmp) );
                    break;
                }
            }
        }
    }
    light->setVisible(isVisible());
    light->setPickable(isPickable());
    light->setOn(on);
    light->setDiffuse(lightDiffuse);
    light->setAmbient(lightAmbient);
    light->setSpecular(lightSpecular);
    light->setPhotometricMap(photometricMap);
    light->setType(lightType);
    light->setDeferred(deferred);
    light->toggleShadows(shadows);
    light->setShadowMapRes(shadowMapRes);
    light->setShadowNearFar(shadowNearFar);
    light->setShadowColor(shadowColor);
    light->setAttenuation(attenuation);
    return light;
}

void VRLight::setType(string type) {
    lightType = type;
    if (type == "point") setPointlight();
    if (type == "directional") setDirectionallight();
    if (type == "spot") setSpotlight();
    if (type == "photometric") setPhotometriclight();
}

void VRLight::setShadowParams(bool b, int res, Color4f c, Vec2d nf) {
    cout << "VRLight::setShadowParams " << deferred << endl;
    setShadows(b);
    setShadowMapRes(res);
    setShadowColor(c);
    setShadowNearFar(nf);
}

void VRLight::setBeacon(VRLightBeaconPtr b) {
    beacon = b;
    b->setLight( ptr() );
    dynamic_pointer_cast<Light>(d_light->core)->setBeacon(b->getNode()->node);
    dynamic_pointer_cast<Light>(p_light->core)->setBeacon(b->getNode()->node);
    dynamic_pointer_cast<Light>(s_light->core)->setBeacon(b->getNode()->node);
    dynamic_pointer_cast<Light>(ph_light->core)->setBeacon(b->getNode()->node);
}

void VRLight::setDiffuse(Color4f c) {
    lightDiffuse = c;
    dynamic_pointer_cast<Light>(d_light->core)->setDiffuse(c);
    dynamic_pointer_cast<Light>(p_light->core)->setDiffuse(c);
    dynamic_pointer_cast<Light>(s_light->core)->setDiffuse(c);
    dynamic_pointer_cast<Light>(ph_light->core)->setDiffuse(c);
    updateDeferredLight();
}

Color4f VRLight::getDiffuse() { return lightDiffuse; }

void VRLight::setAmbient(Color4f c) {
    lightAmbient = c;
    dynamic_pointer_cast<Light>(d_light->core)->setAmbient(c);
    dynamic_pointer_cast<Light>(p_light->core)->setAmbient(c);
    dynamic_pointer_cast<Light>(s_light->core)->setAmbient(c);
    dynamic_pointer_cast<Light>(ph_light->core)->setAmbient(c);
    updateDeferredLight();
}

Color4f VRLight::getAmbient() { return lightAmbient; }

void VRLight::setSpecular(Color4f c) {
    lightSpecular = c;
    dynamic_pointer_cast<Light>(d_light->core)->setSpecular(c);
    dynamic_pointer_cast<Light>(p_light->core)->setSpecular(c);
    dynamic_pointer_cast<Light>(s_light->core)->setSpecular(c);
    dynamic_pointer_cast<Light>(ph_light->core)->setSpecular(c);
}

Color4f VRLight::getSpecular() { return lightSpecular; }

void VRLight::setDeferred(bool b) {
    deferred = b;
    setShadows(shadows);
}

void VRLight::setupShadowEngines() {
#ifndef OSG_OGL_ES2
    ssme = SimpleShadowMapEngine::create();
    gsme = ShaderShadowMapEngine::create();
    ptsme = TrapezoidalShadowMapEngine::create();
    stsme = TrapezoidalShadowMapEngine::create();
    setShadowColor(shadowColor);

    ssme->editShadowTravMask() = 16;
    gsme->editShadowTravMask() = 16;
    ptsme->editShadowTravMask() = 16;
    stsme->editShadowTravMask() = 16;

    ssme->setWidth (shadowMapRes);
    ssme->setHeight(shadowMapRes);
    gsme->setWidth (shadowMapRes);
    gsme->setHeight(shadowMapRes);
    ptsme->setWidth (shadowMapRes);
    ptsme->setHeight(shadowMapRes);
    stsme->setWidth (shadowMapRes);
    stsme->setHeight(shadowMapRes);

    ssme->setOffsetFactor( 4.5f);
    ssme->setOffsetBias  (16.f );

    gsme->setOffsetFactor( 4.5f);
    gsme->setOffsetBias  (16.f );
    gsme->setForceTextureUnit(3);
    ptsme->setOffsetFactor( 4.5f);
    ptsme->setOffsetBias  (16.f );
    ptsme->setForceTextureUnit(3);
    stsme->setOffsetFactor( 4.5f);
    stsme->setOffsetBias  (16.f );
    stsme->setForceTextureUnit(3);
#endif
}

bool VRLight::getShadows() { return shadows; }
Color4f VRLight::getShadowColor() { return shadowColor; }

void VRLight::toggleShadows(bool b) { // TODO: optimize this
    if (shadows == b) return;
    setShadows(b);
}

void VRLight::setShadows(bool b) {
#ifndef OSG_OGL_ES2
    if (!ssme) setupShadowEngines();
    shadows = b;

    auto setShadowEngine = [&](OSGCorePtr l, ShadowMapEngineMTRecPtr e) {
        dynamic_pointer_cast<Light>(l->core)->setLightEngine(e);
    };

    if (b) {
        if (!deferred) setShadowEngine(d_light, ssme);
        if (!deferred) setShadowEngine(p_light, ssme);
        if (!deferred) setShadowEngine(s_light, ssme);
        if (!deferred) setShadowEngine(ph_light, ssme);
        if (deferred) setShadowEngine(d_light, gsme);
        if (deferred) setShadowEngine(p_light, ptsme);
        if (deferred) setShadowEngine(s_light, stsme);
        if (deferred) setShadowEngine(ph_light, ptsme);
        getBoundingbox(); // update osg volume
    } else {
        setShadowEngine(d_light, 0);
        setShadowEngine(p_light, 0);
        setShadowEngine(s_light, 0);
        setShadowEngine(ph_light, 0);
    }

    updateDeferredLight();
#endif
}

void VRLight::setShadowNearFar(Vec2d nf) {
#ifndef OSG_OGL_ES2
    shadowNearFar = nf;
    //if (ssme) ssme->setShadowNear(nf[0]);
    //if (ssme) ssme->setShadowFar(nf[1]);
    if (gsme) gsme->setShadowNear(nf[0]);
    if (gsme) gsme->setShadowFar(nf[1]);
    if (ptsme) ptsme->setShadowNear(nf[0]);
    if (ptsme) ptsme->setShadowFar(nf[1]);
    if (stsme) stsme->setShadowNear(nf[0]);
    if (stsme) stsme->setShadowFar(nf[1]);
#endif
}

void VRLight::setShadowVolume(Boundingbox b) {
    shadowVolume = b;
#ifndef OSG_OGL_ES2
    BoxVolume box(Pnt3f(b.min()), Pnt3f(b.max()));
#ifdef WITH_SHADOW_VOLUME
    cout << "VRLight::setShadowVolume " << b.volume() << endl;
    if (gsme) gsme->setShadowVolume(box);
#endif
#endif
}

Boundingbox VRLight::getShadowVolume() {
    return shadowVolume;
}

void VRLight::setShadowColor(Color4f c) {
    shadowColor = c;
#ifndef OSG_OGL_ES2
    if (ssme) ssme->setShadowColor(c);
    //if (gsme) gsme->setShadowColor(c);
    //if (ptsme) ptsme->setShadowColor(c);
    //if (stsme) stsme->setShadowColor(c);
    updateDeferredLight();
#endif
}

void VRLight::setOn(bool b) {
    on = b;
    dynamic_pointer_cast<Light>(d_light->core)->setOn(b);
    dynamic_pointer_cast<Light>(p_light->core)->setOn(b);
    dynamic_pointer_cast<Light>(s_light->core)->setOn(b);
    dynamic_pointer_cast<Light>(ph_light->core)->setOn(b);
}

bool VRLight::isOn() { return on; }

void VRLight::setAttenuation(Vec3d a) {
    attenuation = a;
    dynamic_pointer_cast<Light>(d_light->core)->setConstantAttenuation(a[0]);
    dynamic_pointer_cast<Light>(d_light->core)->setLinearAttenuation(a[1]);
    dynamic_pointer_cast<Light>(d_light->core)->setQuadraticAttenuation(a[2]);
    dynamic_pointer_cast<PointLight>(p_light->core)->setAttenuation(a[0], a[1], a[2]);
    dynamic_pointer_cast<PointLight>(ph_light->core)->setAttenuation(a[0], a[1], a[2]);
    dynamic_pointer_cast<SpotLight>(s_light->core)->setAttenuation(a[0], a[1], a[2]);
    if (deferred) updateDeferredLight();
}

Vec3d VRLight::getAttenuation() { return attenuation; }

void VRLight::setShadowMapRes(int t) {
#ifndef OSG_OGL_ES2
    shadowMapRes = t;
    if (ssme) ssme->setWidth (t);
    if (ssme) ssme->setHeight(t);
    if (gsme) gsme->setWidth (t);
    if (gsme) gsme->setHeight(t);
    if (ptsme) ptsme->setWidth (t);
    if (ptsme) ptsme->setHeight(t);
    if (stsme) stsme->setWidth (t);
    if (stsme) stsme->setHeight(t);
#endif
}

int VRLight::getShadowMapRes() { return shadowMapRes; }

vector<string> VRLight::getTypes() {
    vector<string> s;
    s.push_back("point");
    s.push_back("directional");
    s.push_back("spot");
    s.push_back("photometric");
    return s;
}

vector<string> VRLight::getShadowMapResolutions() {
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
        params["Photometric light"] = vector<string>();
        params["Directional light"] = vector<string>();
        params["Spot light"] = vector<string>();

        params["Spot light"].push_back("Spot Cut Off");
        params["Spot light"].push_back("Spot Cut Off Deg");
        params["Spot light"].push_back("Spot Exponent");

        params["Photometric light"].push_back("Map");
    }

    if (params.count(type)) return params[type];
    else return vector<string>();
}

// IDEE: licht sucht ob beacon schon da ist, danach sucht beacon ob licht schon da ist.. je nachdem wer wann erstellt wird..

VRLightBeaconPtr VRLight::addBeacon() {
    auto b = VRLightBeacon::create(getName() + "_beacon");
    addChild(b);
    setBeacon(b);
    return b;
}

VRLightBeaconPtr VRLight::getBeacon() { return beacon.lock(); }

void VRLight::setPointlight() { switchCore(p_light); updateDeferredLight(); }
void VRLight::setSpotlight() { switchCore(s_light); updateDeferredLight(); }
void VRLight::setDirectionallight() { switchCore(d_light); updateDeferredLight(); }
void VRLight::setPhotometriclight() { switchCore(ph_light); updateDeferredLight(); }

LightMTRecPtr VRLight::getLightCore() { return dynamic_pointer_cast<Light>(getCore()->core); }
string VRLight::getLightType() { return lightType; };

void VRLight::updateDeferredLight() {
    if (deferred) VRScene::getCurrent()->updateLight( ptr() );
}

void VRLight::reloadDeferredSystem() {
    updateDeferredLight();
}

void VRLight::setPhotometricMap(VRTexturePtr tex) { photometricMap = tex; updateDeferredLight(); }

VRTexturePtr VRLight::getPhotometricMap(bool forVisual) {
    if (!forVisual) return photometricMap;

    int W = photometricMap->getSize()[0];
    int H = photometricMap->getSize()[1];
    vector<Vec3f> texData = vector<Vec3f>(W*H);
    for (int i=0; i<W*H; i++) {
        float c = photometricMap->getPixel(i)[0];
        texData[i] = Vec3f(c,c,c);
    }
    auto tex = VRTexture::create();
    auto img = tex->getImage();
    img->set( Image::OSG_RGB_PF, W, H, 1, 1, 1, 0, (const uint8_t*)&texData[0], Image::OSG_FLOAT32_IMAGEDATA, true, 1);
    return tex;
}

void VRLight::loadPhotometricMap(string path) { // ies files
#ifndef WITHOUT_IES
    if (path == "") return;
    photometricMapPath = path;
    VRIES parser;
    auto tex = parser.read(path);
    setPhotometricMap(tex);
    //cout << parser.toString(false);
#endif
}


