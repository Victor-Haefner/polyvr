#include "VRAnimationManager.h"
#include "VRAnimationManagerT.h"
#include <GL/glut.h>
#include <boost/bind.hpp>
#include "core/utils/VRFunction.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

void VRAnimationManager::updateAnimations() {
    float t = glutGet(GLUT_ELAPSED_TIME)/1000.0;//in seconds
    vector<int> toRemove;

    for (auto a : anim_map)
        if (a.second->update(t) == false) toRemove.push_back(a.first);

    for (auto k : toRemove) {
        anim_map.erase(k);
    }
}

VRAnimationManager::VRAnimationManager() {
    id = 0;
    updateAnimationsFkt = new VRFunction<int>("AnimationUpdateFkt", boost::bind(&VRAnimationManager::updateAnimations, this));
}

void VRAnimationManager::stopAnimation(int i) {
    if (anim_map.count(i)) anim_map.erase(i);
}

OSG_END_NAMESPACE;
