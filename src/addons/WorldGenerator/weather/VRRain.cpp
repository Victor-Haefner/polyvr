#include "VRRain.h"
#include <random>

using namespace OSG;

VRRain::VRRain() {}
VRRain::~VRRain() {}

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
    //
}

void VRRain::startRain() {
    setRainScale();
    setupRain();
}

void VRRain::stopRain() {
    //TODO: list of raindropemitters, stop/delete them here
}

void VRRain::setRain( double durationTransition, double scaleRain ){
    if ( durationTransition<0 || scaleRain<0 || scaleRain>10 ) {
        //TODO: error
    }
    else {
        this->durationTransition = durationTransition;
        this->scaleRain = scaleRain;
    }
}

void VRRain::setRainScale() {
    //TODO: tweak Parameters
    densityRain = scaleRain*densityRainStart;
    speedRain = scaleRain*0.3*speedRainStart;
    colorRain = colorRainStart - scaleRain*0.05;
    lightRain = lightRainStart - scaleRain*0.05;
}

void VRRain::overrideParameters( double densityRain, double speedRain, double colorRain, double lightRain ) {
    this->densityRain = densityRain;
    this->speedRain = speedRain;
    this->colorRain = colorRain;
    this->lightRain = lightRain;
}

Vec2d VRRain::getRain() {
    return Vec2d(durationTransition,scaleRain);
}
