#include "VRCharacter.h"
#include "VRSkeleton.h"
#include "VRBehavior.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/material/VRMaterial.h"
#include "core/utils/toString.h"
#include <math.h>

const float Pi = 3.14159;

using namespace OSG;

VRCharacter::VRCharacter (string name) : VRGeometry(name) {}
VRCharacter::~VRCharacter() {}

VRCharacterPtr VRCharacter::create(string name) { return VRCharacterPtr(new VRCharacter(name) ); }
VRCharacterPtr VRCharacter::ptr() { return dynamic_pointer_cast<VRCharacter>( shared_from_this() ); }

void VRCharacter::setSkeleton(VRSkeletonPtr s) { skeleton = s; }
VRSkeletonPtr VRCharacter::getSkeleton() { return skeleton; }

void VRCharacter::addBehavior(VRBehaviorPtr b) { behaviors[b->getName()] = b; }
//void VRCharacter::addAction(VRBehavior::ActionPtr a) { actions[a->getName()] = a; }

void VRCharacter::move(string endEffector, PosePtr pose) {
    if (!skeleton) return;
    skeleton->move(endEffector, pose);
}

void VRCharacter::simpleSetup() {
    auto s = VRSkeleton::create();
    s->setupSimpleHumanoid();
    setSkeleton(s);

    s->setupGeometry(); // visualize skeleton
    addChild(s);

    // leg configurations
    auto stretched_leg_L = VRSkeleton::Configuration::create("stretched_leg_L");
    stretched_leg_L->setPose(6,Vec3d());
    stretched_leg_L->setPose(7,Vec3d());
    stretched_leg_L->setPose(8,Vec3d());
    stretched_leg_L->setPose(9,Vec3d());

    auto lifted_leg_L = VRSkeleton::Configuration::create("lifted_leg_L");



    // actions
    /*auto stomp_L = VRBehavior::Action::create("stomp_L");
    stomp_L->addConfiguration( stretched_leg_L );
    stomp_L->addConfiguration( lifted_leg_L );
    stomp_L->addConfiguration( stretched_leg_L );
    addAction(stomp_L);*/
}

void VRCharacter::overrideSim(VRUpdateCbPtr cb) {
    if (!skeleton) return;
    skeleton->overrideSim(cb);
}

/** TODO

- Path finding class
- Behavior class

add lots of configurations like
stretched_leg, sitting_leg, kneeling_leg, ...

use state machine to define what configuration is accessible from another!
use path finding in state machine state graph, to get from one configuration in another

refactor skeleton configuration!
dont use absolute joint positions!
maybe add bone lengths?
maybe add joint angles? -> transformation matrices?

*/





