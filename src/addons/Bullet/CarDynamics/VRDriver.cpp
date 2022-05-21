#include "VRDriver.h"
#include "CarDynamics.h"
#include "core/scene/VRScene.h"
#include "core/objects/VRTransform.h"
#include "core/math/pose.h"
#include "core/math/path.h"

using namespace OSG;

VRDriver::VRDriver() {
    auto scene = VRScene::getCurrent();
    updatePtr = VRUpdateCb::create("driver_update", bind(&VRDriver::update, this));
    scene->addUpdateFkt(updatePtr);
}

VRDriver::~VRDriver() {}

VRDriverPtr VRDriver::create() { return VRDriverPtr( new VRDriver() ); }

void VRDriver::setCar(VRCarDynamicsPtr c) { car = c; }

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

    float L = p_path->getLength();
    float aimingLength = 4.0;//2.0*speed;
    float tAim = aimingLength/L;

    float tPath = p_path->getClosestPoint( pos ); // get closest path point
    //cout << " tPath: " << tPath << ", lastPathT: " << lastPathT << endl;
    if (lastPathT > 0) {
        if (tPath > lastPathT + tAim*3) tPath = lastPathT;
        if (tPath < lastPathT - tAim*3) tPath = lastPathT;
    }
    lastPathT2 = lastPathT;
    lastPathT = tPath;


    if (tPath >= to && p_path->getDistance( pos ) < 1) {
        car->update(0, 0.2, 0);
        return;
    }

    Vec3d p0 = p_path->getPose(tPath)->pos();
    float t = tPath + tAim; // aim some meter ahead
    clamp(t,0,1);

    auto tpos = p_path->getPose(t)->pos(); // get target position
    //auto tvel = v_path->getPose(t).pos()[1]; // TODO: get target velocity

    // compute throttle, breaking and clutch
    float sDiff = target_speed-speed;
    float throttle = 0;
    float breaking = 0;
    float clutch = 0;
    if (sDiff > 0) {
        float c = sDiff*0.2;
        clamp(c, 0, 1);
        clutch = 0.6*c;

        throttle = sDiff*0.7;
        if (useClutch) clamp(throttle, 0, 1.0-clutch*0.6);
    }
    if (sDiff < 0) breaking = -sDiff*0.2;


    // compute steering
    Vec3d delta = tpos - pos;
    delta.normalize();
    dir.normalize();
    up.normalize();
    Vec3d w = delta.cross(dir);
    float steering = w.dot(up)*3.0;

    // clamp inputs
    clamp(throttle, 0,1);
    clamp(breaking, 0,1);
    clamp(clutch,   0,1);
    clamp(steering, -1,1);
    if (!useClutch) clutch = 0;

    //throttle = 0.9;
    //cout << "pilot sDiff: " << sDiff << " throttle: " << throttle << " breaking: " << breaking << " clutch: " << clutch << " steering: " << steering << endl;

    // apply inputs
    if (!car->isRunning()) car->setIgnition(true);
    car->update(throttle, breaking, steering, clutch);
}

void VRDriver::followPath(PathPtr p, PathPtr v, float to) {
    p_path = p;
    v_path = v;
    this->to = to;
    active = true;
}

void VRDriver::setTargetSpeed( float speed, bool uCl ) { target_speed = speed; useClutch = uCl; }
void VRDriver::stop() { active = false; }
void VRDriver::resume() { active = true; }
bool VRDriver::isDriving() { return active; }




