#include "VRNavigator.h"

#include <OpenSG/OSGIntersectAction.h>
#include "core/objects/VRTransform.h"
#include "core/utils/VRFunction.h"
#include "core/setup/devices/VRSignal.h"
#include "core/setup/devices/VRDevice.h"
#include "core/setup/devices/VRMouse.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

#define sign(x) (( x >> 31 ) | ( (unsigned int)( -x ) >> 31 ))

string VRN_toString(int i) { char buf[100]; sprintf(buf,"%d", i); return string(buf); }

VRNavBinding::VRNavBinding(VRDeviceCb c, int k, int s, bool repeat) {
    cb = c;
    key = k;
    state = s;
    doRepeat = repeat;
}

VRNavBinding::~VRNavBinding() {
    clearSignal();
}

void VRNavBinding::clearSignal() { if (auto sigp = sig.lock()) sigp->sub(cb); }

VRNavPreset::VRNavPreset() {
    setNameSpace("NavPreset");
    setName("preset");
}

VRNavPreset::~VRNavPreset() {}

void VRNavPreset::updateBinding(VRNavBinding& b) {
    if (!active) return;
    if (dev == 0) return;
    b.clearSignal();
    auto sig = b.doRepeat ? dev->addSignal(b.key) : dev->addSignal( b.key, b.state);
    sig->add(b.cb);
    b.sig = sig;
    //cout << "\nUPDATE BINDING " << b.cb->getName() << endl;
}

void VRNavPreset::setDevice(VRDevice* _dev) {
    dev = _dev;
    dev->setTarget(target);

    for (uint i=0; i<bindings.size(); i++) updateBinding(bindings[i]);
}

void VRNavPreset::setTarget(VRTransformPtr _target) { target = _target; if (dev) dev->setTarget(target); }

void VRNavPreset::activate() {
    active = true;
    if (dev) dev->setSpeed(speed);
    for (uint i=0; i<bindings.size(); i++) updateBinding(bindings[i]);
}

void VRNavPreset::deactivate() {
    active = false;
    for (uint i=0; i<bindings.size(); i++) bindings[i].clearSignal();
}

vector<VRNavBinding>& VRNavPreset::getBindings() { return bindings; }

void VRNavPreset::addKeyBinding(VRNavBinding b) {
    bindings.push_back(b);
    updateBinding(b);
}

void VRNavPreset::setSpeed(float vt, float vr) { speed = Vec2f(vt, vr); }

// preset management

VRNavigator_base::VRNavigator_base () {
    current = 0;

    store("active", &current_name);

    VRNavPreset* np = new VRNavPreset();
    np->setName("None");
    addNavigation(np);
}

VRNavigator_base::~VRNavigator_base() {
    for (auto p : presets) delete p.second;
    presets.clear();
}

void VRNavigator_base::addNavigation(VRNavPreset* ps) {
    string name = ps->getName();
    if (presets.count(name)) delete presets[name];
    presets[name] = ps;
}

void VRNavigator_base::remNavigation(string name) {
    delete presets[name];
    presets.erase(name);
}

void VRNavigator_base::setActiveNavigation(string s) {
    if (s == "") return;
    if (presets.count(s) == 0) return;
    if (current) current->deactivate();
    current = presets[s];
    current->activate();
    current_name = s;
}

VRNavPreset* VRNavigator_base::getNavigation(string s) {
    if (presets.count(s) == 0) return 0;
    return presets[s];
}

string VRNavigator_base::getActiveNavigation() { return current_name; }
vector<string> VRNavigator_base::getNavigationNames() { vector<string> res; for(auto p : presets) res.push_back(p.first); return res; }
map<string, VRNavPreset*> VRNavigator_base::getNavigations() { return presets; }

void VRNavigator_base::storeNavigationCallback(VRDeviceCb cb) { library[cb->getName()] = cb; }
VRDeviceCb VRNavigator_base::getNavigationCallback(string s) { if (library.count(s)) return library[s]; return 0; }
map<string, VRDeviceCb>& VRNavigator_base::getNavigationCallbacks() { return library; }


VRNavigator::VRNavigator() {
    auto addNavCb = [&](string name, boost::function<void(VRDevice*)> fkt) {
        storeNavigationCallback( VRDeviceCb( new VRFunction<VRDevice*>(name, fkt) ) );
    };

    addNavCb("mouse_orbit2d", boost::bind(&VRNavigator::orbit2D, this, _1));
    addNavCb("mouse_orbit2d", boost::bind(&VRNavigator::orbit2D, this, _1) );
    addNavCb("mouse_orbit", boost::bind(&VRNavigator::orbit, this, _1) );
    addNavCb("mouse_zoom_in", boost::bind(&VRNavigator::zoom, this, _1, 1) );
    addNavCb("mouse_zoom_out", boost::bind(&VRNavigator::zoom, this, _1, -1) );
    addNavCb("mouse_focus", boost::bind(&VRNavigator::focus, this, _1) );
    addNavCb("mouse_walk", boost::bind(&VRNavigator::walk, this, _1) );
    addNavCb("fly_walk", boost::bind(&VRNavigator::fly_walk, this, _1) );
    addNavCb("hyd_walk", boost::bind(&VRNavigator::hyd_walk, this, _1) );
}

VRNavigator::~VRNavigator() {}

// callbacks

float VRNavigator::clip_dist_down = 1.5;

void VRNavigator::zoom(VRDevice* dev, int dir) {
    VRTransformPtr target = dev->getTarget();
    if (target == 0) return;

    //Vec2f speed = dev->getSpeed(); // 0.05
    float speed = 0.05;
    target->zoom(speed*dir);
}

void VRNavigator::orbit(VRDevice* dev) {
    VRTransformPtr target = dev->getTarget();
    VRTransformPtr devBeacon = dev->getBeacon();

    if (target == 0) return;
    if (devBeacon == 0) return;

    static int state = 0;
    static Vec3f camPos;
    static Vec3f camRef(0,0,0);
    static Vec2f mouseOnMouseDown;

    Vec3f camDelta;
    Vec2f mousePos;
    mousePos[0] = dev->s_state(5);
    mousePos[1] = dev->s_state(6);

    if (state == 0) { // drag camera
        camPos = target->getFrom();
        mouseOnMouseDown = mousePos;

        Vec3f dir = -target->getDir();
        camRef[0] = dir.length();
        dir.normalize();

        camRef[2] = asin(dir[1]);

        float cosb = cos(camRef[2]);
        float a = acos(dir[0]/cosb);
        if (dir[2]/cosb<0) a *= -1;
        camRef[1] = a;
    }

    // move cam
    mousePos -= mouseOnMouseDown;
    camDelta = camRef;
    camDelta[1] += mousePos[0]*1.5; //yaw
    camDelta[2] -= mousePos[1]*1.5; //pitch

    camDelta[2] = max(camDelta[2], -Pi*0.49f);
    camDelta[2] = min(camDelta[2], Pi*0.49f);

    float cosa = cos(camDelta[1]);
	float sina = sin(camDelta[1]);
	float cosb = cos(camDelta[2]);
	float sinb = sin(camDelta[2]);

	camPos = Vec3f(cosa*cosb, sinb, sina*cosb)*camDelta[0] + target->getAt();
	target->set_orientation_mode(false);
	target->setFrom(camPos);

    state = dev->b_state(dev->key());
}

void VRNavigator::walk(VRDevice* dev) {
    VRTransformPtr target = dev->getTarget();
    VRTransformPtr devBeacon = dev->getBeacon();

    if (target == 0) return;
    if (devBeacon == 0) return;

    Vec3f dir_c = target->getWorldDirection();
    Vec3f dir_m = devBeacon->getAt() - devBeacon->getFrom();
    dir_m.normalize();
    dir_c.normalize();

    Vec2f speed = dev->getSpeed();
    float speedT = speed[0]; // 0.02
    float speedR = speed[1]; // 0.04

    Vec3f x = -dir_c*dir_m[1]*speedT*exp(-abs(dir_m[0]));
    float y = -dir_m[0]*speedR*exp(-abs(dir_m[1]));

    target->rotate(y);
    target->translate(x);
}

void VRNavigator::fly_walk(VRDevice* dev) {
    int key = dev->key();
    float d = 0;
    dev->s_state(key, &d);
    float d_abs = abs(d);

    VRTransformPtr target = dev->getTarget();
    VRTransformPtr flystick = dev->getBeacon();
    if (target == 0 || flystick == 0) return;

    Vec3f dir = flystick->getWorldDirection();
    dir.normalize();

    Vec2f speed = dev->getSpeed();
    float tspeed = speed[0]; // 1.5
    float rspeed = speed[1]; // 0.7

    float dt = 60.0/ max(1u, VRGlobals::get()->FRAME_RATE);
    switch(key) {
        case 10:
            target->rotate(-dt*d*d_abs*0.1*rspeed);
            break;
        case 11:
            target->translate(-dir*dt*d*d_abs*0.2*tspeed);
            break;
    }
}

void VRNavigator::orbit2D(VRDevice* dev) {
    VRTransformPtr target = dev->getTarget();
    VRTransformPtr devBeacon = dev->getBeacon();

    if (target == 0) return;
    if (devBeacon == 0) return;

    Vec3f dir_m = devBeacon->getAt() - devBeacon->getFrom();
    dir_m.normalize();

    float speedT = 0.02;
    float speedR = 0.04;

    float x = dir_m[1]*speedT*exp(-abs(dir_m[0]));
    float y = -dir_m[0]*speedR*exp(-abs(dir_m[1]));

    target->rotateAround(y);
    target->zoom(x);
}

void VRNavigator::focus(VRDevice* dev) {
    VRTransformPtr target = dev->getTarget();
    VRTransformPtr devBeacon = dev->getBeacon();

    if (target == 0) return;
    if (devBeacon == 0) return;

    VRIntersection ins = dev->intersect(target->getRoot());

    if (!ins.hit) return;
    target->setAt( ins.point.subZero() );
}

// presets

void VRNavigator::initWalk(VRTransformPtr target, VRDevice* dev) {
    VRNavPreset* preset = new VRNavPreset();
    preset->setDevice(dev);
    preset->setTarget(target);
    preset->setSpeed(0.02, 0.04);

    VRNavBinding b1( getNavigationCallback("mouse_walk"), 0, 0, true);
    preset->addKeyBinding(b1);
    preset->deactivate();
    preset->setName("Walk");

    addNavigation(preset);
}

void VRNavigator::initOrbit(VRTransformPtr target, VRDevice* dev) {
    VRNavPreset* preset = new VRNavPreset();
    preset->setDevice(dev);
    preset->setTarget(target);

    VRNavBinding b( getNavigationCallback("mouse_orbit"), 1, 0, true);
    VRNavBinding bz1( getNavigationCallback("mouse_zoom_in"), 4, 1, false);
    VRNavBinding bz2( getNavigationCallback("mouse_zoom_out"), 3, 1, false);
    VRNavBinding bf( getNavigationCallback("mouse_focus"), 2, 0, false);

    preset->addKeyBinding(b);
    preset->addKeyBinding(bz1);
    preset->addKeyBinding(bz2);
    preset->addKeyBinding(bf);
    preset->deactivate();
    preset->setName("Orbit");

    addNavigation(preset);
}

void VRNavigator::initOrbit2D(VRTransformPtr target, VRDevice* dev) {
    VRNavPreset* preset = new VRNavPreset();
    preset->setDevice(dev);
    preset->setTarget(target);

    VRNavBinding b( getNavigationCallback("mouse_orbit2d"), 0, 0, true);
    VRNavBinding bz1( getNavigationCallback("mouse_zoom_in"), 4, 1, false);
    VRNavBinding bz2( getNavigationCallback("mouse_zoom_out"), 3, 1, false);
    VRNavBinding bf( getNavigationCallback("mouse_focus"), 2, 0, false);

    preset->addKeyBinding(b);
    preset->addKeyBinding(bz1);
    preset->addKeyBinding(bz2);
    preset->addKeyBinding(bf);
    preset->deactivate();
    preset->setName("Orbit2D");

    addNavigation(preset);
}

void VRNavigator::initFlyOrbit(VRTransformPtr target, VRDevice* dev) { // TODO
    VRNavPreset* preset = new VRNavPreset();
    preset->setDevice(dev);
    preset->setTarget(target);

    VRNavBinding b( getNavigationCallback("mouse_orbit"), 0, 0, true);
    VRNavBinding bz1( getNavigationCallback("mouse_zoom_in"), 4, 1, false);
    VRNavBinding bz2( getNavigationCallback("mouse_zoom_out"), 3, 1, false);

    preset->addKeyBinding(b);
    preset->addKeyBinding(bz1);
    preset->addKeyBinding(bz2);
    preset->deactivate();
    preset->setName("FlyOrbit2D");

    addNavigation(preset);
}

void VRNavigator::initFlyWalk(VRTransformPtr target, VRDevice* dev) {
    VRNavPreset* preset = new VRNavPreset();
    preset->setDevice(dev);
    preset->setTarget(target);
    preset->setSpeed(1.5, 0.7);

    VRNavBinding b1( getNavigationCallback("fly_walk"), 10, 0, false);
    VRNavBinding b2( getNavigationCallback("fly_walk"), 11, 0, false);
    preset->addKeyBinding(b1);
    preset->addKeyBinding(b2);
    preset->deactivate();
    preset->setName("FlyWalk");

    addNavigation(preset);
}

void VRNavigator::hyd_walk(VRDevice* dev) {
    int key = dev->key();
    float d = 0;
    dev->s_state(key, &d);
    float d_abs = abs(d);

    VRTransformPtr target = dev->getTarget();
    VRTransformPtr flystick = dev->getBeacon();
    if (target == 0 || flystick == 0) return;

    Vec3f dir = flystick->getWorldDirection();
    dir.normalize();

    float rspeed = 0.1;
    float tspeed = 0.1;

    float dt = 60.0/ max(1u, VRGlobals::get()->FRAME_RATE);
    switch(key) {
        case 20:
        case 23:
            target->rotate(-dt*d*d_abs*0.1*rspeed);
            break;
        case 21:
        case 24:
            target->translate(-dir*dt*d*d_abs*0.2*tspeed);
            break;
    }
}

void VRNavigator::initHydraFly(VRTransformPtr target, VRDevice* dev) {
    VRNavPreset* preset = new VRNavPreset();
    preset->setDevice(dev);
    preset->setTarget(target);

    VRNavBinding b1( getNavigationCallback("hyd_walk"), 20, 0, false);
    VRNavBinding b2( getNavigationCallback("hyd_walk"), 21, 0, false);
    VRNavBinding b3( getNavigationCallback("hyd_walk"), 23, 0, false);
    VRNavBinding b4( getNavigationCallback("hyd_walk"), 24, 0, false);

    preset->addKeyBinding(b1);
    preset->addKeyBinding(b2);
    preset->addKeyBinding(b3);
    preset->addKeyBinding(b4);
    preset->deactivate();
    preset->setName("Hydra");

    addNavigation(preset);
}

void VRNavigator::update() {
    setActiveNavigation(getActiveNavigation());
}


OSG_END_NAMESPACE;
