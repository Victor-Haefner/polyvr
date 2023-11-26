#include "VRAnimation.h"

#include "core/utils/VRFunction.h"
#include "core/utils/system/VRSystem.h"
#include "core/scene/VRScene.h"

using namespace OSG;

VRAnimation::VRAnimation(string name) {
    setNameSpace("animation");
    setName(name);
}

VRAnimation::VRAnimation(float _duration, float _offset, VRAnimCbPtr _fkt, float _start, float _end, bool _loop, bool owned) : VRAnimation(_fkt->name) {
    run = false;

    duration = _duration;
    offset = _offset;
    loop = _loop;

    if (owned) addCallback(_fkt);
    else addUnownedCallback(_fkt);
    start_value = _start;
    end_value = _end;

    setNameSpace("animation");
    setName("anim"); // TODO: _fkt->getBaseName() is an empty string??
}

VRAnimation::~VRAnimation() { stop(); }

shared_ptr<VRAnimation> VRAnimation::create(string name) { return shared_ptr<VRAnimation>(new VRAnimation(name)); }
shared_ptr<VRAnimation> VRAnimation::create(float duration, float offset, VRAnimCbPtr fkt, float start, float end, bool loop, bool owned) { return shared_ptr<VRAnimation>(new VRAnimation(duration, offset, fkt, start, end, loop, owned)); }

void VRAnimation::execCallbacks(float t) {
    float val = start_value + (end_value - start_value)*t;
    for (auto cb : weakCallbacks) if ( auto CB = cb.lock() ) (*CB)(val);
    for (auto CB : ownedCallbacks) if ( CB ) (*CB)(val);
}

void VRAnimation::start(float offset) {
    this->offset = offset;
    start_time = getTime()*1e-6;
    run = true;
    VRScene::getCurrent()->addAnimation(shared_from_this());
}

void VRAnimation::stop() { run = false; }
bool VRAnimation::isActive() { return run; }
bool VRAnimation::isPaused() { return paused; }

void VRAnimation::addUnownedCallback(VRAnimCbPtr fkt) {
    weakCallbacks.push_back(fkt);
}

void VRAnimation::addCallback(VRAnimCbPtr fkt) {
    ownedCallbacks.push_back(fkt);
}

bool VRAnimation::getLoop() { return loop; }
float VRAnimation::getDuration() { return duration; }

void VRAnimation::setLoop(bool b) { loop = b; }

void VRAnimation::setDuration(float T) {
    if (run) start_time += t*(duration-T);
    duration = T;
}

bool VRAnimation::update(float current_time) {
    update_time = current_time;
    if (!run) return false;
    if (paused) return true;

    t = update_time - start_time - offset;
    if (t < 0) return true;

    if (duration > 0.00001) t /= duration;
    else t = 2;

    if (t >= 1) {
        if (loop) start(offset);
        else {
            stop();
            execCallbacks(1);
        }
        return true;
    }

    //cout << " t: " << t << endl;

    execCallbacks(t);
    return true;
}

void VRAnimation::goTo(float f) {
    if (run) {
        start_time -= (f-t) * duration;
    } else { // TODOt dt = (f-t) * duration;
    }
}

void VRAnimation::pause() {
    if (!run) return;
    if (paused) return;
    paused = true;
    pause_time = update_time;
    cout << "pause" << endl;
}

void VRAnimation::resume() {
    if (!run) return;
    if (!paused) return;
    start_time += update_time - pause_time;
    paused = false;
    cout << "resume" << endl;
}
