#include "VRRain.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRMaterialT.h"
#include "core/objects/material/VRTextureGenerator.h"
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
	mat->enableTransparency();

    setVolumeCheck(false, true);

    //textureSize = 2048;
    textureSize = 512;
	VRTextureGenerator tg;
	tg.setSize(textureSize, textureSize);
    tg.add(PERLIN, 1, Color3f(0.9), Color3f(1.0));
	tg.add(PERLIN, 1.0/2, Color3f(0.8), Color3f(1.0));
	tg.add(PERLIN, 1.0/4, Color3f(0.7), Color3f(1.0));
	tg.add(PERLIN, 1.0/8, Color3f(0.6), Color3f(1.0));
	tg.add(PERLIN, 1.0/16, Color3f(0.4), Color3f(1.0));
	tg.add(PERLIN, 1.0/32, Color3f(0.2), Color3f(1.0));
	tg.add(PERLIN, 1.0/64, Color3f(0.1), Color3f(1.0));
	tg.add(PERLIN, 1.0/128, Color3f(0.1), Color3f(1.0));
	mat->setTexture(tg.compose(0));

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
        //TODO: SETUP RAINCLUSTER HERE
    /*
    for (int x=-7; x<7; x++) {
        for (int y=-7; y<7; y++) {
            VRParticles rainemitter = VRParticles("rd"+x+y);
            rainemitter.setLifetime(2,0);

            int range = 600 - 300 + 1;
            int num = rand() % range + 300;

            Vec3d basevec = Vec3d(x*0.1,num,y*0.1);
            Vec3d dirvec = Vec3d(0,-1,0);
            int fromR = 0;
            int toR = 10;
            rainemitter.setEmitter(basevec,dirvec,fromR,toR,100,false);

        }
    }
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
    if (scale == 0) setScale(1.3);
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
    setScale(0);
}


void VRRain::setScale( double scale ){
    if ( scale<0 || scale>10 ) {
        //TODO: error
        cout << "Input for Scale of Rain between 0 and 10\n";
    }
    else {
        //this->durationTransition = durationTransition;
        this->scale = scale;
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

void VRRain::updateRain(float dt) {
    /*
    cloudOffset += Vec2f(cloudVel) * dt;
    cloudOffset[0] = fmod(cloudOffset[0], textureSize);
    cloudOffset[1] = fmod(cloudOffset[1], textureSize);
    mat->setShaderParameter<Vec2f>("cloudOffset", cloudOffset);
    */
    //mat->setShaderParameter<Vec2f>("rainOffset", rainOffset);
    offset += dt;
    mat->setShaderParameter<float>("rainOffset", offset);
    mat->readVertexShader(vScript);
    mat->readFragmentShader(fScript);
}

void VRRain::update() {
    if (!isVisible()) return;

    double current = glutGet(GLUT_ELAPSED_TIME)*0.001; //seconds
    float dt = (current - lastTime)*0.01; //1/100 of a second
    lastTime = current;

    offset += dt;
    mat->setShaderParameter<float>("rainOffset", offset);
    mat->readVertexShader(vScript);
    mat->readFragmentShader(fScript);
}

void VRRain::reloadShader() {
    string resDir = VRSceneManager::get()->getOriginalWorkdir() + "/shader/Rain/";
    mat->readVertexShader(resDir + "Rain.vp");
    mat->readFragmentShader(resDir + "Rain.fp");
}

