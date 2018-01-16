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

VRRainCarWindshieldPtr VRRainCarWindshield::create() { return VRRainCarWindshieldPtr( new VRRainCarWindshield() ); }
VRRainCarWindshieldPtr VRRainCarWindshield::ptr() { return static_pointer_cast<VRRainCarWindshield>( shared_from_this() ); }

float VRRainCarWindshield::get() { return scale; }

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
}

void VRRainCarWindshield::doTestFunction() {
    string tmp = "asdf";
    cout << tmp << endl;
}


