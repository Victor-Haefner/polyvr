#include "VRAnimation.h"

#include "core/utils/VRFunction.h"
#include "core/utils/system/VRSystem.h"
#include "core/scene/VRScene.h"

using namespace OSG;

template<> string typeName(const VRAnimation& t) { return "Animation"; }


VRAnimation::interpolator::~interpolator() {;}

VRAnimation::VRAnimation(string name) {
    setNameSpace("animation");
    setName(name);
}

VRAnimation::~VRAnimation() {}

shared_ptr<VRAnimation> VRAnimation::create(string name) { return shared_ptr<VRAnimation>(new VRAnimation(name)); }

void VRAnimation::start(float offset) {
    this->offset = offset;
    start_time = getTime()*1e-6;
    run = true;
    VRScene::getCurrent()->addAnimation(shared_from_this());
}

void VRAnimation::stop() { run = false; }
bool VRAnimation::isActive() { return run; }
bool VRAnimation::isPaused() { return paused; }

void VRAnimation::setUnownedCallback(VRAnimCbPtr fkt) {
    setCallback(fkt);
    interp->own(false);
}

void VRAnimation::setCallback(VRAnimCbPtr fkt) {
    run = false;
    duration = 1;

    if (interp) delete interp;
    auto i = new interpolatorT<float>();
    i->sp = fkt;
    i->fkt = fkt;
    i->start_value = 0;
    i->end_value = 1;
    interp = i;
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
    if (accum_pause_time > 0) t -= accum_pause_time;
    if (t < 0) return true;

    if (duration > 0.00001) t /= duration;
    else t = 2;

    if (t >= 1) {
        if (loop) start(offset);
        else {
            stop();
            if (interp) interp->update(1);
        }
        return true;
    }

    cout << " t: " << t << endl;

    if (interp) interp->update(t);
    return true;
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
    accum_pause_time += update_time - pause_time;
    paused = false;
    cout << "resume" << endl;
}
