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

VRNavBinding::VRNavBinding(VRDevCb* c, int k, int s, bool repeat) {
    cb = c;
    key = k;
    state = s;
    doRepeat = repeat;
    sig = 0;
}



VRNavPreset::VRNavPreset() {
    dev = 0;
    target = 0;
    active = false;
}

VRNavPreset::~VRNavPreset() {
    for (uint i=0; i<bindings.size(); i++) {
        VRNavBinding b = bindings[i];
        if (b.sig) b.sig->sub(b.cb);
    }
    bindings.clear();
}

void VRNavPreset::updateBinding(VRNavBinding& b) {
    if (!active) return;
    if (dev == 0) return;
    if (b.sig) b.sig->sub(b.cb);
    if (b.doRepeat) b.sig = dev->addSignal(b.key);
    else b.sig = dev->addSignal( b.key, b.state);
    b.sig->add(b.cb);
    //cout << "\nUPDATE BINDING " << b.cb->getName() << endl;
}

void VRNavPreset::setDevice(VRDevice* _dev) {
    dev = _dev;
    dev->setTarget(target);

    for (uint i=0; i<bindings.size(); i++) updateBinding(bindings[i]);
}

void VRNavPreset::setTarget(VRTransform* _target) { target = _target; if (dev) dev->setTarget(target); }

void VRNavPreset::activate() {
    active = true;
    for (uint i=0; i<bindings.size(); i++) updateBinding(bindings[i]);
}

void VRNavPreset::deactivate() {
    active = false;
    for (uint i=0; i<bindings.size(); i++) {
        if (bindings[i].sig) bindings[i].sig->sub(bindings[i].cb);
    }
}

vector<VRNavBinding>& VRNavPreset::getBindings() { return bindings; }

void VRNavPreset::addKeyBinding(VRNavBinding b) {
    bindings.push_back(b);
    updateBinding(b);
}

// preset management

VRNavigator_base::VRNavigator_base () {
    current = 0;
}

void VRNavigator_base::addPreset(VRNavPreset* ps, string& name) {
    string tmp = name; //make unique name
    int i=2;
    while (presets.count(tmp) == 1) { tmp = name + VRN_toString(i); i++; }
    name = tmp;

    presets[name] = ps;
}

void VRNavigator_base::remPreset(string name) {
    delete presets[name];
    presets.erase(name);
}

void VRNavigator_base::setActivePreset(string s) {
    if (s == "") return;
    if (presets.count(s) == 0) return;
    if (current) current->deactivate();
    current = presets[s];
    current->activate();
}

VRNavPreset* VRNavigator_base::getPreset(string s) {
    if (presets.count(s) == 0) return 0;
    return presets[s];
}

map<string, VRNavPreset*> VRNavigator_base::getPresets() { return presets; }

void VRNavigator_base::storeCallback(VRDevCb* cb) { library[cb->getName()] = cb; }

map<string, VRDevCb*>& VRNavigator_base::getCallbacks() { return library; }

// callbacks

float VRNavigator::clip_dist_down = 1.5;

void VRNavigator::zoom(VRDevice* dev, int dir) {
    VRTransform* target = dev->getTarget();
    if (target == 0) return;

    float speed = 0.05;
    target->zoom(speed*dir);
}

void VRNavigator::orbit(VRDevice* dev) {
    VRTransform* target = dev->getTarget();
    VRTransform* devBeacon = dev->getBeacon();

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
    VRTransform* target = dev->getTarget();
    VRTransform* devBeacon = dev->getBeacon();

    if (target == 0) return;
    if (devBeacon == 0) return;

    Vec3f dir_c = target->getWorldDirection();
    Vec3f dir_m = devBeacon->getAt() - devBeacon->getFrom();
    dir_m.normalize();
    dir_c.normalize();


    float speedT = 0.02;
    float speedR = 0.04;

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

    VRTransform* target = dev->getTarget();
    VRTransform* flystick = dev->getBeacon();
    if (target == 0 or flystick == 0) return;

    Vec3f dir = flystick->getWorldDirection();
    dir.normalize();

    float rspeed = 0.7;
    float tspeed = 1.5;

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
    VRTransform* target = dev->getTarget();
    VRTransform* devBeacon = dev->getBeacon();

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
    VRTransform* target = dev->getTarget();
    VRTransform* devBeacon = dev->getBeacon();

    if (target == 0) return;
    if (devBeacon == 0) return;

    VRIntersection ins = dev->intersect(target->getRoot());

    if (!ins.hit) return;
    if (ins.object == 0) return;
    Vec3f at = ins.point.subZero();
    target->setAt(at);
}

// presets

void VRNavigator::initWalk(VRTransform* target, VRDevice* dev) {
    VRNavPreset* preset = new VRNavPreset();
    preset->setDevice(dev);
    preset->setTarget(target);

    VRDevCb* cb = new VRDevCb( "mouse_walk", boost::bind(&VRNavigator::walk, this, _1) );
    storeCallback(cb);

    VRNavBinding b1(cb, 0, 0, true);
    preset->addKeyBinding(b1);
    preset->deactivate();

    string name = "Walk";
    addPreset(preset, name);
}

void VRNavigator::initOrbit(VRTransform* target, VRDevice* dev) {
    VRNavPreset* preset = new VRNavPreset();
    preset->setDevice(dev);
    preset->setTarget(target);

    VRDevCb* cb = new VRDevCb( "mouse_orbit", boost::bind(&VRNavigator::orbit, this, _1) );
    VRDevCb* cb_zoom1 = new VRDevCb( "mouse_zoom", boost::bind(&VRNavigator::zoom, this, _1, 1) );
    VRDevCb* cb_zoom2 = new VRDevCb( "mouse_zoom2", boost::bind(&VRNavigator::zoom, this, _1, -1) );
    VRDevCb* cb_focus = new VRDevCb( "mouse_focus", boost::bind(&VRNavigator::focus, this, _1) );
    storeCallback(cb);
    storeCallback(cb_zoom1);
    storeCallback(cb_zoom2);
    storeCallback(cb_focus);

    VRNavBinding b(cb, 1, 0, true);
    VRNavBinding bz1(cb_zoom1, 4, 1, false);
    VRNavBinding bz2(cb_zoom2, 3, 1, false);
    VRNavBinding bf(cb_focus, 2, 0, false);

    preset->addKeyBinding(b);
    preset->addKeyBinding(bz1);
    preset->addKeyBinding(bz2);
    preset->addKeyBinding(bf);
    preset->deactivate();

    string name = "Orbit";
    addPreset(preset, name);
}

void VRNavigator::initOrbit2D(VRTransform* target, VRDevice* dev) {
    VRNavPreset* preset = new VRNavPreset();
    preset->setDevice(dev);
    preset->setTarget(target);

    VRDevCb* cb = new VRDevCb( "mouse_orbit2d", boost::bind(&VRNavigator::orbit2D, this, _1) );
    VRDevCb* cb_zoom1 = new VRDevCb( "mouse_zoom", boost::bind(&VRNavigator::zoom, this, _1, 1) );
    VRDevCb* cb_zoom2 = new VRDevCb( "mouse_zoom2", boost::bind(&VRNavigator::zoom, this, _1, -1) );
    storeCallback(cb);
    storeCallback(cb_zoom1);
    storeCallback(cb_zoom2);

    VRNavBinding b(cb, 0, 0, true);
    VRNavBinding bz1(cb_zoom1, 4, 1, false);
    VRNavBinding bz2(cb_zoom2, 3, 1, false);

    preset->addKeyBinding(b);
    preset->addKeyBinding(bz1);
    preset->addKeyBinding(bz2);
    preset->deactivate();

    string name = "Orbit2D";
    addPreset(preset, name);
}

void VRNavigator::initFlyOrbit(VRTransform* target, VRDevice* dev) { // TODO
    VRNavPreset* preset = new VRNavPreset();
    preset->setDevice(dev);
    preset->setTarget(target);

    VRDevCb* cb = new VRDevCb( "fly_orbit2d", boost::bind(&VRNavigator::orbit, this, _1) );
    VRDevCb* cb_zoom1 = new VRDevCb( "fly_zoom", boost::bind(&VRNavigator::zoom, this, _1, 1) );
    VRDevCb* cb_zoom2 = new VRDevCb( "fly_zoom2", boost::bind(&VRNavigator::zoom, this, _1, -1) );
    storeCallback(cb);
    storeCallback(cb_zoom1);
    storeCallback(cb_zoom2);

    VRNavBinding b(cb, 0, 0, true);
    VRNavBinding bz1(cb_zoom1, 4, 1, false);
    VRNavBinding bz2(cb_zoom2, 3, 1, false);

    preset->addKeyBinding(b);
    preset->addKeyBinding(bz1);
    preset->addKeyBinding(bz2);
    preset->deactivate();

    string name = "FlyOrbit2D";
    addPreset(preset, name);
}

void VRNavigator::initFlyWalk(VRTransform* cam, VRDevice* dev) {
    VRNavPreset* preset = new VRNavPreset();
    preset->setDevice(dev);
    preset->setTarget(target);

    VRDevCb* cb = new VRDevCb( "fly_walk", boost::bind(&VRNavigator::fly_walk, this, _1) );
    storeCallback(cb);

    VRNavBinding b1(cb, 10, 0, false);
    VRNavBinding b2(cb, 11, 0, false);
    preset->addKeyBinding(b1);
    preset->addKeyBinding(b2);
    preset->deactivate();

    string name = "FlyWalk";
    addPreset(preset, name);
}

/*void VRNavigator::initFlyWalk(VRTransform* target, VRDevice* dev) { // TODO
    VRNavPreset* preset = new VRNavPreset();
    preset->setDevice(dev);
    preset->setTarget(target);

    VRDevCb* cb = new VRDevCb( "fly_orbit2d", boost::bind(&VRNavigator::orbit2D, this, _1) );
    VRDevCb* cb_zoom1 = new VRDevCb( "fly_zoom", boost::bind(&VRNavigator::zoom, this, _1, 1) );
    VRDevCb* cb_zoom2 = new VRDevCb( "fly_zoom2", boost::bind(&VRNavigator::zoom, this, _1, -1) );
    storeCallback(cb);
    storeCallback(cb_zoom1);
    storeCallback(cb_zoom2);

    VRNavBinding b(cb, 0, 0, true);
    VRNavBinding bz1(cb_zoom1, 4, 1, false);
    VRNavBinding bz2(cb_zoom2, 3, 1, false);

    preset->addKeyBinding(b);
    preset->addKeyBinding(bz1);
    preset->addKeyBinding(bz2);
    preset->deactivate();

    string name = "FlyOrbit2D";
    addPreset(preset, name);
}*/


VRNavigator::VRNavigator() {}


OSG_END_NAMESPACE;
