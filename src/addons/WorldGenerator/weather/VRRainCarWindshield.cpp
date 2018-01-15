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
    vScriptTex = resDir + "RainTex.vp";
    fScriptTex = resDir + "RainTex.fp";

    //Shader setup
    mat = VRMaterial::create("RainCarWindshield");
    mat->readVertexShader(vScript);
    mat->readFragmentShader(fScript);
    setMaterial(mat);
    setPrimitive("Plane", "2 2 1 1");
    mat->setLit(false);
	mat->setDiffuse(Color3f(1));
	mat->enableTransparency();

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

    mat->readVertexShader(vScript);
    mat->readFragmentShader(fScript);
}

void VRRainCarWindshield::doTestFunction() {
    string tmp = "asdf";
    cout << tmp << endl;
}


