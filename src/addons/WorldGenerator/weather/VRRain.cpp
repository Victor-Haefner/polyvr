#include "VRRain.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRMaterialT.h"
//#include "core/objects/material/VRTextureGenerator.h"
#include "core/objects/geometry/VRSky.h"
#include "core/tools/VRTextureRenderer.h"
#include "core/utils/VRFunction.h"
#include "core/scene/VRScene.h"

#include <math.h>
#include <random>
#include <boost/bind.hpp>
#include <time.h>
#include <GL/glut.h>

using namespace OSG;

VRRain::VRRain() : VRGeometry("Rain") {
    type = "Rain";
    string resDir = VRSceneManager::get()->getOriginalWorkdir() + "/shader/Rain/";
    vScript = resDir + "Rain.vp";
    fScript = resDir + "Rain.fp";

    auto sky = VRScene::getCurrent()->getSky();
    // shader setup
    mat = VRMaterial::create("Rain");
    mat->readVertexShader(vScript);
    mat->readFragmentShader(fScript);
    setMaterial(mat);
    setPrimitive("Plane", "2 2 1 1");
    mat->setLit(false);
	mat->setDiffuse(Color3f(1));
	offset = 0;
	density = 0;
	mat->setShaderParameter<float>("rainOffset", offset);
	mat->setShaderParameter<float>("rainDensity", density);
	mat->setShaderParameter<float>("camH", camH);
	mat->enableTransparency();

    setVolumeCheck(false, true);

    //TexRenderer setup
    textureSize = 512;
    auto camDef = VRScene::getCurrent()->getActiveCamera();
    auto camTex = VRCamera::create("camTex");
    auto tr = VRTextureRenderer::create("tr");
    camTex->setType(1);
    camDef->activate();
    tr->setup(camTex,512,512);

    density = densityStart;
    speedX = speedStartX;
    speedY = speedStartY;
    color = colorStart;
    light = lightStart;
    //TODO: FIND LIGHT IN SCENEGRAPH
    //TODO: ADDLINK LIGHT
    /*
    VRTextureGenerator tg;
	tg.setSize(textureSize, textureSize);
    */
    Vec3d tmp = camDef->getFrom();
    camTex->setFrom(Vec3d(tmp[0],tmp[1]+40,tmp[2]));

    mat->setTexture(tr->getMaterial()->getTexture(1));
    mat->setShaderParameter<float>("tex", 1);

    updatePtr = VRUpdateCb::create("rain update", boost::bind(&VRRain::update, this));
    VRScene::getCurrent()->addUpdateFkt(updatePtr);

    cout << "VRRain::VRRain()\n";
}
VRRain::~VRRain() {}

VRRainPtr VRRain::create() { return VRRainPtr( new VRRain() ); }
VRRainPtr VRRain::ptr() { return static_pointer_cast<VRRain>( shared_from_this() ); }

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
}

void VRRain::startRainCallback(float t) {
    auto sky = VRScene::getCurrent()->getSky();

    sky->setClouds(densityStart+(density-densityStart)*t, 1e-5, 3000, Vec2d(speedStartX+(speedX-speedStartX)*t, speedStartY+(speedX-speedStartY)*t), Color4f(colorStart-(colorStart-color)*t,colorStart-(colorStart-color)*t,colorStart-(colorStart-color)*t,1));
	//cl=1-0.4*t
	//VR.find('light').setDiffuse([cl,cl,cl,0])
	if (tnow != floor(t*10)) {
        cout << "Raincallback Start:" << tnow << " - " << floor(t*10) << " - " << scale*floor(t*10)/10 << endl;
        updateScale(scale*floor(t*10)/10);
	}
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
    //scaleRN = 0;
    //float rainDensity = 0.2 * 10/scaleRN;
    //mat->setShaderParameter<float>("rainDensity", rainDensity);
}

void VRRain::stopRainCallback(float t) {
    auto sky = VRScene::getCurrent()->getSky();

    sky->setClouds(density-(density-densityStart)*t, 1e-5, 3000, Vec2d(speedX-(speedX-speedStartX)*t, speedX-(speedX-speedStartY)*t), Color4f(color+(colorStart-color)*t,color+(colorStart-color)*t,color+(colorStart-color)*t,1));
	//cl=0.6+0.4*t
	//VR.find('light').setDiffuse([cl,cl,cl,0])
	if (tnow != floor(t*10)) updateScale(scale-scale*floor(t*10)/10);
    tnow = floor(t*10);
}

void VRRain::setScale( float scale ){
    if ( scale<0 || scale>10 ) {
        //TODO: error
        cout << "Input for Scale of Rain between 0 and 10\n";
    }
    else {
        this->scale = scale;
        density = 1;
        speedX = 0.004;
        speedY = 0.004;
        color = 0.4;
        light = 0.4;
        //updateScale(scale); //TODO: DELETE LINE AFTER FULL IMPLEMENTATION, RIGHT NOW DOES LIVE UPDATE OF RAINSCALE
    }
}

void VRRain::overrideParameters( float rainDensity, float durationTransition, float color, float light ) {
    this->rainDensity = rainDensity;
    mat->setShaderParameter<float>("rainDensity", rainDensity);
}

Vec2d VRRain::get() {
    return Vec2d(density,scale);
}

void VRRain::updateScale( float scaleNow ){
    scaleRN = scaleNow;
    float rainDensity = 0.2 * 10/scaleRN;
    mat->setShaderParameter<float>("rainDensity", rainDensity);
    cout << rainDensity << endl;
    //reloadshader();
}

void VRRain::update() {
    if (!isVisible()) return;

    double offset = glutGet(GLUT_ELAPSED_TIME)*0.001; //seconds

    mat->setShaderParameter<float>("rainOffset", offset);
    mat->readVertexShader(vScript);
    mat->readFragmentShader(fScript);

    //cout << "updater" << endl;

    //Vec3d tmp = camDef->getFrom();
    //camTex->setFrom(Vec3d(tmp[0],tmp[1]+40,tmp[2]));
    //camTex->setAt(tmp);
    //camTex->setUp(Vec3d(0,0,1));
}

void VRRain::reloadShader() {
    Vec3d tmpasdf = camDef->getFrom();
    //camTex->setFrom(Vec3d(tmp[0],tmp[1]+40,tmp[2]));
    //camTex->setAt(tmp);
    //camTex->setUp(Vec3d(0,0,1));
    //sky->setClouds(1, 1e-5, 3000, Vec2d(0.004,0.004),Color4f(0.3,0.3,0.3,1));
    //sky.setClouds(float density, float scale, float height, Vec2d speed, Color4f color);

    /*
    string resDir = VRSceneManager::get()->getOriginalWorkdir() + "/shader/Rain/";
    mat->readVertexShader(resDir + "Rain.vp");
    mat->readFragmentShader(resDir + "Rain.fp");
    */
}

