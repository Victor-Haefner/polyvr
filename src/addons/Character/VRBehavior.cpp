#include "VRBehavior.h"
#include "core/utils/toString.h"

using namespace OSG;

/*VRBehavior::Action::Action(string n) { setNameSpace("bhAction"); setName(n); }
VRBehavior::Action::~Action() {}
VRBehavior::ActionPtr VRBehavior::Action::create(string n) { return ActionPtr(new Action(n) ); }

void VRBehavior::Action::addConfiguration(VRSkeleton::ConfigurationPtr c) {
    configurations[c->getName()] = c;
}*/


VRBehavior::VRBehavior(string n) { setNameSpace("behavior"); setName(n); }
VRBehavior::~VRBehavior() {}
VRBehaviorPtr VRBehavior::create(string n) { return VRBehaviorPtr(new VRBehavior(n) ); }

void VRBehavior::setSkeleton(VRSkeletonPtr s) { skeleton = s; }

void VRBehavior::updateBones() {

}
