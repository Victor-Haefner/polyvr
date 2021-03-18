#include "VRRainCarWindshield.h"
#include "VRRain.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRMaterialT.h"
#include "core/objects/material/VRTextureGenerator.h"
#include "core/objects/geometry/VRSky.h"
#include "core/tools/VRTextureRenderer.h"
#include "core/utils/VRFunction.h"
#include "core/utils/system/VRSystem.h"
#include "core/scene/VRScene.h"

#include "core/objects/VRCamera.h"
#include "core/objects/VRLight.h"
#include "core/objects/VRLightBeacon.h"

#include <math.h>
#include <random>

using namespace OSG;

/* Python usage example:
	def initDummyWindshield():
		VR.dummyWindshield = VR.Geometry('windshield')
		VR.dummyWindshield.setPrimitive('Box 1.7 0.01 0.5 1 1 1')
		VR.dummyWindshield.setTransform([0,50,0],[0,1,1],[0,1,0])
		VR.dummyWindshield.setPickable(1)
		VR.scene.addChild(VR.dummyWindshield)
		VR.dummyWindshield.setVisible(False)

	def initRainyWindshield():
		VR.rcShield = VR.RainCarWindshield()
		windshield = VR.find('windshield') #Geometry
		VR.rcShield.setWindshield(windshield)
		VR.scene.addChild(VR.rcShield)
		VR.rcShield.start()
		VR.rcShield.setScale(True,3)

    initDummyWindshield()
	initRainyWindshield()
*/

VRRainCarWindshield::VRRainCarWindshield() : VRGeometry("RainCarWindshield") {
    type = "RainCarWindshield";

    //Shader setup
    mat = VRMaterial::create("RainCarWindshield");
    reloadShader();
    setMaterial(mat);
    setPrimitive("Plane 2 2 1 1");
    mat->setLit(false);
	mat->setDiffuse(Color3f(1));
	mat->enableTransparency();
    mat->setShaderParameter("pass", 0);

	mat->addPass();
    mat->readVertexShader(vScript);
    mat->readFragmentShader(fScript);
    setMaterial(mat);
    setPrimitive("Plane 2 2 1 1");
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
    setShaderParameter("hasPower", hasPower);

    updatePtr = VRUpdateCb::create("VRRainCarWindshield update", bind(&VRRainCarWindshield::update, this));
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

VRRainCarWindshieldPtr VRRainCarWindshield::create() {
    auto w = VRRainCarWindshieldPtr( new VRRainCarWindshield() );
    w->hide("SHADOW");
    return w;
}

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

    tnow = getTime()*1e-6; //seconds
    tdelta = tnow-tlast;
    tlast = tnow;

    auto applyWiperSpeed = [&]() {
        wiperSpeed = newWiperSpeed;
        setShaderParameter("wiperSpeed", wiperSpeed);
    };

    if (isWiping && newWiperSpeed == wiperSpeed){
        //resetting time stamp for neutral wiper state
        if (tnow-durationWiper/wiperSpeed>tWiperstart) tWiperstart=tnow;
    } else {
        //start of wipers
        if (isWiping && wiperSpeed == 0) {
            float newPeriod = durationWiper/newWiperSpeed;
            tWiperstart=tnow - wiperState*newPeriod;
            applyWiperSpeed();
        }
        //seamless live changes of wiperSpeeds
        if (isWiping && wiperSpeed > 0) {
            float thisPeriod = durationWiper/wiperSpeed;
            float newPeriod = durationWiper/newWiperSpeed;
            wiperState = (tnow-tWiperstart)/thisPeriod;
            tWiperstart=tnow - wiperState*newPeriod;
            applyWiperSpeed();
        }
        //soft-stopping wipers (stop in neutral state)
        if (!isWiping && wiperSpeed > 0 && tnow-durationWiper/wiperSpeed>tWiperstart) {
            newWiperSpeed = 0;
            wiperState = 0;
            applyWiperSpeed();
        }
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
    mat->readFragmentShader(dfScript, true);
    mat->updateDeferredShader();
    mat->setActivePass(1);
    mat->readVertexShader(vScript);
    mat->readFragmentShader(fScript);
    mat->readFragmentShader(dfScript, true);
    mat->updateDeferredShader();

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
    setShaderParameter("wiperPos", wiperPos);
    setShaderParameter("wiperSize", wiperSize);
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

void VRRainCarWindshield::setWipers(bool isWiping, float newWiperSpeed) {
    this->isWiping = isWiping;
    this->newWiperSpeed = newWiperSpeed;
    hasPower = true;
    setShaderParameter("hasPower", hasPower);
    cout << "VRRainCarWindshield::setWipers(" << isWiping <<","<< newWiperSpeed << ")" << endl;
}

void VRRainCarWindshield::setWiperPos(Vec2d in) {
    wiperPos = convertV2dToV2f(in);
}

void VRRainCarWindshield::setWiperSize(float in) {
    wiperSize = in;
}

void VRRainCarWindshield::cutPower() {
    isWiping = false;
    hasPower = false;
    if (wiperSpeed != 0) {
        float thisPeriod = durationWiper/wiperSpeed;
        wiperState = (tnow-tWiperstart)/thisPeriod;
    }
    newWiperSpeed = 0;
    wiperSpeed = newWiperSpeed;
    setShaderParameter("wiperSpeed", wiperSpeed);
    setShaderParameter("hasPower", hasPower);
    setShaderParameter("wiperState", wiperState);
    cout << "VRRainCarWindshield::cutPower()" << endl;
}

void VRRainCarWindshield::reloadShader() {
    cout << "VRRainCarWindshield::reloadShader()" << endl;
    string resDir = VRSceneManager::get()->getOriginalWorkdir() + "/shader/Rain/";
    vScript = resDir + "RainCarWindshield.vp";
    fScript = resDir + "RainCarWindshield.fp";
    dfScript = resDir + "RainCarWindshield.dfp";

    mat->readVertexShader(vScript);
    mat->readFragmentShader(fScript);
    mat->readFragmentShader(dfScript, true);

    mat->updateDeferredShader();
}


