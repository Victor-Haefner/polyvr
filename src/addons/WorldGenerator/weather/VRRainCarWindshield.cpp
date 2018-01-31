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

    setScale(scale);

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

void VRRainCarWindshield::setScale(float scale) {
    this->scale = scale;
    setShaderParameter("scale", scale);
}

void VRRainCarWindshield::setWindshield(VRGeometryPtr geoWindshield) {
    //cout << cubeWindshield->getFrom() << " from " << cubeWindshield->getDir() << endl;
    this->geoWindshield = geoWindshield;
}

void VRRainCarWindshield::update() {
    if (!isVisible()) return;

    tnow = glutGet(GLUT_ELAPSED_TIME)*0.001; //seconds
    tdelta = tnow-tlast;
    tlast = tnow;

    Vec3d windshieldPos = geoWindshield->getWorldPosition();
    Vec3d windshieldDir = geoWindshield->getWorldDirection();
    Vec3d windshieldUp = geoWindshield->getWorldUp();

    mat->setActivePass(0);
    mat->readVertexShader(vScript);
    mat->readFragmentShader(fScript);
    mat->setActivePass(1);
    mat->readVertexShader(vScript);
    mat->readFragmentShader(fScript);

    setShaderParameter("tnow", tnow);
    setShaderParameter("offset", tdelta);
    setShaderParameter("windshieldPos", convertV3dToV3f(windshieldPos));
    setShaderParameter("windshieldDir", convertV3dToV3f(windshieldDir));
    setShaderParameter("windshieldUp", convertV3dToV3f(windshieldUp));
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
    this->wiperSpeed = wiperSpeed;
    setShaderParameter("isWiping", isWiping);
    setShaderParameter("wiperSpeed", wiperSpeed);
    cout << "VRRainCarWindshield::setWipers()" << endl;
}


