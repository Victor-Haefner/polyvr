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
	rainOffset = 0;
	mat->setShaderParameter<float>("rainOffset", rainOffset);

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

void VRRain::startRain() {
    //setRainScale();
    //setupRain();
    cout << "VRRain::startRain()\n";
}

void VRRain::stopRain() {
    //clearRain();
}

void VRRain::setupRain() {
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
}

void VRRain::clearRain(){
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
    */
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

void VRRain::updateRain(float dt) {
    /*
    cloudOffset += Vec2f(cloudVel) * dt;
    cloudOffset[0] = fmod(cloudOffset[0], textureSize);
    cloudOffset[1] = fmod(cloudOffset[1], textureSize);
    mat->setShaderParameter<Vec2f>("cloudOffset", cloudOffset);
    */
    //mat->setShaderParameter<Vec2f>("rainOffset", rainOffset);
    rainOffset += dt;
    mat->setShaderParameter<float>("rainOffset", rainOffset);
    mat->readVertexShader(vScript);
    mat->readFragmentShader(fScript);
}

// SKY METHODS ---------------------------------------------------------------------------------------------------
void VRRain::update() {
    if (!isVisible()) return;

    double current = glutGet(GLUT_ELAPSED_TIME)*0.001;
    float dt = (current - lastTime)*speed;
    lastTime = current;

    date.propagate(dt);
    sunFromTime();
    updateClouds(dt);
    calculateZenithColor();

}

void VRRain::Date::propagate(double seconds) {
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
double VRRain::Date::getDay() { // returns time since 12:00 at 1-1-2000
    // not accounting for leap or other inconsistencies
    double d1 = (year - 2000)*365 + day;
    double d2 = (second/3600.0 + hour - 12) / 24.0;
    //cout << "VRSky::Date::getDay " << std::setprecision(16) << d1 << " " << d2 << " " << double(d1 + d2) << endl;
    return double(d1 + d2);
}
void VRRain::setSpeed(float s) { speed = s; }

void VRRain::setTime(double second, int hour, int day, int year) {
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

void VRRain::setPosition(float latitude, float longitude) {
    observerPosition.latitude = latitude;
    observerPosition.longitude = longitude;
}

void VRRain::setCloudVel(float x, float z) {
    cloudVel = Vec2d(x, z);
}

void VRRain::updateClouds(float dt) {
    cloudOffset += Vec2f(cloudVel) * dt;
    cloudOffset[0] = fmod(cloudOffset[0], textureSize);
    cloudOffset[1] = fmod(cloudOffset[1], textureSize);
    mat->setShaderParameter<Vec2f>("cloudOffset", cloudOffset);
}

void VRRain::calculateZenithColor() {
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

void VRRain::sunFromTime() {

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

void VRRain::setClouds(float density, float scale, float height, Vec2d vel, Color4f color) {
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

void VRRain::setLuminance(float t) {
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

void VRRain::reloadShader() {
    string resDir = VRSceneManager::get()->getOriginalWorkdir() + "/shader/Rain/";
    mat->readVertexShader(resDir + "Rain.vp");
    mat->readFragmentShader(resDir + "Rain.fp");
}

