#include "VRAnimationManager.h"
#include "VRAnimationManagerT.h"

#include <vector>
#include "core/objects/VRAnimation.h"
#include "core/utils/VRFunction.h"
#include "core/utils/system/VRSystem.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

void VRAnimationManager::updateAnimations() {
    float t = getTime()*1e-6;//in seconds
    vector<string> toRemove;
    for (auto a : anim_map)
        if (a.second->update(t) == false) toRemove.push_back(a.first);
    for (auto k : toRemove) anim_map.erase(k);
}

VRAnimationManager::VRAnimationManager() {
    updateAnimationsFkt = VRUpdateCb::create("AnimationUpdateFkt", bind(&VRAnimationManager::updateAnimations, this));
}

void VRAnimationManager::addAnimation(VRAnimationPtr anim) {
    anim_map[anim->getName()] = anim;
}

void VRAnimationManager::remAnimation(VRAnimationPtr anim) {
    string n = anim->getName();
    if (anim_map.count(n)) anim_map.erase(n);
}

OSG_END_NAMESPACE;
