#include "VRAnimation.h"

#include "core/utils/VRFunction.h"
#include "core/scene/VRScene.h"

#include <GL/glut.h>

OSG_BEGIN_NAMESPACE;

VRAnimation::interpolator::~interpolator() {;}

VRAnimation::VRAnimation(string name) {
    setNameSpace("animation");
    setName(name);
}

VRAnimation::~VRAnimation() {}

shared_ptr<VRAnimation> VRAnimation::create(string name) { return shared_ptr<VRAnimation>(new VRAnimation(name)); }

void VRAnimation::start(float offset) {
    this->offset = offset;
    start_time = glutGet(GLUT_ELAPSED_TIME)/1000.0;
    run = true;
    VRScene::getCurrent()->addAnimation(shared_from_this());
}

void VRAnimation::stop() { run = false; }
bool VRAnimation::isActive() { return run; }

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

void VRAnimation::setLoop(bool b) { loop = b; }
bool VRAnimation::getLoop() { return loop; }

void VRAnimation::setDuration(float t) { duration = t; }
float VRAnimation::getDuration() { return duration; }

bool VRAnimation::update(float current_time) {
    if (!run) return false;

    float t = current_time - start_time - offset;
    if (t < 0) return true;

    if (duration > 0.00001) t /= duration;
    else t = 2;

    if (t > 1) {
        if (loop) start(offset);
        else {
            stop();
            if (interp) interp->update(1);
        }
        return true;
    }

    if (interp) interp->update(t);
    return true;
}

OSG_END_NAMESPACE;
