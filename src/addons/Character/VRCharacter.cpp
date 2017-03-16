#include "VRCharacter.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/material/VRMaterial.h"
#include <math.h>

const float Pi = 3.14159;

OSG_BEGIN_NAMESPACE;
using namespace std;


VRBehavior::Action::Action(string n) { setNameSpace("bhAction"); setName(n); }
VRBehavior::Action::~Action() {}
VRBehavior::ActionPtr VRBehavior::Action::create(string n) { return ActionPtr(new Action(n) ); }

void VRBehavior::Action::addConfiguration(VRSkeleton::ConfigurationPtr c) {
    configurations[c->getName()] = c;
}


VRBehavior::VRBehavior(string n) { setNameSpace("behavior"); setName(n); }
VRBehavior::~VRBehavior() {}
VRBehaviorPtr VRBehavior::create(string n) { return VRBehaviorPtr(new VRBehavior(n) ); }

void VRBehavior::setSkeleton(VRSkeletonPtr s) { skeleton = s; }

void VRBehavior::updateBones() {

}




void VRSkeleton::Joint::update(Graph::node& n, bool changed) {
    ;
}

VRSkeleton::JointPtr VRSkeleton::Joint::create() { return JointPtr(new Joint() ); }

VRSkeleton::JointPtr VRSkeleton::Joint::duplicate() {
    auto j = Joint::create();
    *j = *this;
    return j;
}



VRSkeleton::Configuration::Configuration(string n) { setNameSpace("skConfig"); setName(n); }
VRSkeleton::Configuration::~Configuration() {}
VRSkeleton::ConfigurationPtr VRSkeleton::Configuration::create(string n) { return ConfigurationPtr(new Configuration(n) ); }

void VRSkeleton::Configuration::setPose(int i, Vec3f p) { joints[i] = p; }


VRSkeleton::VRSkeleton() {
    armature = Armature::create();
}

VRSkeleton::~VRSkeleton() {}

VRSkeletonPtr VRSkeleton::create() { return VRSkeletonPtr(new VRSkeleton() ); }

int VRSkeleton::addBone(int j1, int j2) {
    auto& edge = armature->connect(j1, j2);
    //return edge.ID; // TODO
    return 0;
}

int VRSkeleton::addJoint(JointPtr c, Vec3f p) {
    auto i = armature->addNode();
    joints[i] = Joint::create();
    armature->getNode(i).box.setCenter(p);
    return i;
}

//Graph::edge VRSkeleton::getBone(int id) { /*return armature->getEdge(id);*/ } // TODO
VRConstraintPtr VRSkeleton::getJoint(int id) { return dynamic_pointer_cast<VRConstraint>( joints[id] ); }

void VRSkeleton::asGeometry(VRGeoData& data) {
    for (auto& joint : armature->getNodes()) {
        data.pushVert( Pnt3f(joint.box.center()) );
    }

    for (auto& jv : armature->getEdges()) {
        for (auto& j : jv) {
            data.pushLine(j.from, j.to);
        }
    }
}

void VRSkeleton::setupGeometry() {
    VRGeoData geo;
    asGeometry(geo);
    geo.apply( ptr() );

    auto m = VRMaterial::get("skeleton");
    m->setLit(0);
    m->setDiffuse(Vec3f(0,1,0));
    setMaterial(m);
}

void VRSkeleton::updateGeometry() {
    VRGeoData data(ptr());
    auto& joints = armature->getNodes();
    for (int i=0; i<joints.size(); i++) {
        auto& joint = joints[i];
        data.setVert(i, Pnt3f(joint.box.center()) );
    }
}

void VRSkeleton::simpleHumanoid() {
    auto ball = Joint::create();
    auto hinge = Joint::create();

    for (int i=3; i<6; i++) ball->setMinMax(i, -Pi*0.5, Pi*0.5);
    hinge->setMinMax(5, 0, Pi*0.5);

    auto ballJoint = [&]() { return ball->duplicate(); };
    auto hingeJoint = [&]() { return hinge->duplicate(); };

    // spine
    auto s1 = addJoint(ballJoint(), Vec3f(0,1,0) ); // lower spine
    auto s2 = addJoint(ballJoint(), Vec3f(0,1.2,0) );
    auto s3 = addJoint(ballJoint(), Vec3f(0,1.4,0) );
    auto s4 = addJoint(ballJoint(), Vec3f(0,1.5,0) );
    auto s5 = addJoint(ballJoint(), Vec3f(0,1.6,0) );
    auto s6 = addJoint(ballJoint(), Vec3f(0,1.8,0) ); // head

    auto b1 = addBone(s1,s2);
    auto b2 = addBone(s2,s3);
    auto b3 = addBone(s3,s4);
    auto b4 = addBone(s4,s5);
    auto b5 = addBone(s5,s6);

    auto doLeg = [&](float d) {
        auto l1 = addJoint(ballJoint(), Vec3f(d,1,0) ); // hip
        auto l2 = addJoint(hingeJoint(), Vec3f(d,0.5,0) ); // knee
        auto l3 = addJoint(ballJoint(), Vec3f(d,0,0) );
        auto l4 = addJoint(ballJoint(), Vec3f(d,0,-0.2) ); // toes

        auto b0 = addBone(s1,l1);
        auto b1 = addBone(l1,l2);
        auto b2 = addBone(l2,l3);
        auto b3 = addBone(l3,l4);
    };

    auto doArm = [&](float d) {
        auto a1 = addJoint(ballJoint(), Vec3f(d,1.5,0) ); // shoulder
        auto a2 = addJoint(hingeJoint(), Vec3f(d,1.2,0) ); // elbow
        auto a3 = addJoint(ballJoint(), Vec3f(d,0.8,0) );
        auto a4 = addJoint(ballJoint(), Vec3f(d,0.7,0) ); // hand

        auto b0 = addBone(s4,a1);
        auto b1 = addBone(a1,a2);
        auto b2 = addBone(a2,a3);
        auto b3 = addBone(a3,a4);
    };

    doLeg(-0.2); // left
    doLeg(0.2); // right
    doArm(-0.3); // left
    doArm(0.3); // right
}


VRCharacter::VRCharacter (string name) : VRGeometry(name) {}
VRCharacter::~VRCharacter() {}

VRCharacterPtr VRCharacter::create(string name) { return VRCharacterPtr(new VRCharacter(name) ); }
VRCharacterPtr VRCharacter::ptr() { return dynamic_pointer_cast<VRCharacter>( shared_from_this() ); }

void VRCharacter::setSkeleton(VRSkeletonPtr s) { skeleton = s; }
VRSkeletonPtr VRCharacter::getSkeleton() { return skeleton; }

void VRCharacter::addBehavior(VRBehaviorPtr b) { behaviors[b->getName()] = b; }
void VRCharacter::addAction(VRBehavior::ActionPtr a) { actions[a->getName()] = a; }

void VRCharacter::simpleSetup() {
    auto s = VRSkeleton::create();
    s->simpleHumanoid();
    setSkeleton(s);

    s->setupGeometry(); // visualize skeleton
    addChild(s);

    // leg configurations
    auto stretched_leg_L = VRSkeleton::Configuration::create("stretched_leg_L");
    stretched_leg_L->setPose(6,Vec3f());
    stretched_leg_L->setPose(7,Vec3f());
    stretched_leg_L->setPose(8,Vec3f());
    stretched_leg_L->setPose(9,Vec3f());

    auto lifted_leg_L = VRSkeleton::Configuration::create("lifted_leg_L");



    // actions
    auto stomp_L = VRBehavior::Action::create("stomp_L");
    stomp_L->addConfiguration( stretched_leg_L );
    stomp_L->addConfiguration( lifted_leg_L );
    stomp_L->addConfiguration( stretched_leg_L );
    addAction(stomp_L);
}

OSG_END_NAMESPACE;

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





