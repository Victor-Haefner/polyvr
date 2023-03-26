#include "VRNavigator.h"

#include <OpenSG/OSGIntersectAction.h>
#include "core/objects/VRTransform.h"
#include "core/utils/VRFunctionFwd.h"
#include "core/utils/VRFunction.h"
#include "core/utils/toString.h"
#include "core/utils/VRGlobals.h"
#include "core/utils/VRStorage_template.h"
#include "core/setup/VRSetup.h"
#include "core/setup/devices/VRSignal.h"
#include "core/setup/devices/VRDevice.h"
#include "core/setup/devices/VRMouse.h"
#include "core/scene/VRScene.h"
#include "core/math/path.h"


using namespace OSG;


#define sign(x) (( x >> 31 ) | ( (unsigned int)( -x ) >> 31 ))

string VRN_toString(int i) { char buf[100]; sprintf(buf,"%d", i); return string(buf); }

VRNavBinding::VRNavBinding(VRDeviceCbPtr c, int k, int s, bool repeat) {
    cb = c;
    key = k;
    state = s;
    doRepeat = repeat;
}

VRNavBinding::~VRNavBinding() {
    clearSignal();
}

void VRNavBinding::clearSignal() {
    if (auto sigp = sig.lock()) {
        sigp->sub(cb);
    }
}

VRNavPreset::VRNavPreset() {
    setNameSpace("NavPreset");
    setName("preset");
}

VRNavPreset::~VRNavPreset() {}
shared_ptr<VRNavPreset> VRNavPreset::create() { return shared_ptr<VRNavPreset>(new VRNavPreset()); }

void VRNavPreset::updateBinding(VRNavBinding& b) {
    if (!active || !b.active) return;
    if (dev == 0) return;

    b.clearSignal();
    auto sig = b.doRepeat ? dev->addToggleSignal(b.key) : dev->newSignal( b.key, b.state);
    sig->add(b.cb);
    b.sig = sig;
    //cout << "\nUPDATE BINDING " << b.key << " " << b.state << " " << dev->getName() << endl;
}

void VRNavPreset::setDevice(VRDevicePtr _dev) {
    dev = _dev;
    dev->setTarget(target.lock());
    for (auto& b : bindings) updateBinding(b);
}

void VRNavPreset::setTarget(VRTransformPtr _target) { target = _target; if (dev) dev->setTarget(_target); }

void VRNavPreset::setActive(bool b) {
    cout << "VRNavPreset::setActive " << name << ", " << b << endl;
    if (b) activate(); else deactivate(); }
bool VRNavPreset::isActive() { return active; }

void VRNavPreset::activate() {
    active = true;
    if (dev) dev->setSpeed(Vec2d(speedX, speedY));
    for (auto& b : bindings) updateBinding(b);
}

void VRNavPreset::deactivate() {
    active = false;
    for (auto& b : bindings) b.clearSignal();
}

void VRNavPreset::setBindingState(size_t i, bool b) {
    if (i >= bindings.size()) return;
    bindings[i].active = b;
    if (active) {
        deactivate();
        activate();
    }
}

vector<VRNavBinding>::iterator VRNavPreset::begin() { return bindings.begin(); }
vector<VRNavBinding>::iterator VRNavPreset::end() { return bindings.end(); }

vector<VRNavBinding>& VRNavPreset::getBindings() { return bindings; }

void VRNavPreset::addKeyBinding(VRNavBinding b) {
    bindings.push_back(b);
    updateBinding(b);
}

void VRNavPreset::setSpeed(float vt, float vr) {
    speedX = vt;
    speedY = vr;
    if (active) activate();
}

// preset management

VRNavigator_base::VRNavigator_base () {
    storeMap("presetStates", &presetStates, true);
}

VRNavigator_base::~VRNavigator_base() {
    presets.clear();
}

void VRNavigator_base::setNavigationState(string name, bool active) {
    presetStates[name] = active;
    if (presets.count(name)) presets[name]->setActive(active);
}

void VRNavigator_base::addNavigation(shared_ptr<VRNavPreset> ps) {
    string name = ps->getName();
    presets[name] = ps;
}

void VRNavigator_base::remNavigation(string name) {
    presets.erase(name);
}

shared_ptr<VRNavPreset> VRNavigator_base::getNavigation(string s) {
    if (presets.count(s) == 0) return 0;
    return presets[s];
}

string VRNavigator_base::getNavigationTip(string s) {
    string res;
    if (presets.count(s) == 0) return res;
    auto p = presets[s];

    auto& bngs = p->getBindings();
    for ( unsigned int i=0; i<bngs.size(); i++ ) {
        auto& v = bngs[i];
        res += v.cb->name + " ";
        res += toString(v.key) + " ";
        res += toString(v.state);
        if (i < bngs.size()-1) res += "\n";
    }

    return res;
}

vector<string> VRNavigator_base::getNavigationNames() { vector<string> res; for(auto p : presets) res.push_back(p.first); return res; }
map<string, shared_ptr<VRNavPreset>> VRNavigator_base::getNavigations() { return presets; }

void VRNavigator_base::storeNavigationCallback(VRDeviceCbPtr cb) { library[cb->name] = cb; }
VRDeviceCbPtr VRNavigator_base::getNavigationCallback(string s) { if (library.count(s)) return library[s]; return 0; }
map<string, VRDeviceCbPtr>& VRNavigator_base::getNavigationCallbacks() { return library; }


VRNavigator::VRNavigator() {
    cout << "VRNavigator::VRNavigator" << endl;
    setStorageType("Navigation");
    auto addNavCb = [&](string name, boost::function<bool(VRDeviceWeakPtr)> fkt) {
        storeNavigationCallback( VRDeviceCb::create(name, fkt) );
    };

    addNavCb("mouse_orbit2d", bind(&VRNavigator::orbit2D, this, _1) );
    addNavCb("mouse_orbit", bind(&VRNavigator::orbit, this, _1) );
    addNavCb("mouse_zoom_in", bind(&VRNavigator::zoom, this, _1, 1) );
    addNavCb("mouse_zoom_out", bind(&VRNavigator::zoom, this, _1, -1) );
    addNavCb("mouse_focus", bind(&VRNavigator::focus, this, _1) );
    addNavCb("mouse_walk", bind(&VRNavigator::walk, this, _1) );
    addNavCb("fly_walk", bind(&VRNavigator::fly_walk, this, _1) );
    addNavCb("hyd_walk", bind(&VRNavigator::hyd_walk, this, _1) );
}

VRNavigator::~VRNavigator() {}

// callbacks

float VRNavigator::clip_dist_down = 1.5;

bool VRNavigator::zoom(VRDeviceWeakPtr _dev, int dir) {
    auto dev = _dev.lock();
    if (!dev) return true;
    VRTransformPtr target = dev->getTarget();
    if (target == 0) return true;

    //Vec2d speed = dev->getSpeed(); // 0.05
    float speed = 0.05;
    target->zoom(speed*dir);
    return true;
}

bool VRNavigator::isCtrlDown() { return isKeyDown(65507); }
bool VRNavigator::isShiftDown() { return isKeyDown(65505); }

bool VRNavigator::isKeyDown(int k) {
    auto kb = VRSetup::getCurrent()->getDevice("keyboard");
    if (kb) return (kb->b_state(k) == 1);
    else return false;
}

bool VRNavigator::orbit(VRDeviceWeakPtr _dev) {
    auto dev = _dev.lock();
    if (!dev) return true;
    VRTransformPtr target = dev->getTarget();
    VRTransformPtr devBeacon = dev->getBeacon();

    if (target == 0) return true;
    if (devBeacon == 0) return true;

    static int state = 0;
    static Vec3d camPos;
    static Vec3d camSphereRef(0,0,0);
    static Vec3d camAtRef(0,0,0);
    static Pose camPanRef;
    static Vec2d mouseOnMouseDown;
    static bool doPan = false;

    Vec3d camDelta;
    Vec2d mousePos;
    mousePos[0] = dev->s_state(5);
    mousePos[1] = dev->s_state(6);

    if (state == 0) { // drag camera
        doPan = isShiftDown();

        camPos = target->getFrom();
        camAtRef = target->getAt();
        camPanRef = *target->getPose();
        camPanRef.makeUpOrthogonal();
        mouseOnMouseDown = mousePos;

        Vec3d dir = -target->getDir();
        camSphereRef[0] = dir.length();
        dir.normalize();

        camSphereRef[2] = asin(dir[1]);

        float cosb = cos(camSphereRef[2]);
        float cosa = dir[0]/cosb;
        cosa = min(cosa,  1.f);
        cosa = max(cosa, -1.f);
        float a = acos(cosa);
        if (dir[2]/cosb < 0) a *= -1;
        camSphereRef[1] = a;
    }

    state = dev->b_state(dev->key());
    //state = dev->b_state(1);
    //if (state < 0) return;
    //cout << "orbit state " << dev->key() << " " << state << endl;

    // move cam
    mousePos -= mouseOnMouseDown;

    if (!doPan) {
        camDelta = camSphereRef;
        camDelta[1] += mousePos[0]*1.5; //yaw
        camDelta[2] -= mousePos[1]*1.5; //pitch

        camDelta[2] = max(camDelta[2], -Pi*0.49);
        camDelta[2] = min(camDelta[2],  Pi*0.49);

        float cosa = cos(camDelta[1]);
        float sina = sin(camDelta[1]);
        float cosb = cos(camDelta[2]);
        float sinb = sin(camDelta[2]);

        camPos = Vec3d(cosa*cosb, sinb, sina*cosb)*camDelta[0] + camAtRef;
        target->setAt(camAtRef);
        target->setFrom(camPos);
	} else {
        // TODO:
        //  get reference at
        //  virtually move that point -> thus moving the camera accordingly

        Vec3d D = (camAtRef-camPanRef.pos());
        double d = D.length();

        Vec3d x = camPanRef.x(); x.normalize();
        Vec3d u = x.cross( camPanRef.dir() ); u.normalize();
        Vec3d delta = (-mousePos[0]*x -mousePos[1]*u)*d;
        camPos = camPanRef.pos() + delta;
        target->setAt(camAtRef + delta);
        target->setFrom(camPos);
	}
	return true;
}

bool VRNavigator::walk(VRDeviceWeakPtr _dev) {
    auto dev = _dev.lock();
    if (!dev) return true;
    VRTransformPtr target = dev->getTarget();
    VRTransformPtr devBeacon = dev->getBeacon();

    if (target == 0) return true;
    if (devBeacon == 0) return true;

    Vec3d dir_c = target->getWorldDirection();
    Vec3d dir_m = devBeacon->getAt() - devBeacon->getFrom();
    dir_m.normalize();
    dir_c.normalize();

    Vec2d speed = dev->getSpeed();
    float speedT = speed[0]; // 0.02
    float speedR = speed[1]; // 0.04

    Vec3d x = dir_c*dir_m[1]*speedT*exp(-abs(dir_m[0]));
    float y = -dir_m[0]*speedR*exp(-abs(dir_m[1]));

    target->rotate(y);
    target->translate(x);
    return true;
}

bool VRNavigator::fly_walk(VRDeviceWeakPtr _dev) {
    auto dev = _dev.lock();
    if (!dev) return true;
    int key = dev->key();
    float d = 0;
    dev->s_state(key, &d);
    float d_abs = abs(d);

    VRTransformPtr target = dev->getTarget();
    VRTransformPtr flystick = dev->getBeacon();
    if (target == 0 || flystick == 0) return true;
    if (target->getParent() == 0 ) return true;

    Vec3d dir = target->getParent()->getPoseTo(flystick)->dir();
    //Vec3d dir = flystick->getDir();
    dir.normalize();

    Vec2d speed = dev->getSpeed();
    float tspeed = speed[0]; // 1.5
    float rspeed = speed[1]; // 0.7

    float dt = 60.0/ max(1lu, VRGlobals::FRAME_RATE.fps);
    switch(key) {
        case 10:
            target->rotate(-dt*d*d_abs*0.1*rspeed);
            break;
        case 11:
            target->translate(dir*dt*d*d_abs*0.2*tspeed);
            break;
    }
    return true;
}

bool VRNavigator::orbit2D(VRDeviceWeakPtr _dev) {
    auto dev = _dev.lock();
    if (!dev) return true;
    VRTransformPtr target = dev->getTarget();
    VRTransformPtr devBeacon = dev->getBeacon();

    if (target == 0) return true;
    if (devBeacon == 0) return true;

    Vec3d dir_m = devBeacon->getAt() - devBeacon->getFrom();
    dir_m.normalize();

    float speedT = 0.02;
    float speedR = 0.04;

    float x = dir_m[1]*speedT*exp(-abs(dir_m[0]));
    float y = -dir_m[0]*speedR*exp(-abs(dir_m[1]));

    target->rotateAround(y);
    target->zoom(x);
    return true;
}

void animPathAt(VRTransformWeakPtr trp, PathPtr p, float t) {
    auto tr = trp.lock();
    if (!tr) return;
    tr->setWorldAt( p->getPosition(t) );
}

bool VRNavigator::focus(VRDeviceWeakPtr _dev) {
    auto dev = _dev.lock();
    if (!dev) return true;
    VRTransformPtr target = dev->getTarget();
    VRTransformPtr devBeacon = dev->getBeacon();
    auto scene = VRScene::getCurrent();

    if (target == 0) return true;
    if (devBeacon == 0) return true;

    VRIntersectionPtr ins = dev->intersect(target->getRoot());
    if (!ins->hit) return true;

    Vec3d z;
    auto p = Path::create();
    p->addPoint( Pose(target->getWorldAt(), z));
    p->addPoint( Pose(ins->point.subZero(), z));
    p->compute(20);

    focus_fkt = VRAnimCb::create("TransAnim", bind(animPathAt, VRTransformWeakPtr(target), p, _1));
    scene->addAnimation(0.3,0,focus_fkt,0,1);
    return true;
}

// presets

void VRNavigator::initWalk(VRTransformPtr target, VRDevicePtr dev) {
    auto preset = VRNavPreset::create();
    preset->setDevice(dev);
    preset->setTarget(target);
    preset->setSpeed(0.02, 0.04);

    VRNavBinding b1( getNavigationCallback("mouse_walk"), 0, 0, true);
    preset->addKeyBinding(b1);
    preset->deactivate();
    preset->setName("Walk");

    addNavigation(preset);
}

void VRNavigator::initOrbit(VRTransformPtr target, VRDevicePtr dev) {
    cout << "VRNavigator::initOrbit" << endl;
    auto preset = VRNavPreset::create();
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

void VRNavigator::initOrbit2D(VRTransformPtr target, VRDevicePtr dev) {
    auto preset = VRNavPreset::create();
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

void VRNavigator::initFlyOrbit(VRTransformPtr target, VRDevicePtr dev) { // TODO
    auto preset = VRNavPreset::create();
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

void VRNavigator::initFlyWalk(VRTransformPtr target, VRDevicePtr dev) {
    auto preset = VRNavPreset::create();
    preset->setDevice(dev);
    preset->setTarget(target);
    preset->setSpeed(0.4, 0.4);

    VRNavBinding b1( getNavigationCallback("fly_walk"), 10, 0, false);
    VRNavBinding b2( getNavigationCallback("fly_walk"), 11, 0, false);
    preset->addKeyBinding(b1);
    preset->addKeyBinding(b2);
    preset->deactivate();
    preset->setName("FlyWalk");

    addNavigation(preset);
}

bool VRNavigator::hyd_walk(VRDeviceWeakPtr _dev) {
    auto dev = _dev.lock();
    if (!dev) return true;
    int key = dev->key();
    float d = 0;
    dev->s_state(key, &d);
    float d_abs = abs(d);

    VRTransformPtr target = dev->getTarget();
    VRTransformPtr flystick = dev->getBeacon();
    if (target == 0 || flystick == 0) return true;

    Vec3d dir = flystick->getWorldDirection();
    dir.normalize();

    float rspeed = 0.1;
    float tspeed = 0.1;

    float dt = 60.0/ max(1lu, VRGlobals::FRAME_RATE.fps);
    switch(key) {
        case 20:
        case 23:
            target->rotate(-dt*d*d_abs*0.1*rspeed);
            break;
        case 21:
        case 24:
            target->translate(dir*dt*d*d_abs*0.2*tspeed);
            break;
    }
    return true;
}

void VRNavigator::initHydraFly(VRTransformPtr target, VRDevicePtr dev) {
    auto preset = VRNavPreset::create();
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
    for (auto p : presets) {
        bool active = true;
        if (presetStates.count(p.first)) active = presetStates[p.first];
        p.second->setActive(active);
    }
}


