#include "VRAnimation.h"

#include "core/utils/VRFunction.h"
#include "core/scene/VRSceneManager.h"
#include "core/scene/VRScene.h"

#include <GL/glut.h>

OSG_BEGIN_NAMESPACE;

VRAnimation::VRAnimation(string name) {
    setNameSpace("animation");
    setName(name);
}

void VRAnimation::start() {
    start_time = glutGet(GLUT_ELAPSED_TIME)/1000.0;
    run = true;
    VRSceneManager::getCurrent()->addAnimation(this);
}

void VRAnimation::stop() { run = false; }
bool VRAnimation::isActive() { return run; }

bool VRAnimation::update(float current_time) {
    if (!run) return false;

    float t = current_time - start_time - offset;
    if (t < 0) return true;

    if (duration > 0.00001) t /= duration;
    else t = 2;

    if (t > 1) {
        if (loop) start();
        else {
            stop();
            interp->update(1);
        }
        return true;
    }

    interp->update(t);
    return true;
}

OSG_END_NAMESPACE;
