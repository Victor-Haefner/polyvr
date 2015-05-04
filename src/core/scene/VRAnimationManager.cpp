#include "VRAnimationManager.h"
#include "VRAnimationManagerT.h"
#include <GL/glut.h>
#include <boost/bind.hpp>
#include <vector>
#include "core/utils/VRFunction.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

void VRAnimationManager::updateAnimations() {
    float t = glutGet(GLUT_ELAPSED_TIME)/1000.0;//in seconds
    vector<string> toRemove;

    for (auto a : anim_map)
        if (a.second->update(t) == false) toRemove.push_back(a.first);

    for (auto k : toRemove) {
        anim_map.erase(k);
    }
}

VRAnimationManager::VRAnimationManager() {
    updateAnimationsFkt = new VRFunction<int>("AnimationUpdateFkt", boost::bind(&VRAnimationManager::updateAnimations, this));
}

void VRAnimationManager::addAnimation(VRAnimation* anim) {
    anim->start();
    anim_map[anim->getName()] = anim;
}

void VRAnimationManager::stopAnimation(VRAnimation* anim) {
    string n = anim->getName();
    if (anim_map.count(n)) anim_map.erase(n);
}

OSG_END_NAMESPACE;
