#include "VRRainCarWindshield.h"
#include "VRRain.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRMaterialT.h"
#include "core/objects/material/VRTextureGenerator.h"
#include "core/objects/geometry/VRSky.h"
#include "core/tools/VRTextureRenderer.h"
#include "core/utils/VRFunction.h"
#include "core/scene/VRScene.h"

#include "core/objects/VRCamera.h"
#include "core/objects/VRLight.h"
#include "core/objects/VRLightBeacon.h"

#include <math.h>
#include <random>
#include <boost/bind.hpp>
#include <time.h>
#include <GL/glut.h>

using namespace OSG;

/* Python usage example:
def initRainyWindshield():
	VR.rcShield = VR.RainCarWindshield()
	windshield = VR.find('windshield') #Geometry
	VR.rcShield.setWindshield(windshield)
	VR.scene.addChild(VR.rcShield)
*/

VRRainCarWindshield::VRRainCarWindshield() : VRGeometry("RainCarWindshield") {
    type = "RainCarWindshield";
    string resDir = VRSceneManager::get()->getOriginalWorkdir() + "/shader/Rain/";
    vScript = resDir + "RainCarWindshield.vp";
    fScript = resDir + "RainCarWindshield.fp";

    //Shader setup
    mat = VRMaterial::create("RainCarWindshield");
    mat->readVertexShader(vScript);
    mat->readFragmentShader(fScript);
    setMaterial(mat);
    setPrimitive("Plane", "2 2 1 1");
    mat->setLit(false);
	mat->setDiffuse(Color3f(1));
	mat->enableTransparency();
    mat->setShaderParameter("pass", 0);

	mat->addPass();
    mat->readVertexShader(vScript);
    mat->readFragmentShader(fScript);
    setMaterial(mat);
    setPrimitive("Plane", "2 2 1 1");
    mat->setLit(false);
	mat->setDiffuse(Color3f(1));
	mat->enableTransparency();
    mat->setShaderParameter("pass", 1);

    setVolumeCheck(false, true);

    auto scene = VRScene::getCurrent();
    scene->getRoot()->addChild(texRenderer);
    auto lightF = scene->getRoot()->find("light");

    setScale(false, scale);
    setShaderParameter("durationWiper", durationWiper);

    updatePtr = VRUpdateCb::create("VRRainCarWindshield update", boost::bind(&VRRainCarWindshield::update, this));
    VRScene::getCurrent()->addUpdateFkt(updatePtr);

    cout << "VRRainCarWindshield::VRRainCarWindshield()\n";
}
VRRainCarWindshield::~VRRainCarWindshield() {}

Vec3f VRRainCarWindshield::convertV3dToV3f(Vec3d in) {
    Vec3f out;
    out[0] = (float)in[0];
    out[1] = (float)in[1];
    out[2] = (float)in[2];
    return out;
}
Vec2f VRRainCarWindshield::convertV2dToV2f(Vec2d in) {
    Vec2f out;
    out[0] = (float)in[0];
    out[1] = (float)in[1];
    return out;
}
float signum(float in) {
    if (in<0) return -1;
    if (in>0) return 1;
    return 0;
}


VRRainCarWindshieldPtr VRRainCarWindshield::create() { return VRRainCarWindshieldPtr( new VRRainCarWindshield() ); }
VRRainCarWindshieldPtr VRRainCarWindshield::ptr() { return static_pointer_cast<VRRainCarWindshield>( shared_from_this() ); }

float VRRainCarWindshield::get() { return scale; }

template<typename T>
void VRRainCarWindshield::setShaderParameter(string name, T t) {
    mat->setActivePass(0);
    mat->setShaderParameter<T>(name, t);
    mat->setActivePass(1);
    mat->setShaderParameter<T>(name, t);
}

void VRRainCarWindshield::setScale(bool liveChange, float scale) {
    this->scale = scale;
    if (liveChange){
        setShaderParameter("scale", scale);
    }
}

void VRRainCarWindshield::setWindshield(VRGeometryPtr geoWindshield) {
    //cout << cubeWindshield->getFrom() << " from " << cubeWindshield->getDir() << endl;
    this->geoWindshield = geoWindshield;
    lastWorldPosition = geoWindshield->getWorldPosition();
}

Vec2f VRRainCarWindshield::calcAccComp(Vec3d accelerationVec,Vec3d windshieldDir,Vec3d windshieldUp){
    Vec2f out;
    out = Vec2f(0,-1);
    out[0] = (windshieldDir.cross(windshieldUp)).dot(accelerationVec);
    out[1] = windshieldDir.dot(accelerationVec);
    float l = out.length();
    out[1] = 0;
    return out/l;
}

void VRRainCarWindshield::update() {
    if (!isVisible()) return;

    tnow = glutGet(GLUT_ELAPSED_TIME)*0.001; //seconds
    tdelta = tnow-tlast;
    tlast = tnow;

    if (isWiping && tnow-durationWiper/wiperSpeed>tWiperstart) tWiperstart=tnow;
    if (!isWiping && wiperSpeed>0 && tnow-durationWiper/wiperSpeed>tWiperstart) {
            wiperSpeed=0;
            setShaderParameter("wiperSpeed", wiperSpeed);
    }

    Vec3d windshieldPos = geoWindshield->getWorldPosition();
    Vec3d windshieldDir = geoWindshield->getWorldDirection();
    Vec3d windshieldUp = geoWindshield->getWorldUp();
    Vec3d velocityVec = (lastWorldPosition - windshieldPos)/tdelta; // [m/s]
    Vec3d accelerationVec = (lastVelocityVec - velocityVec)/tdelta;
    lastWorldPosition = windshieldPos;
    lastVelocityVec = velocityVec;
    float multiplier = 0.001;
    float multiplierAcc = 0.0001;
    Vec2f accelerationComponent = calcAccComp(accelerationVec,windshieldDir,windshieldUp);
    Vec2f mapOffset0=oldMapOffset0 +  Vec2f(0,1)*multiplierAcc + Vec2f(0,-1)*multiplier*velocityVec.length()*tdelta;
    Vec2f mapOffset1=oldMapOffset1 + (Vec2f(0,1)*multiplierAcc + Vec2f(0,-1)*multiplier*velocityVec.length()*tdelta)*1.2;
    //multiplierAcc*(accelerationComponent+oldAccelerationComponent)*4 +
    oldMapOffset0=mapOffset0;
    oldMapOffset1=mapOffset1;
    oldAccelerationComponent = accelerationComponent;
    if (oldMapOffset0[0]>400||oldMapOffset0[1]>400) {
        oldMapOffset0=Vec2f(0,0);
    }
    if (oldMapOffset1[0]>400||oldMapOffset1[1]>400) {
        oldMapOffset1=Vec2f(0,0);
    }
    mat->setActivePass(0);
    mat->readVertexShader(vScript);
    mat->readFragmentShader(fScript);
    mat->setActivePass(1);
    mat->readVertexShader(vScript);
    mat->readFragmentShader(fScript);

    setShaderParameter("tnow", tnow);
    setShaderParameter("tWiperstart", tWiperstart);
    setShaderParameter("isWiping", isWiping);
    setShaderParameter("offset", tdelta);
    setShaderParameter("windshieldPos", convertV3dToV3f(windshieldPos));
    setShaderParameter("windshieldDir", convertV3dToV3f(windshieldDir));
    setShaderParameter("windshieldUp", convertV3dToV3f(windshieldUp));
    setShaderParameter("velocityVec", convertV3dToV3f(velocityVec));
    setShaderParameter("accelerationVec", convertV3dToV3f(accelerationVec));
    setShaderParameter("mapOffset0", mapOffset0);
    setShaderParameter("mapOffset1", mapOffset1);
}

void VRRainCarWindshield::doTestFunction() {
    string tmp = "VRRainCarWindshield::doTestFunction() ";
    Vec3d wPos = geoWindshield->getWorldPosition();
    Vec3d wDir = geoWindshield->getWorldDirection();
    isRaining=!isRaining;
    setShaderParameter("isRaining", isRaining);
    cout << tmp << wPos << " " << wDir << endl;
}

void VRRainCarWindshield::start() {
    isRaining = true;
    setShaderParameter("isRaining", isRaining);
    cout << "VRRainCarWindshield::start()" << endl;
}

void VRRainCarWindshield::stop() {
    isRaining= false;
    setShaderParameter("isRaining", isRaining);
    cout << "VRRainCarWindshield::stop()" << endl;
}

void VRRainCarWindshield::setWipers(bool isWiping, float wiperSpeed) {
    this->isWiping = isWiping;
    if (isWiping) this->wiperSpeed = wiperSpeed;
    if (isWiping) setShaderParameter("wiperSpeed", wiperSpeed);
    cout << "VRRainCarWindshield::setWipers(" << isWiping <<","<< wiperSpeed << ")" << endl;
}


