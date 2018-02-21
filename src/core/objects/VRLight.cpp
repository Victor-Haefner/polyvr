#include "VRLight.h"
#include "core/utils/toString.h"
#include "core/utils/VRStorage_template.h"
#include "core/objects/material/VRTexture.h"
#include "core/scene/VRScene.h"
#include "core/objects/OSGObject.h"
#include "core/objects/object/OSGCore.h"
#include "VRLightBeacon.h"
#include "VRShadowEngine.h"

#include <OpenSG/OSGNode.h>
#include <OpenSG/OSGShadowStage.h>
#include <OpenSG/OSGImage.h>
#include <OpenSG/OSGTextureObjChunk.h>
#include <OpenSG/OSGTextureEnvChunk.h>
#include <OpenSG/OSGSimpleMaterial.h>
#include <OpenSG/OSGPointLight.h>
#include <OpenSG/OSGDirectionalLight.h>
#include <OpenSG/OSGSpotLight.h>
#include <OpenSG/OSGSimpleShadowMapEngine.h>
#include <OpenSG/OSGShaderShadowMapEngine.h>
#include <OpenSG/OSGTrapezoidalShadowMapEngine.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

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
    storeObjName("beacon", &beacon, &beacon_name);
    regStorageSetupFkt( VRUpdateCb::create("light setup", boost::bind(&VRLight::setup, this)) );
    regStorageSetupAfterFkt( VRUpdateCb::create("light setup after", boost::bind(&VRLight::setup_after, this)) );

    // test scene
    //shadow_test_scene* sts = new shadow_test_scene();
    //addChild(sts->rootNode);
}

VRLight::~VRLight() {
    VRScene::getCurrent()->subLight( getID() );
}

VRLightPtr VRLight::ptr() { return static_pointer_cast<VRLight>( shared_from_this() ); }
VRLightPtr VRLight::create(string name) {
    auto l = shared_ptr<VRLight>(new VRLight(name) );
    VRScene::getCurrent()->addLight(l);
    return l;
}

void VRLight::setup() {
    setType(lightType);
    setShadows(shadows);
    setShadowColor(shadowColor);
    setDiffuse(lightDiffuse);
    setAmbient(lightAmbient);
    setSpecular(lightSpecular);
    setOn(on);
    loadPhotometricMap(photometricMapPath);

    setup_after(); // TODO: deffered shading needs to have the light beacon before adding the node!?!
}

void VRLight::setup_after() {
    auto root = VRScene::getCurrent()->getRoot();
    VRObjectPtr tmp = getRoot()->find(beacon_name);
    if (tmp) setBeacon( static_pointer_cast<VRLightBeacon>(tmp) );
    else cout << "  !! could not find light beacon: " << root << " " << this << endl;
}

void VRLight::setType(string type) {
    lightType = type;
    if (type == "point") setPointlight();
    if (type == "directional") setDirectionallight();
    if (type == "spot") setSpotlight();
    if (type == "photometric") setPhotometriclight();
}

void VRLight::setShadowParams(bool b, int res, Color4f c) {
    setShadows(b);
    setShadowMapRes(res);
    setShadowColor(c);
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
}

Color4f VRLight::getDiffuse() { return lightDiffuse; }

void VRLight::setAmbient(Color4f c) {
    lightAmbient = c;
    dynamic_pointer_cast<Light>(d_light->core)->setAmbient(c);
    dynamic_pointer_cast<Light>(p_light->core)->setAmbient(c);
    dynamic_pointer_cast<Light>(s_light->core)->setAmbient(c);
    dynamic_pointer_cast<Light>(ph_light->core)->setAmbient(c);
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
    //ssme = static_pointer_cast<VRShadowEngine>( VRShadowEngine::create() );
    ssme = VRShadowEngine::create();
    //ssme = SimpleShadowMapEngine::create();
    gsme = ShaderShadowMapEngine::create();
    ptsme = TrapezoidalShadowMapEngine::create();
    stsme = TrapezoidalShadowMapEngine::create();
    setShadowColor(shadowColor);

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
}

bool VRLight::getShadows() { return shadows; }
Color4f VRLight::getShadowColor() { return shadowColor; }

void VRLight::setShadows(bool b) {
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
}

void VRLight::setShadowColor(Color4f c) {
    shadowColor = c;
    if (!ssme) return;
    ssme->setShadowColor(c);
    //gsme->setShadowColor(c);
    //tsme->setShadowColor(c);
    // TODO: shadow color has to be passed as uniform to light fragment shader
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
}

Vec3d VRLight::getAttenuation() { return attenuation; }

void VRLight::setShadowMapRes(int t) {
    shadowMapRes = t;
    if (ssme) ssme->setWidth (t);
    if (ssme) ssme->setHeight(t);
    if (gsme) gsme->setWidth (t);
    if (gsme) gsme->setHeight(t);
    if (ptsme) ptsme->setWidth (t);
    if (ptsme) ptsme->setHeight(t);
    if (stsme) stsme->setWidth (t);
    if (stsme) stsme->setHeight(t);
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

VRLightBeaconWeakPtr VRLight::getBeacon() { return beacon; }

void VRLight::setPointlight() { switchCore(p_light); updateDeferredLight(); }
void VRLight::setSpotlight() { switchCore(s_light); updateDeferredLight(); }
void VRLight::setDirectionallight() { switchCore(d_light); updateDeferredLight(); }
void VRLight::setPhotometriclight() { switchCore(ph_light); updateDeferredLight(); }

LightMTRecPtr VRLight::getLightCore() { return dynamic_pointer_cast<Light>(getCore()->core); }
string VRLight::getLightType() { return lightType; };

void VRLight::updateDeferredLight() {
    VRScene::getCurrent()->updateLight( ptr() );
}

void VRLight::setPhotometricMap(VRTexturePtr tex) { photometricMap = tex; }
VRTexturePtr VRLight::getPhotometricMap() { return photometricMap; }

void VRLight::loadPhotometricMap(string path) { // ies files
    auto sample2D = [&](float i, float j, vector<float>& data, int aNv, int aNh) {
        int aNv_ = aNv - 1;
        int aNh_ = aNh - 1;

        // 4 corners
        int i1 = floor(i*aNv_);
        int i2 = ceil(i*aNv_);
        int j1 = floor(j*aNh_);
        int j2 = ceil(j*aNh_);

        // sample pnt
        float s,t;
        if (i1==i2) s = 0.0;
        else s = i*aNv_ - i1;
        if (j1==j2) t = 0.0;
        else t = j*aNh_ - j1;

        //quad
        float A = data[i1 + j1*aNv];
        float B = data[i2 + j1*aNv];
        float C = data[i1 + j2*aNv];
        float D = data[i2 + j2*aNv];

        float E = A + s*(B-A);
        float F = C + s*(D-C);

        float P = E + t*(F-E);
        return P;
    };

    auto rescale = [&](vector<float>& data, int Nv, int Nh, int N2v, int N2h, float scale) {
        vector<float> result(N2v*N2v);
        int k = 0;
        for (int i = 0; i < N2v; i++) {
            for (int j = 0; j < N2h; j++, k++) {
                float gi = acos(1-2*i/(N2v-1))/Pi;
                float gj = acos(1-2*j/(N2h-1))/Pi;

                float f = sample2D(gj, gi, data, Nv, Nh) * scale;
                result[k] = f;
            }
        }
        return result;
    };

    auto parseFile = [&]() {
        ifstream file(path);
        string data((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
        auto lines = splitString(data, '\n');
        if (lines.size() < 10) return vector<float>();

        // read parameters
        auto params = splitString( lines[9] );
        int aNv = toInt(params[3]); // number of vertical angles
        int aNh = toInt(params[4]); // number of horizontal angles
        int N = aNv*aNh;
        float lmScale = toFloat(params[2]); // candela multiplier

        cout << "photometricMapPath " << lines[9] << endl;

        // read data
        string dataChunk;
        for (int i=11; i<lines.size(); i++) dataChunk += lines[i] + " ";
        auto ss = stringstream(dataChunk);

        vector<float> aTheta(aNv, 0);
        vector<float> aPhi(aNh, 0);
        vector<float> candela(N, 0);
        for (int i=0; i<aNv; i++) ss >> aTheta[i];
        for (int i=0; i<aNh; i++) ss >> aPhi[i];
        for (int i=0; i<N; i++) ss >> candela[i];

        return rescale(candela, aNv, aNh, 64, 64, lmScale);
    };

    photometricMapPath = path;
    auto candela = parseFile();
    auto tex = VRTexture::create();
    tex->setInternalFormat(GL_ALPHA32F_ARB); // important for unclamped float
    auto img = tex->getImage();
    img->set( Image::OSG_A_PF, 64, 64, 1, 1, 1, 0, (const uint8_t*)&candela[0], Image::OSG_FLOAT32_IMAGEDATA, true, 1);
    setPhotometricMap(tex);
}


OSG_END_NAMESPACE;
