#include "VRSky.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRMaterialT.h"
#include "core/objects/material/VRTextureGenerator.h"
#include "core/utils/VRFunction.h"
#include "core/utils/system/VRSystem.h"
#include "core/scene/VRScene.h"

#include <math.h>
#include <time.h>

using namespace OSG;

template<> string typeName(const VRSky& o) { return "Sky"; }


// not accounting for leap or other inconsistencies
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
    double d1 = (year - 2000)*365 + day;
    double d2 = (second/3600.0 + hour - 12) / 24.0;
    //cout << "VRSky::Date::getDay " << std::setprecision(16) << d1 << " " << d2 << " " << double(d1 + d2) << endl;
    return double(d1 + d2);
}

VRSky::VRSky() : VRGeometry("Sky") {
    type = "Sky";

    // shader setup
    mat = VRMaterial::create("Sky");
    reloadShader();
    setMaterial(mat);
    setPrimitive("Plane 2 2 1 1");
    mat->setLit(false);
	mat->setDiffuse(Color3f(1));

    setPosition(49.0069, 8.4037); // 49.0069° N, 8.4037° E Karlsruhe
    setTime(0,12,120,2016);
    sunFromTime();
    setClouds(0.1, 1e-5, 3000, Vec2d(.002, .001), Color4f(1,1,1, 1.0));
	setLuminance(1.75);
    setVolumeCheck(false, true);

    //textureSize = 2048;
    textureSize = 512;
	VRTextureGenerator tg;
	tg.setSize(textureSize, textureSize);
    tg.add(PERLIN, 1, Color4f(0.9), Color4f(1.0));
	tg.add(PERLIN, 1.0/2, Color4f(0.8), Color4f(1.0));
	tg.add(PERLIN, 1.0/4, Color4f(0.7), Color4f(1.0));
	tg.add(PERLIN, 1.0/8, Color4f(0.6), Color4f(1.0));
	tg.add(PERLIN, 1.0/16, Color4f(0.4), Color4f(1.0));
	tg.add(PERLIN, 1.0/32, Color4f(0.2), Color4f(1.0));
	tg.add(PERLIN, 1.0/64, Color4f(0.1), Color4f(1.0));
	tg.add(PERLIN, 1.0/128, Color4f(0.1), Color4f(1.0));
	/*
	tg.add(PERLIN, 1, Color3f(0.95), Color3f(1.0));
	tg.add(PERLIN, 1.0/2, Color3f(0.9), Color3f(1.0));
	tg.add(PERLIN, 1.0/4, Color3f(0.7), Color3f(1.0));
	tg.add(PERLIN, 1.0/8, Color3f(0.6), Color3f(1.0));
	tg.add(PERLIN, 1.0/16, Color3f(0.5), Color3f(1.0));
	tg.add(PERLIN, 1.0/32, Color3f(0.4), Color3f(1.0));
	tg.add(PERLIN, 1.0/64, Color3f(0.4), Color3f(1.0));
	tg.add(PERLIN, 1.0/128, Color3f(0.3), Color3f(1.0));
	tg.add(PERLIN, 1.0/256, Color3f(0.2), Color3f(1.0));
	*/
	mat->setTexture(tg.compose(0));

    updatePtr = VRUpdateCb::create("sky update", bind(&VRSky::update, this));
    VRScene::getCurrent()->addUpdateFkt(updatePtr);
}

VRSky::~VRSky() {}

VRSkyPtr VRSky::create() {
    auto sky = VRSkyPtr( new VRSky() );
    sky->hide("SHADOW");
    return sky;
}

VRSkyPtr VRSky::ptr() { return static_pointer_cast<VRSky>( shared_from_this() ); }

void VRSky::update() {
    if (!isVisible()) return;

    double current = getTime()*1e-6;
    float dt = (current - lastTime)*speed;
    lastTime = current;

    date.propagate(dt);
    sunFromTime();
    updateClouds(dt);
    calculateZenithColor();
}

void VRSky::setSpeed(float s) { speed = s; }

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

void VRSky::setCloudVel(float x, float z) { cloudVel = Vec2d(x, z); }

void VRSky::updateClouds(float dt) {
    cloudOffset += Vec2f(cloudVel) * dt;
    cloudOffset[0] = fmod(cloudOffset[0], textureSize);
    cloudOffset[1] = fmod(cloudOffset[1], textureSize);
    mat->setShaderParameter<Vec2f>("cloudOffset", cloudOffset);
}

void VRSky::calculateZenithColor() {
	float chi = ((4. / 9.) - (turbidity / 120.) ) * ( Pi - 2 * theta_s );
	xyY_z[2] = (4.0453 * turbidity - 4.9710) * tan(chi) - 0.2155 * turbidity + 2.4192; // get luminance

	Vec4d vTheta_s = Vec4d(theta_s*theta_s*theta_s, theta_s*theta_s, theta_s, 1);
	Vec4d vTurbidities = Vec4d(turbidity*turbidity, turbidity, 1, 0);

	// this format in row-major order, all others column major
	Matrix4d x_mat = Matrix4d( 0.0017, -0.0037, 0.0021, 0.000,
                          -0.0290, 0.0638, -0.0320, 0.0039,
                           0.1169, -0.2120, 0.0605, 0.2589,
                           0., 0., 0., 0.);
	Matrix4d y_mat = Matrix4d(0.0028, -0.0061, 0.0032, 0.000,
                         -0.0421, 0.0897, -0.0415, 0.0052,
                          0.1535, -0.2676, 0.0667, 0.2669,
                          0., 0., 0., 0.);

    Vec4d tmp;
    x_mat.mult(vTheta_s, tmp);
    xyY_z[0] = vTurbidities.dot(tmp);
    y_mat.mult(vTheta_s, tmp);
    xyY_z[1] = vTurbidities.dot(tmp);

    mat->setShaderParameter<Vec3f>("xyY_z", xyY_z);
}

void VRSky::sunFromTime() {

	double day = date.getDay();
	double g = 357.529 + 0.98560028 * day; // mean anomaly of sun
	double q = 280.459 + 0.98564736 * day; // mean longitude of sun
	double L = (q + 1.915 * sin(g*Pi/180) + 0.020 * sin(2*g*Pi/180))*Pi/180; // geocentric (apparent/adjusted) elliptic (in rad)
	// long. of sun

	// approx. ecliptic latitude as b=0.
	// obliquity (deg)
	double e = (23.439 - 0.00000036 * day)*Pi/180; // (in rad)

	double right_ascension = atan2(cos(e) * sin(L), cos(L)); // right ascension (in rad)
	double declination = asin(sin(e) * sin(L)); // declination (in rad)

    // convert right ascension to hour angle
    double gmst = 18.697374558 + 24.06570982441908 * day; // Greenwich mean sidereal time
    double hour_angle = (gmst*15 - observerPosition.longitude - right_ascension); // (deg)
    // map hour angle onto (0,2pi)
    hour_angle = fmod(hour_angle,360)*Pi/180; // (rad)

    // solar zenith
    double cos_theta_s = sin(observerPosition.latitude*Pi/180)*sin(declination)
                   + cos(observerPosition.latitude*Pi/180)*cos(declination)*cos(hour_angle);

    theta_s = acos(cos_theta_s);
    double sin_theta_s = sin(theta_s); // check mapping

    //solar azimuth
    // z = cos(phi) * sin(theta)
    double z = sin(declination)*cos(observerPosition.latitude*Pi/180)
                -cos(hour_angle)*cos(declination)*sin(observerPosition.latitude*Pi/180);

    double phi_s = acos(z/sin_theta_s);
    // take care of negatives
    if (hour_angle > Pi) phi_s *= -1;

    // let +z dir be south => x is east
    // azimuth = 0 => sun directly south should be defines as angle to z axis
    // also rotating clockwise (-ve)
    // r = 1 (normalised)
    double y = cos_theta_s;
    double x = sin(phi_s)*sin_theta_s;

    sunPos = Vec3f(x, y, z);
    //cout << "kajshdgkajgs " << date.hour << " " << date.second << "   " << sunPos << endl;

    mat->setShaderParameter<float>("theta_s", theta_s);
    mat->setShaderParameter<Vec3f>("sunPos", sunPos);
}

void VRSky::setClouds(float density, float scale, float height, Vec2d vel, Color4f color) {
    cloudDensity = density;
    cloudScale = scale;
    cloudHeight = height;
    cloudVel = vel;
    cloudColor = color;
    mat->setShaderParameter<float>("cloudDensity", cloudDensity);
    mat->setShaderParameter<float>("cloudScale", cloudScale);
    mat->setShaderParameter<float>("cloudHeight", cloudHeight);
    mat->setShaderParameter<Vec2f>("cloudOffset", cloudOffset);
    mat->setShaderParameter<Color4f>("cloudColor", cloudColor);
}

void VRSky::setLuminance(float t) {
    turbidity = t;
    mat->setShaderParameter<float>("turbidity", turbidity);

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

int VRSky::getHour(){
    return date.hour;
}

Vec3d VRSky::getSunPos(){
    return Vec3d(sunPos[0],sunPos[1],sunPos[2]);
}

void VRSky::reloadShader() {
    cout << "VRSky::reloadShader" << endl;
    string resDir = VRSceneManager::get()->getOriginalWorkdir() + "/shader/Sky/";
    mat->readVertexShader(resDir + "Sky.vp");
    mat->readFragmentShader(resDir + "Sky.fp");
    mat->readFragmentShader(resDir + "Sky.dfp", true);
    mat->updateDeferredShader();
}


