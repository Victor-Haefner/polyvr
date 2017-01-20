#include "VRDriver.h"
#include "CarDynamics.h"
#include "core/scene/VRScene.h"
#include "core/objects/VRTransform.h"
#include "core/math/pose.h"
#include "core/math/path.h"
#include <boost/bind.hpp>

using namespace OSG;

VRDriver::VRDriver() {
    auto scene = VRScene::getCurrent();
    updatePtr = VRFunction<int>::create("driver_update", boost::bind(&VRDriver::update, this));
    scene->addUpdateFkt(updatePtr);
}

VRDriver::~VRDriver() {}

VRDriverPtr VRDriver::create() { return VRDriverPtr( new VRDriver() ); }

void VRDriver::setCar(CarDynamicsPtr c) { car = c; }

void VRDriver::update() {
    if (!active) return;
    if (!p_path || !v_path || !car) return;

    auto clamp = [&](float& v, float a, float b) {
        if (v < a) v = a;
        if (v > b) v = b;
    };

    auto cpose = car->getChassis()->getPose();
    auto pos = cpose->pos();
    auto dir = cpose->dir();
    auto up = cpose->up();
    auto speed = car->getSpeed();

    float t = p_path->getClosestPoint( pos ); // get closest path point
    float L = p_path->getLength();
    float aimingLength = 4.0;//2.0*speed;

    Vec3f p0 = p_path->getPose(t).pos();
    t += aimingLength/L; // aim some meter ahead
    clamp(t,0,1);

    auto tpos = p_path->getPose(t).pos(); // get target position
    //auto tvel = v_path->getPose(t).pos()[1]; // get target velocity

    float target_speed = 5; // TODO: get from path

    // compute throttle and breaking
    float sDiff = target_speed-speed;
    float throttle = 0;
    float breaking = 0;
    if (sDiff > 0) throttle = sDiff*0.7;
    if (sDiff < 0) breaking = -sDiff*0.2;
    //cout << "pilot " << sDiff << " " << throttle << " " << breaking << endl;


    // compute steering
    Vec3f delta = tpos - pos;
    delta.normalize();
    dir.normalize();
    up.normalize();
    Vec3f w = delta.cross(dir);
    float steering = w.dot(up)*3.0;

    // clamp inputs
    clamp(throttle, 0,1);
    clamp(breaking, 0,1);
    clamp(steering, -1,1);

    // apply inputs
    car->setGear(1);
    car->setThrottle(throttle);
    car->setBreak(breaking);
    car->setSteering(steering);
}

void VRDriver::followPath(pathPtr p, pathPtr v) {
    p_path = p;
    v_path = v;
    active = true;
}

void VRDriver::stop() { active = false; }
bool VRDriver::isDriving() { return active; }




