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

/*	Python usage example:
    VR.rain = VR.Rain()
	VR.rain.setScale(3)
	VR.scene.addChild(VR.rain)
	VR.rain.start()
*/

VRRain::VRRain() : VRGeometry("Rain") {
    type = "Rain";
    string resDir = VRSceneManager::get()->getOriginalWorkdir() + "/shader/Rain/";
    vScript = resDir + "Rain.vp";
    fScript = resDir + "Rain.fp";
    vScriptTex = resDir + "RainTex.vp";
    fScriptTex = resDir + "RainTex.fp";

    //Shader setup
    mat = VRMaterial::create("Rain");
    mat->readVertexShader(vScript);
    mat->readFragmentShader(fScript);
    setMaterial(mat);
    setPrimitive("Plane", "2 2 1 1");
    mat->setLit(false);
	mat->setDiffuse(Color3f(1));
	mat->setShaderParameter<float>("rainOffset", offset);
	mat->setShaderParameter<float>("rainDensity", rainDensity);
	mat->setShaderParameter<float>("camH", camH);
	mat->enableTransparency();

    setVolumeCheck(false, true);

    //TexRenderer setup
    textureSize = 512;

    auto camDef = VRScene::getCurrent()->getActiveCamera();
    oldCamTex = camDef;

    texRenderer = VRTextureRenderer::create("rainTexRenderer");
    texRenderer-> setPersistency(0);
    auto scene = VRScene::getCurrent();
    scene->getRoot()->addChild(texRenderer);
    auto lightF = scene->getRoot()->find("light");

    camTex = VRCamera::create("camRainTexture");
    camTex-> setPersistency(0);
    lightF->addChild(camTex);
    camTex->setType(1);
    texRenderer->setup(camTex,512,512);
    camTex->setFrom(Vec3d(0,100,0));
    camTex->setAt(Vec3d(0,1,-1));
    texRenderer->addLink(lightF);

    renderMat = texRenderer->getMaterial();
    renderMat->readVertexShader(vScriptTex);
    renderMat->readFragmentShader(fScriptTex);
    renderMat->setShaderParameter<float>("tex0", 0);
    renderMat->setShaderParameter<float>("tex1", 1);

    mat->setTexture(renderMat->getTexture(1));
    mat->setShaderParameter("tex", 1);

    cube = VRGeometry::create("cubeRainTexture");
	cube->setMaterial(renderMat);
	lightF->addChild(cube);

    camDef->activate();

	density = densityStart;
    speedX = speedStartX;
    speedY = speedStartY;
    color = colorStart;
    light = lightStart;

    updatePtr = VRUpdateCb::create("rain update", boost::bind(&VRRain::update, this));
    VRScene::getCurrent()->addUpdateFkt(updatePtr);

    cout << "VRRain::VRRain()\n";
}
VRRain::~VRRain() {}

VRRainPtr VRRain::create() { return VRRainPtr( new VRRain() ); }
VRRainPtr VRRain::ptr() { return static_pointer_cast<VRRain>( shared_from_this() ); }

float VRRain::get() { return scale; }
VRTextureRendererPtr VRRain::getRenderer() { return texRenderer; }
VRMaterialPtr VRRain::getTexMat() { return renderMat; }

void VRRain::start() {
    if (!isRaining) {
        if (scale == 0) setScale(3);
        else setScale(scale);

        auto rainAnimation = VRAnimation::create("startRain");
        rainAnimationCb = VRAnimCb::create("rain update", boost::bind(&VRRain::startRainCallback, this, _1));
        rainAnimation->setCallback(rainAnimationCb);
        rainAnimation->setDuration(durationTransition);
        rainAnimation->start();
        isRaining = true;
        cout << "VRRain::startRain()\n";
    }
    else cout << "it's already raining" << endl;
}

void VRRain::startRainCallback(float t) {
    auto sky = VRScene::getCurrent()->getSky();
    sky->setClouds(densityStart+(density-densityStart)*t, 1e-5, 3000, Vec2d(speedStartX+(speedX-speedStartX)*t, speedStartY+(speedX-speedStartY)*t), Color4f(colorStart-(colorStart-color)*t,colorStart-(colorStart-color)*t,colorStart-(colorStart-color)*t,1));

	//IDEA: MAYBE CHANGE LIGHT SETTINGS OF SCENE
	//lightMain = VRScene::getCurrent()->getRoot()->find("light");
    //float cl = 1-0.4*t;
	//lightMain->setDiffuse(Color4f(cl,cl,cl,0));

	if (tnow != floor(t*10)) updateScale(scale*floor(t*10)/10);
	tnow = floor(t*10);
}

void VRRain::stop() {
    if (isRaining) {
        auto rainAnimation = VRAnimation::create("stopRain");
        rainAnimationCb = VRAnimCb::create("rain update", boost::bind(&VRRain::stopRainCallback, this, _1));
        rainAnimation->setCallback(rainAnimationCb);
        rainAnimation->setDuration(durationTransition);
        rainAnimation->start();
        isRaining = false;
    }
    else cout << "currently not raining" << endl;
}

void VRRain::stopRainCallback(float t) {
    auto sky = VRScene::getCurrent()->getSky();
    sky->setClouds(density-(density-densityStart)*t, 1e-5, 3000, Vec2d(speedX-(speedX-speedStartX)*t, speedX-(speedX-speedStartY)*t), Color4f(color+(colorStart-color)*t,color+(colorStart-color)*t,color+(colorStart-color)*t,1));

    //lightMain = VRScene::getCurrent()->getRoot()->find("light");
    //float cl = 0.6+0.4*t;
	//lightMain->setDiffuse(Color4f(cl,cl,cl,0));
	if (tnow != floor(t*10)) updateScale(scale-scale*floor(t*10)/10);
    tnow = floor(t*10);
}

void VRRain::setScale( float scale ){
    if ( scale<0 || scale>10 ) {
        //TODO: error
        cout << "Input for Scale of Rain should be between 0 and 10" << endl;
    }
    else {
        this->scale = scale;
        density = 0.4+scale*0.06;    //density of clouds
        speedX = 0.004+scale*0.0001;
        speedY = 0.004+scale*0.0001;
        color = 0.5-scale*0.02;
        light = 0.4;
    }
}

void VRRain::overrideParameters( float durationTransition, float rainDensity, float density, float speedX, float speedY, float color, float light ) {
    this->durationTransition = durationTransition;
    this->rainDensity = rainDensity;
    this->density = density;
    this->speedX = speedX;
    this->speedY = speedY;
    this->color = color;
    this->light = light;
    mat->setShaderParameter<float>("rainDensity", rainDensity);
}

void VRRain::updateScale( float scaleNow ){
    scaleRN = scaleNow;
    float rainDensity = 0.2 * 10/scaleRN;
    mat->setShaderParameter<float>("rainDensity", rainDensity);
    cout << " " << rainDensity << endl;
}

void VRRain::update() {
    if (!isVisible()) return;

    double offset = glutGet(GLUT_ELAPSED_TIME)*0.001; //seconds

    auto camDef = VRScene::getCurrent()->getActiveCamera();
    if (debugRain) cout << "defcam: " << camDef->getName() << endl;
    if (camDef == camTex) camDef = oldCamTex;
    else oldCamTex = camDef;
    //if (camDef->getName() == "car") oldCamTex = camDef;
    if (debugRain) cout << "oldcam: " << oldCamTex->getName() << endl;
    auto defCamPos = camDef->getWorldPosition();//camDef->getFrom();
    camTex->setFrom(defCamPos);
    camTex->translate(Vec3d(0,40,0));
    camTex->setAt(defCamPos);
    camTex->setUp(Vec3d(0,0,1));
    if (debugRain) cout << "camdeffrom: " << camDef->getFrom() << endl;
    if (debugRain) cout << "camtexfrom: " << camTex->getFrom() << endl;

    mat->setShaderParameter<float>("rainOffset", offset);
    mat->readVertexShader(vScript);
    mat->readFragmentShader(fScript);
}

void VRRain::doTestFunction() {
    auto tmp = camTex->getFrom();
    cout << tmp << endl;
}

