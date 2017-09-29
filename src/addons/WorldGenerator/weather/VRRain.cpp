#include "VRRain.h"
#include <random>

using namespace OSG;

VRRain::VRRain() {}
VRRain::~VRRain() {}

VRRainPtr VRRain::create(string name) { return VRRainPtr( new VRRain() ); }

void VRRain::startRain() {
    setRainScale();
    setupRain();
}

void VRRain::stopRain() {
    clearRain();
}

void VRRain::setupRain() {
    //TODO: SETUP RAINCLUSTER HERE
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
}

void VRRain::clearRain(){
    //TODO: list of raindropemitters, stop/delete them here

    //TODO: callback [clouds, light-diffuse, rain]
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
}

void VRRain::setRain( double durationTransition, double scaleRain ){
    if ( durationTransition<0 || scaleRain<0 || scaleRain>10 ) {
        //TODO: error
        printf("Input for Duration of Transition must be greater 0, Scale of Rain between 0 and 10");
    }
    else {
        this->durationTransition = durationTransition;
        this->scaleRain = scaleRain;
    }
}

void VRRain::setRainScale() {
    //TODO: tweak Parameters before|after transition
    densityRain = scaleRain/10;             //scale 10 => 0.1|1
    speedRainX = 1/4500*scaleRain + 2/1125; //scale 10 => 0.002|0.004
    speedRainY = 1/3000*scaleRain + 1/1500; //scale 10 => 0.001|0.004
    colorRain = -4/45*scaleRain + 49/45;    //scale 10 => 1|0.2
    lightRain = -1/15*scaleRain + 16/15;    //scale 10 => 1|0.4

    /* better use a variant where everything uses the original values, unlike above
    densityRain = densityRainStart * scaleRain;   //scale 10 => 0.1|1
    speedRainX = speedRainStartX * scaleRain/5;   //scale 10 => 0.002|0.004
    speedRainY = speedRainStartY * scaleRain/5*2; //scale 10 => 0.001|0.004
    colorRain = colorRainStart/scaleRain*2;    //scale 10 => 1|0.2
    lightRain = lightRainStart/scaleRain*4;    //scale 10 => 1|0.4
    */
}

void VRRain::overrideParameters( double densityRain, double speedRain, double colorRain, double lightRain ) {
    this->densityRain = densityRain;
    this->speedRainX = speedRain;
    this->speedRainY = speedRain;
    this->colorRain = colorRain;
    this->lightRain = lightRain;
}

Vec2d VRRain::getRain() {
    return Vec2d(durationTransition,scaleRain);
}
