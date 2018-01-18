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
    //parameter temp setup
    carOrigin = Vec3f(0,0,0);
    carDir = Vec3f(1,0,0);
    posOffset = Vec3f(-10,0,0);

    setVolumeCheck(false, true);

    auto scene = VRScene::getCurrent();
    scene->getRoot()->addChild(texRenderer);
    auto lightF = scene->getRoot()->find("light");

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

/*void VRRainCarWindshield::setDir(Vec3d windshieldDir) {
    //cout << cubeWindshield->getFrom() << " from " << cubeWindshield->getDir() << endl;
    this windshieldDir = windshieldDir;
}*/

void VRRainCarWindshield::setWindshield(VRGeometryPtr geoWindshield) {
    //cout << cubeWindshield->getFrom() << " from " << cubeWindshield->getDir() << endl;
    this->geoWindshield = geoWindshield;
}

void VRRainCarWindshield::update() {
    if (!isVisible()) return;

    /** need to get car position and direction from car */
    mat->setShaderParameter<Vec3f>("carOrigin", carOrigin);
    mat->setShaderParameter<Vec3f>("carDir", carDir);
    mat->setShaderParameter<Vec3f>("posOffset", posOffset);

    tnow = glutGet(GLUT_ELAPSED_TIME)*0.001; //seconds
    tdelta = tnow-tlast;
    tlast = tnow;
    mat->setShaderParameter<float>("tnow", tnow);
    mat->setShaderParameter<float>("offset", tdelta);
    mat->readVertexShader(vScript);
    mat->readFragmentShader(fScript);

    Vec3d windshieldPos = geoWindshield->getWorldPosition();
    Vec3d windshieldDir = geoWindshield->getDir();
    Vec3d windshieldUp = geoWindshield->getWorldUp();
    mat->setShaderParameter<Vec3f>("windshieldPos", convertV3dToV3f(windshieldPos));
    mat->setShaderParameter<Vec3f>("windshieldDir", convertV3dToV3f(windshieldDir));
    mat->setShaderParameter<Vec3f>("windshieldUp", convertV3dToV3f(windshieldUp));
}

void VRRainCarWindshield::doTestFunction() {
    string tmp = "asdf ";
    Vec3d wPos = geoWindshield->getWorldPosition();
    Vec3d wDir = geoWindshield->getDir();
    cout << tmp << wPos << " " << wDir << endl;
}


