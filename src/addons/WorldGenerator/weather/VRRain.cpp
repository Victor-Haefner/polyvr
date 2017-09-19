#include "VRRain.h"

using namespace OSG;

VRRain::VRRain(){
    VRParticles.setLifetime(1,0);
}
VRRain::~VRRain(){

}

void VRRain::startRain() {
    setRainScale();
    //TODO: callback
}

void VRRain::stopRain() {

}

void VRRain::setRain( double durationTransition, double scaleRain ){
    if ( durationTransition<0 || scaleRain<0 || scaleRain>10 ) {
        //TODO: error
    }
    else {
        this->durationTrans = durationTransition;
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

void overrideParameters( double densityRain, double speedRain, double colorRain, double lightRain ) {
    this->densityRain = densityRain;
    this->speedRain = speedRain;
    this->colorRain = colorRain;
    this->lightRain = lightRain;
}

Vec2d VRRain::getRain() {
    //TODO: return vec of duration, and scale
}
