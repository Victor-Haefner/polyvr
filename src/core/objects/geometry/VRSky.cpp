#include "VRSky.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRTextureGenerator.h"
#include "core/utils/VRFunction.h"
#include "core/scene/VRScene.h"

#include <boost/bind.hpp>
#include <time.h>

using namespace OSG;

VRSky::VRSky() : VRGeometry("Sky") {
    type = "Sky";
    string resDir = VRSceneManager::get()->getOriginalWorkdir() + "/shader/Sky/";
    string vScript = resDir + "Sky.vp";
    string fScript = resDir + "Sky.fp";
    auto m = VRMaterial::create("Sky");
    m->readVertexShader(vScript);
    m->readFragmentShader(fScript);
    setMaterial(m);
    setPrimitive("Plane", "2 2 1 1");
    m->setLit(false);
	m->setDiffuse(OSG::Vec3f(1.,1.,1.));

	VRTextureGenerator tg;
	tg.add(PERLIN, 1, Vec3f(0.9), Vec3f(1.0));
	tg.add(PERLIN, 1.0/2, Vec3f(0.7), Vec3f(1.0));
	tg.add(PERLIN, 1.0/4, Vec3f(0.6), Vec3f(1.0));
	tg.add(PERLIN, 1.0/8, Vec3f(0.5), Vec3f(1.0));
	tg.add(PERLIN, 1.0/16, Vec3f(0.4), Vec3f(1.0));

	m->setTexture(tg.compose(0));

    allowCulling(false, true);

    updatePtr = VRFunction<int>::create("sky update", boost::bind(&VRSky::update, this));
    VRScene::getCurrent()->addUpdateFkt(updatePtr);
}

VRSky::~VRSky() {}

VRSkyPtr VRSky::create() { return VRSkyPtr( new VRSky() ); }
VRSkyPtr VRSky::ptr() { return static_pointer_cast<VRSky>( shared_from_this() ); }

void VRSky::update() {
    if (!isVisible()) return;


    // update clouds movement

    // update

    //auto m = getMaterial();
    //m->setShaderParameter<Vec3f>("sunPos", Vec3f(1,0,0));

    //auto t = time(0);
}

void VRSky::setTime() {

}

Vec3f VRSky::sunFromTime() {
	// lets set the lattitude and longitude
	// Karlsruhe 49.0069° N, 8.4037° E
	// http://aa.usno.navy.mil/faq/docs/SunApprox.php

	// and time of day

	// ...and time of year...

	// ...and year - lets assume 1. jan 2000 12:00 GMT (=&gt; n=0) then
	//D = JD - 2451545.0
	float day = 0.;

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

	float RA = atan2(cos(e) * sin(L) / cos(L)); // right ascension
	float d = asin(sin(e) * sin(L)); // declination

    Vec3f dir = Vec3f(1., 0., 0.);
	return dir;
}
