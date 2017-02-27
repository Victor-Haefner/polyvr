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

double VRSky::Date::getDay() { // returns time since 12:00 at 1-1-2000
    // not accounting for leap or other inconsistencies
    return (year - 2000)*365 + day + (second/3600 + hour - 12) / 24;
}

VRSky::VRSky() : VRGeometry("Sky") {
    type = "Sky";
    string resDir = VRSceneManager::get()->getOriginalWorkdir() + "/shader/Sky/";
    string vScript = resDir + "Sky.vp";
    string fScript = resDir + "Sky.fp";

    // TODO: break up into separate setup functions

	// observer params
    speed = 1;
    // 49.0069° N, 8.4037° E Karlsruhe
    observerPosition.latitude = 49.0069;
    observerPosition.longitude = 8.4037;

    setTime(0,12,120,2016);

    // shader setup
    mat = VRMaterial::create("Sky");
    mat->readVertexShader(vScript);
    mat->readFragmentShader(fScript);
    setMaterial(mat);
    setPrimitive("Plane", "2 2 1 1");
    mat->setLit(false);
	mat->setDiffuse(Vec3f(1));



    // sun params
    sunFromTime();
    setTurbidity(2.);

    float factor = 1/speed;

    // cloud params
    cloudDensity = 0.1;
    cloudScale = 2e-5;
    cloudHeight = 3000.;
    cloudVel =  Vec2f(.005*factor, .0003*factor);
	cloudOffset =  Vec2f(0, 0);
    mat->setShaderParameter<float>("cloudDensity", cloudDensity);
    mat->setShaderParameter<float>("cloudScale", cloudScale);
    mat->setShaderParameter<float>("cloudHeight", cloudHeight);
    mat->setShaderParameter<Vec2f>("cloudOffset", cloudOffset);

    //textureSize = 2048;
    textureSize = 512;
	VRTextureGenerator tg;
	tg.setSize(textureSize, textureSize);
    tg.add(PERLIN, 1, Vec3f(0.9), Vec3f(1.0));
	tg.add(PERLIN, 1.0/2, Vec3f(0.8), Vec3f(1.0));
	tg.add(PERLIN, 1.0/4, Vec3f(0.7), Vec3f(1.0));
	tg.add(PERLIN, 1.0/8, Vec3f(0.6), Vec3f(1.0));
	tg.add(PERLIN, 1.0/16, Vec3f(0.4), Vec3f(1.0));
	tg.add(PERLIN, 1.0/32, Vec3f(0.2), Vec3f(1.0));
	tg.add(PERLIN, 1.0/64, Vec3f(0.1), Vec3f(1.0));
	/*
	tg.add(PERLIN, 1, Vec3f(0.95), Vec3f(1.0));
	tg.add(PERLIN, 1.0/2, Vec3f(0.9), Vec3f(1.0));
	tg.add(PERLIN, 1.0/4, Vec3f(0.7), Vec3f(1.0));
	tg.add(PERLIN, 1.0/8, Vec3f(0.6), Vec3f(1.0));
	tg.add(PERLIN, 1.0/16, Vec3f(0.5), Vec3f(1.0));
	tg.add(PERLIN, 1.0/32, Vec3f(0.4), Vec3f(1.0));
	tg.add(PERLIN, 1.0/64, Vec3f(0.4), Vec3f(1.0));
	tg.add(PERLIN, 1.0/128, Vec3f(0.3), Vec3f(1.0));
	tg.add(PERLIN, 1.0/256, Vec3f(0.2), Vec3f(1.0));
	*/
	mat->setTexture(tg.compose(0));

	// sky params
    calculateCoeffs();

    calculateZenithColor();

    allowCulling(false, true);

    updatePtr = VRFunction<int>::create("sky update", boost::bind(&VRSky::update, this));
    VRScene::getCurrent()->addUpdateFkt(updatePtr);
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
    sunFromTime();
    // update clouds
    updateClouds(dt);
    calculateZenithColor();

}

void VRSky::setTurbidity(float t) {
    turbidity = t;
    mat->setShaderParameter<float>("turbidity", turbidity);
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
                <<date.hour<<":"<<floor(date.second/60)<<":"<<fmod(date.second,60)<<std::endl;
    }
}

void VRSky::setPosition(float latitude, float longitude) {
    observerPosition.latitude = latitude;
    observerPosition.longitude = longitude;
}

void VRSky::setCloudVel(float x, float z) {
    cloudVel = Vec2f(x, z);
}

void VRSky::updateClouds(float dt) {
    cloudOffset += cloudVel * dt;
    cloudOffset[0] = fmod(cloudOffset[0], textureSize);
    cloudOffset[1] = fmod(cloudOffset[1], textureSize);
    mat->setShaderParameter<Vec2f>("cloudOffset", cloudOffset);
}

void VRSky::calculateZenithColor() {
	float chi = ((4. / 9.) - (turbidity / 120.) ) * ( Pi - 2 * theta_s );
	xyY_z[2] = (4.0453 * turbidity - 4.9710) * tan(chi) - 0.2155 * turbidity + 2.4192; // get luminance

	Vec4f vTheta_s = Vec4f(theta_s*theta_s*theta_s, theta_s*theta_s, theta_s, 1);
	Vec4f vTurbidities = Vec4f(turbidity*turbidity, turbidity, 1, 0);

	// this format in row-major order, all others column major
	Matrix x_mat = Matrix( 0.0017, -0.0037, 0.0021, 0.000,
                          -0.0290, 0.0638, -0.0320, 0.0039,
                           0.1169, -0.2120, 0.0605, 0.2589,
                           0., 0., 0., 0.);
	Matrix y_mat = Matrix(0.0028, -0.0061, 0.0032, 0.000,
                         -0.0421, 0.0897, -0.0415, 0.0052,
                          0.1535, -0.2676, 0.0667, 0.2669,
                          0., 0., 0., 0.);

    Vec4f tmp;
    x_mat.mult(vTheta_s, tmp);
    xyY_z[0] = vTurbidities.dot(tmp);
    y_mat.mult(vTheta_s, tmp);
    xyY_z[1] = vTurbidities.dot(tmp);

    mat->setShaderParameter<Vec3f>("xyY_z", xyY_z);
}

void VRSky::calculateCoeffs() {
    coeffsA[2] = 0.1787 * turbidity - 1.4630;
    coeffsB[2] = - 0.3554 * turbidity + 0.4275;
    coeffsC[2] = - 0.0227 * turbidity + 5.3251;
    coeffsD[2] = 0.1206 * turbidity - 2.5771;
    coeffsE[2] = - 0.0670 * turbidity + 0.3703;

    coeffsA[0] = - 0.0193 * turbidity - 0.2592;
    coeffsB[0] = - 0.0665 * turbidity + 0.0008;
    coeffsC[0] = - 0.0004 * turbidity + 0.2125;
    coeffsD[0] = - 0.0641 * turbidity - 0.8989;
    coeffsE[0] = - 0.0033 * turbidity + 0.0452;

    coeffsA[1] = - 0.0167 * turbidity - 0.2608;
    coeffsB[1] = - 0.0950 * turbidity + 0.0092;
    coeffsC[1] = - 0.0079 * turbidity + 0.2102;
    coeffsD[1] = - 0.0441 * turbidity - 1.6537;
    coeffsE[1] = - 0.0109 * turbidity + 0.0529;

    mat->setShaderParameter<Vec3f>("A", coeffsA);
    mat->setShaderParameter<Vec3f>("B", coeffsB);
    mat->setShaderParameter<Vec3f>("C", coeffsC);
    mat->setShaderParameter<Vec3f>("D", coeffsD);
    mat->setShaderParameter<Vec3f>("E", coeffsE);
}

void VRSky::sunFromTime() {

	float day = date.getDay();
	float g = 357.529 + 0.98560028 * day; // mean anomaly of sun
	float q = 280.459 + 0.98564736 * day; // mean longitude of sun
	float L = (q + 1.915 * sin(g*Pi/180) + 0.020 * sin(2*g*Pi/180))*Pi/180; // geocentric (apparent/adjusted) elliptic (in rad)
	// long. of sun

	// approx. ecliptic latitude as b=0.
	// obliquity (deg)
	float e = (23.439 - 0.00000036 * day)*Pi/180; // (in rad)

	float right_ascension = atan2(cos(e) * sin(L), cos(L)); // right ascension (in rad)
	float declination = asin(sin(e) * sin(L)); // declination (in rad)

    // convert right ascension to hour angle
    float gmst = 18.697374558 + 24.06570982441908 * day; // Greenwich mean sidereal time
    float hour_angle = (gmst*15 - observerPosition.longitude - right_ascension); // (deg)
    // map hour angle onto (0,2pi)
    hour_angle = fmod(hour_angle,360)*Pi/180; // (rad)

    // solar zenith
    float cos_theta_s = sin(observerPosition.latitude*Pi/180)*sin(declination)
                   + cos(observerPosition.latitude*Pi/180)*cos(declination)*cos(hour_angle);

    theta_s = acos(cos_theta_s);
    float sin_theta_s = sin(theta_s); // check mapping

    //solar azimuth
    // z = cos(phi) * sin(theta)
    float z = sin(declination)*cos(observerPosition.latitude*Pi/180)
                -cos(hour_angle)*cos(declination)*sin(observerPosition.latitude*Pi/180);

    float phi_s = acos(z/sin_theta_s);
    // take care of negatives
    if (hour_angle > Pi) phi_s *= -1;

    // let +z dir be south => x is east
    // azimuth = 0 => sun directly south should be defines as angle to z axis
    // also rotating clockwise (-ve)
    // r = 1 (normalised)
    float y = cos_theta_s;
    float x = sin(phi_s)*sin_theta_s;

    sunPos = Vec3f(x, y, z);

    mat->setShaderParameter<float>("theta_s", theta_s);
    mat->setShaderParameter<Vec3f>("sunPos", sunPos);
}


void VRSky::reloadShader() {
    string resDir = VRSceneManager::get()->getOriginalWorkdir() + "/shader/Sky/";
    mat->readVertexShader(resDir + "Sky.vp");
    mat->readFragmentShader(resDir + "Sky.fp");
}




