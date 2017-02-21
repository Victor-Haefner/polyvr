#include "VRSky.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRMaterialT.h"
#include "core/objects/material/VRTextureGenerator.h"
#include "core/utils/VRFunction.h"
#include "core/scene/VRScene.h"
#include <math.h>

#include <boost/bind.hpp>
#include <time.h>
#include <GL/glut.h>

using namespace OSG;

void VRSky::Date::propagate(double seconds) {
    int sign = (seconds > 0) - (seconds < 0); // check sign
    second += seconds;
    if (second >= 3600 || second < 0) {
        second -= 3600;
        hour += 1*sign;
        if (hour >= 24 || second < 0) {
            hour -= 24;
            day += 1*sign;
            if (day >= 365 || second < 0) {
                day -= 365;
                year += 1*sign;
            }
        }
    }
}

double VRSky::Date::getDay() {
    return day + (second + hour*3600) / (24*3600);
}

VRSky::VRSky() : VRGeometry("Sky") {
    type = "Sky";
    string resDir = VRSceneManager::get()->getOriginalWorkdir() + "/shader/Sky/";
    string vScript = resDir + "Sky.vp";
    string fScript = resDir + "Sky.fp";
    textureSize = 128;
    speed = 1;
    mat = VRMaterial::create("Sky");
    mat->readVertexShader(vScript);
    mat->readFragmentShader(fScript);
    setMaterial(mat);
    setPrimitive("Plane", "2 2 1 1");
    mat->setLit(false);
	mat->setDiffuse(OSG::Vec3f(1.,1.,1.));

    sunPos = sunFromTime();
	mat->setShaderParameter<Vec3f>("sunPos", sunPos);

	cloudVel =  Vec2f(1000, 200);
	cloudOffset =  Vec2f(0, 0);
    mat->setShaderParameter<Vec2f>("cloudOffset", cloudOffset);

	VRTextureGenerator tg;
	tg.setSize(textureSize, textureSize);
	tg.add(PERLIN, 1, Vec3f(0.9), Vec3f(1.0));
	tg.add(PERLIN, 1.0/2, Vec3f(0.7), Vec3f(1.0));
	tg.add(PERLIN, 1.0/4, Vec3f(0.6), Vec3f(1.0));
	tg.add(PERLIN, 1.0/8, Vec3f(0.5), Vec3f(1.0));
	tg.add(PERLIN, 1.0/16, Vec3f(0.4), Vec3f(1.0));

	mat->setTexture(tg.compose(0));

    allowCulling(false, true);

    updatePtr = VRFunction<int>::create("sky update", boost::bind(&VRSky::update, this));
    VRScene::getCurrent()->addUpdateFkt(updatePtr);

    // test date function
    setTime(3601, 23, 364, 1);

}

VRSky::~VRSky() {}

VRSkyPtr VRSky::create() { return VRSkyPtr( new VRSky() ); }
VRSkyPtr VRSky::ptr() { return static_pointer_cast<VRSky>( shared_from_this() ); }

void VRSky::update() {
    if (!isVisible()) return;

    // update time
    double current = glutGet(GLUT_ELAPSED_TIME)*0.001;
    float dt = (current - lastTime)*speed;
    lastTime = current;

    date.propagate(dt);

    // update sun
    sunPos = sunFromTime();
    sunPos[1] = cos(current);

    mat->setShaderParameter<Vec3f>("sunPos", sunPos);
    // update clouds
    updateClouds(dt);
    mat->setShaderParameter<Vec2f>("cloudOffset", cloudOffset);

}

void VRSky::setTime(double second, int hour, int day, int year) {
    bool warning = false;
    if (second >= 0 && second < 3600) date.second = second;
    else {
        date.second = 0;
        warning = true;
    }
    if (hour >= 0 && hour < 24) date.hour = hour;
    else {
        date.hour = 0;
        warning = true;
    }
    if (day >= 0 && day < 365) date.day = day; // TODO: adjust for leap
    else {
        date.day = 0;
        warning = true;
    }
    date.year = year;
    if (warning) {
        std::cout<<"WARNING: setTime(double second, int hour, int day, int year = 2000) must be of format:\n";
        std::cout<<"seconds in [0,3600), hours in [0,24), days in [0, 365), year.\n";
        std::cout<<"Date set as: day "<<date.day<<" of year "<<date.year<<" at "
                <<date.hour<<":"<<date.second/60<<":"<<fmod(date.second,60)<<std::endl;
    }
}

void VRSky::setCloudVel(float x, float z) {
    cloudVel = Vec2f(x, z);
}

void VRSky::updateClouds(float dt) {
    cloudOffset += cloudVel * dt;
    cloudOffset[0] = fmod(cloudOffset[0], textureSize);
    cloudOffset[1] = fmod(cloudOffset[1], textureSize);
    //if (cloudOffset[0] == 0 && cloudOffset[1] == 0) {
    //    std::cout<<"no offset"<<std::endl;
    //    std::cout<<"dt: "<<dt<<" Vel.: "<<cloudVel[0]<<" "<<cloudVel[1];
    //    std::cout<<"\nOff.: "<<cloudOffset[0]<<" "<<cloudOffset[1]<<std::endl;
    //}
}

Vec3f VRSky::sunFromTime() {
	// lets set the lattitude and longitude
	// Karlsruhe 49.0069° N, 8.4037° E
	// http://aa.usno.navy.mil/faq/docs/SunApprox.php

	// and time of day

	// ...and time of year...

	// ...and year - lets assume 1. jan 2000 12:00 GMT (=&gt; n=0) then
	//D = JD - 2451545.0
	float day = date.getDay();

	// use these values to calculate the sun angle

	// all in deg
	float g = 357.529 + 0.98560028 * day; // mean anomaly of sun
	float q = 280.459 + 0.98564736 * day; // mean longitude of sun
	float L = q + 1.915 * sin(g) + 0.020 * sin(2*g); // geocentric (apparent/adjusted) elliptic
	// long. of sun

	// map to range 360 deg?

	// approx. ecliptic latitude as b=0.
	// approx. distance of the Sun from the Earth R (AU)
	float R = 1.00014 - 0.01671 * cos(g) - 0.00014 * cos(2*g);
	// obliquity (deg)
	float e = 23.439 - 0.00000036 * day;

	float RA = atan2(cos(e) * sin(L), cos(L)); // right ascension
	float d = asin(sin(e) * sin(L)); // declination

    return Vec3f(1, 0, 0);
}





