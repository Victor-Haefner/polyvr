#include "VRSky.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRMaterialT.h"
#include "core/objects/material/VRTextureGenerator.h"
#include "core/objects/VRCamera.h"
#include "core/utils/VRFunction.h"
#include "core/utils/system/VRSystem.h"
#include "core/scene/VRScene.h"
#include "core/setup/VRSetup.h"
#include "core/setup/windows/VRView.h"

#include <math.h>
#include <time.h>

#define GLSL(shader) #shader

using namespace OSG;


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
    update();

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

#ifdef WASM
    // get inverse of modelviewprojection for wasm
    VRCameraPtr cam = VRScene::getCurrent()->getActiveCamera();
    Matrix mModelView = toMatrix4f(VRScene::getCurrent()->getActiveCamera()->getWorldMatrix());
    mModelView[3] = Vec4f(0,0,0,mModelView[3][3]);
    mModelView.invert();
    Vec2i viewSize = VRSetup::getCurrent()->getView(0)->getSize();
    Matrix mProj = cam->getProjectionMatrix(viewSize[0], viewSize[1]);
    mModelView.multLeft(mProj);
    mModelView.invert();
    mat->setShaderParameter("extInvMat", mModelView);
#endif

    double current = getTime()*1e-6;
    float dt = (current - lastTime)*speed;
    lastTime = current;

    date.propagate(dt);
    sunFromTime();
    updateClouds(dt);
    calculateZenithColor();
}

void VRSky::setSpeed(float s) { speed = s; }
float VRSky::getSpeed() { return speed; }

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
    mat->setVertexShader(skyVP, "skyVP");
    mat->setFragmentShader(skyFP, "skyFP");
    mat->readFragmentShader(resDir + "Sky.dfp", true); // TODO
    Matrix m;
    mat->setShaderParameter("extInvMat", m);
    mat->updateDeferredShader();
}

string VRSky::skyVP =
#ifndef __EMSCRIPTEN__
"#version 400 compatibility\n"
#endif
GLSL(
\n
#ifdef __EMSCRIPTEN__
\n
varying vec3 norm;
varying vec4 pos;
varying vec2 tcs;
varying mat4 mFragInv;

attribute vec4 osg_Vertex;
attribute vec3 osg_Normal;
attribute vec2 osg_MultiTexCoord0;
uniform mat4 extInvMat;
\n
#else
\n
out vec3 norm;
out vec4 pos;
out vec2 tcs;
out mat4 mFragInv;

in vec4 osg_Vertex;
in vec3 osg_Normal;
in vec2 osg_MultiTexCoord0;
\n
#endif
\n

uniform int c1; // test input

void main() {
\n
#ifdef __EMSCRIPTEN__
\n
	mFragInv = extInvMat;
\n
#else
\n
	mFragInv = gl_ModelViewProjectionMatrix;
	mFragInv[3] = vec4(0,0,0,mFragInv[3][3]);
	mFragInv = inverse(mFragInv);
\n
#endif
\n

	pos = osg_Vertex * 0.5;
	pos.z = 0.5; // try to fix stereo
	gl_Position = osg_Vertex;
	norm = osg_Normal;
	tcs = osg_MultiTexCoord0;
}
);

string VRSky::skyFP =
#ifndef __EMSCRIPTEN__
"#version 400 compatibility\n"
#else
"#extension GL_EXT_frag_depth : enable\n"
#endif
GLSL(
\n
#ifdef __EMSCRIPTEN__
\n
precision mediump float;
\n
#endif
\n

\n
#ifndef __EMSCRIPTEN__
\n
in vec3 norm;
in vec4 pos;
in vec2 tcs;
in mat4 mFragInv;
\n
#else
\n
varying vec3 norm;
varying vec4 pos;
varying vec2 tcs;
varying mat4 mFragInv;
\n
#endif
\n

vec3 fragDir;
vec4 color;
uniform vec2 OSGViewportSize;

// sun
uniform vec3 sunPos; //define sun direction
uniform float theta_s;

// clouds
uniform sampler2D tex;
uniform vec2 cloudOffset; //to shift cloud texture
uniform vec4 cloudColor; //to shift cloud texture
uniform float cloudScale; // 1e-5
uniform float cloudDensity;
uniform float cloudHeight; // 1000.

//sky
uniform vec3 xyY_z;
uniform vec3 A;
uniform vec3 B;
uniform vec3 C;
uniform vec3 D;
uniform vec3 E;

float gamma;
float theta;

vec3 real_fragDir;
vec4 colGround = vec4(0.6, 0.6, 0.7, 1.0);
float rad_earth = 6.371e6;


void computeDirection() {\n
	real_fragDir = (mFragInv * pos).xyz;
	float tol = 1e-5;
	fragDir = real_fragDir;
	if(fragDir.y<tol) fragDir.y = tol;
	fragDir = normalize( fragDir );
}

vec3 xyYRGB(vec3 xyY) {\n
	float Y = xyY[2];
	float X = xyY[0] * Y / xyY[1];
	float Z = (1. - xyY[0] - xyY[1]) * Y / xyY[1];


	// scale XYZ from https://www.w3.org/Graphics/Color/srgb

	X = 0.0125313*(X - 0.1901);
	Y = 0.0125313*(Y - 0.2);
	Z = 0.0125313*(Z - 0.2178);
/*
	mat4 m_inv = mat4(3.2406255, -0.9689307, 0.0557101, 0.,
			-1.537208,  1.8757561,  -0.02040211, 0.,
			-0.4986286, 0.0415175,  1.0569959, 0.,
			0., 0., 0., 0.);
*/
	mat4 m_inv = mat4(3.06322, -0.96924, 0.06787, 0.,
			-1.39333,  1.87597,  -0.22883, 0.,
			-0.47580, 0.04156,  1.06925, 0.,
			0., 0., 0., 0.);

	vec4 rgb = m_inv * vec4(X, Y, Z, 0.);

	// add non-linearity
	for (int i=0; i<3; ++i) {
		clamp(rgb[i], 0.0, 1.0);
		if (rgb[i] > 0.0031308) {
			rgb[i] = 1.055 * pow(rgb[i], 1.0/2.4) - 0.055;
		} else {
			rgb[i] = 12.92 * rgb[i];
		}
	}

	return vec3(rgb[0], rgb[1], rgb[2]);
}



// returns the x-z coordinates on plane y = h
// at which the ray fragDir intersects
vec2 planeIntersect(float h) {\n
	vec2 pt = vec2(0.);
	// r = h / cos \phi
	pt = - h * fragDir.xz / fragDir.y ;
	return pt;
}

vec2 sphereIntersect(float h) {\n
	float r = rad_earth;
	float R = rad_earth + h;
	vec2 pt = vec2(0.);

	float b = -2.0*real_fragDir.y*r;
	float c = r*r-R*R;

	float disc = sqrt(b*b-4.0*c);
	float lambda = 0.0;
	lambda = max(b - disc, b + disc)*0.5;

	pt.x = lambda*real_fragDir.x;
	pt.y = lambda*real_fragDir.z;
	return pt;
}

float computeLuminanceOvercast() {\n
	// overcast luminance model (CIE - from preetham)
	// Y_z(1 + 2 cos \theta) / 3
	return xyY_z.z * (1.0 + 2.0 * fragDir.y) / 3.0;
}

float computeCloudLuminance() {\n
	float factor = 0.5;
	float offset = 0.4;
	float l = factor*(offset + (1.0 - offset)*computeLuminanceOvercast());
	return clamp(l, 0.0, 1.0);
}

void addCloudLayer(float height, vec2 offset, float density, float luminance) {\n
	vec2 uv = cloudScale * sphereIntersect(height) + offset;
\n
#ifdef __EMSCRIPTEN__
\n
	float cloud = texture2D(tex, uv).x;
	float noise = 0.9 + 0.1*texture2D(tex, uv * 4.0).x;
\n
#else
\n
	float cloud = texture(tex, uv).x;
	float noise = 0.9 + 0.1*texture(tex, uv * 4.0).x;
\n
#endif
\n

	cloud = smoothstep(0.0, 1.0, (1.0 - density) * cloud);
	vec3 c = mix(cloudColor.xyz, color.xyz, 1.0 - luminance);
	color = mix(vec4(noise*c, 1.0), color, cloud);
}

void computeClouds() {\n
	if (fragDir.y > 0.0 && cloudDensity > 0.0) {
		float density = cloudDensity;
		float scale = cloudScale;
		float y = computeCloudLuminance(); // compute luminance of clouds based on angle

		addCloudLayer(1.5*cloudHeight, 0.25*cloudOffset, 0.5*cloudDensity, y);
		addCloudLayer(    cloudHeight,      cloudOffset,     cloudDensity, 0.8*y);
	}
}

void addSun() {\n
	float sunAngSize = 0.00873; // defined sun size in radians
	float s = smoothstep( sunAngSize * 0.7, sunAngSize * 1.0, gamma);
	color = mix(vec4(1.0, 1.0, 1.0, 1.0), color, s);
}

void addGround() {\n
	float offset = 0.75;
	float factor = 0.3 * (offset + (1.0 - offset)*xyY_z.z);
	color = mix(factor * colGround, color, smoothstep( -0.05, 0.0, real_fragDir.y) );
}

vec3 f_f0() {\n
	// f  = (1 + A*exp(B/cos(theta)))
	//      * (1 + C*exp(D*gamma) + E*pow(cos(gamma),2))
	vec3 xyY = vec3(0.0,0.0,0.0);
	for (int i=0; i<3; ++i) {
		float f = (1.0 + A[i] * exp( B[i] / cos(theta) ) )
		    * (1.0 + C[i]*exp( D[i] * gamma ) + E[i] * pow( cos( gamma ), 2.0 ) );

		float f0 = (1.0 + A[i] * exp( B[i] ) )
		    * (1.0 + C[i] * exp( D[i] * theta_s ) + E[i] * pow( cos( theta_s ), 2.0) );
		if (f0 != 0.0) xyY[i] = f/f0;
	}
	return xyY;
}

void main() {\n

	computeDirection();

	// \gamma is angle between viewing direction (fragDir) and sun (sunPos)
	gamma = acos(dot(fragDir, sunPos));
	// \theta is angle of fragDir to zenith
	theta = acos(fragDir.y);

	vec3 xyY = f_f0();
	//xyY.x = xyY_z.x;
	//xyY.y = xyY_z.y;
	xyY.x *= xyY_z.x;
	xyY.y *= xyY_z.y;
	xyY.z *= xyY_z.z;

	//vec3 xyY = vec3(0.150017, 0.060007, 7.22);
	color = vec4(xyYRGB(xyY), 1.0);

	color.rgb *= 1.0; // hack
	color.b *= 1.1;
	color.r *= 0.85;

	// add cloud cover
	computeClouds();

	// add sun
	addSun();
	addGround();

    color[3] = 1.0;
	gl_FragColor = color;
\n
#ifdef __EMSCRIPTEN__
\n
	gl_FragDepthEXT = 1.0; // depth is infinite at 1.0? behind all else (check)
\n
#else
\n
	gl_FragDepth = 1.0; // depth is infinite at 1.0? behind all else (check)
\n
#endif
\n

	// vertical line for testing
	//if (real_fragDir.x < 0.01 && real_fragDir.x > 0) gl_FragColor = vec4(0,0,0,1);
}
);

string VRSky::skyDFP =
GLSL(
bla
);


