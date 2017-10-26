#include "VRRain.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRMaterialT.h"
//#include "core/objects/material/VRTextureGenerator.h"
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

    //auto sky = VRSky::create();
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

    //TODO: FIND LIGHT IN SCENEGRAPH
    //TODO: ADDLINK LIGHT
    /*
    VRTextureGenerator tg;
	tg.setSize(textureSize, textureSize);
    */
    Vec3d tmp = camDef->getFrom();
    camDef->setFrom(Vec3d(tmp[0],tmp[1]+40,tmp[2]));

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
    //setRainScale();
    //setupRain();
    /*
    density = 0.2 * 10/scale;
    mat->setShaderParameter<float>("rainDensity", density)
    //TODO: callback [clouds, light-diffuse, rain]
    double t = 1;
    double densityRainRN = densityRainStart+(densityRain-densityRainStart)*t;
    double cloudSpeedXRN = speedRainStartX+(speedRainX-speedRainStartX)*t;
    double cloudSpeedYRN = speedRainStartY+(speedRainY-speedRainStartY)*t;
    Vec2d cloudSpeedRN = Vec2d(cloudSpeedXRN,cloudSpeedYRN);
    double colorRainRN = colorRainStart-(colorRainStart-colorRain)*t;
    Vec4d cloudColorRN = Vec4d(colorRain,colorRain,colorRain,1);

    //rainSky.setClouds(densityRainRN,1e-5,3000,cloudSpeedRN,cloudColorRN);

    double lightRainRN = lightRainStart - (lightRainStart-lightRain)*t;
    Color4f rainColorRN = Color4f(lightRainRN,lightRainRN,lightRainRN,1);
    //VRLight.setDiffuse(rainColorRN);
    //
    */
    //setScale(0.3);
    if (scale == 0) setScale(3);
        else setScale(scale);
    cout << "VRRain::startRain()\n";
}

void VRRain::stop() {
    //TODO: list of raindropemitters, stop/delete them here

    //TODO: callback [clouds, light-diffuse, rain]
    /*
    double t = 1;
    double densityRainRN = densityRain-(densityRain-densityRainStart)*t;
    double cloudSpeedXRN = speedRainX-(speedRainX-speedRainStartX)*t;
    double cloudSpeedYRN = speedRainY-(speedRainY-speedRainStartY)*t;
    Vec2d cloudSpeedRN = Vec2d(cloudSpeedXRN,cloudSpeedYRN);
    double colorRainRN = colorRain+(colorRainStart-colorRain)*t;
    Color4f cloudColorRN = Color4f(colorRain,colorRain,colorRain,1);

    //VRSky.setClouds(densityRainRN,1e-5,3000,cloudSpeedRN,cloudColorRN);

    double lightRainRN = lightRain + (lightRainStart-lightRain)*t;
    Color4f rainColorRN = Color4f(lightRainRN,lightRainRN,lightRainRN,1);
    //VRLight.setDiffuse(rainColorRN);
    *///clearRain();
    //setScale(0);
    scaleRN = 0;
    density = 0.2 * 10/scaleRN;
    mat->setShaderParameter<float>("rainDensity", density);
}

void VRRain::setScale( double scale ){
    if ( scale<0 || scale>10 ) {
        //TODO: error
        cout << "Input for Scale of Rain between 0 and 10\n";
    }
    else {
        this->scale = scale;
        scaleRN = scale;
        density = 0.2 * 10/scale;
        mat->setShaderParameter<float>("rainDensity", density);
    }
}

void VRRain::overrideParameters( double density, double durationTransition, double color, double light ) {
    this->density = density;
    mat->setShaderParameter<float>("rainDensity", density);
}

Vec2d VRRain::get() {
    return Vec2d(density,scale);
}

void VRRain::update() {
    if (!isVisible()) return;

    double offset = glutGet(GLUT_ELAPSED_TIME)*0.001; //seconds

    mat->setShaderParameter<float>("rainOffset", offset);
    mat->readVertexShader(vScript);
    mat->readFragmentShader(fScript);

    //Vec3d tmp = camDef->getFrom();
    //camTex->setFrom(Vec3d(tmp[0],tmp[1]+40,tmp[2]));
    //camTex->setAt(tmp);
    //camTex->setUp(Vec3d(0,0,1));
}

void VRRain::reloadShader() {
    //sky->setClouds(1, 1e-5, 3000, Vec2d(0.004,0.004),Color4f(0.3,0.3,0.3,1));
    //sky.setClouds(float density, float scale, float height, Vec2d speed, Color4f color);

    /*
    string resDir = VRSceneManager::get()->getOriginalWorkdir() + "/shader/Rain/";
    mat->readVertexShader(resDir + "Rain.vp");
    mat->readFragmentShader(resDir + "Rain.fp");
    */
}

