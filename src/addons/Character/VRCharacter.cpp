#include "VRCharacter.h"
#include "VRSkeleton.h"
#include "VRBehavior.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/VRAnimation.h"
#include "core/utils/toString.h"
#include "core/utils/VRFunction.h"
#include "core/math/path.h"
#include "core/scene/VRScene.h"
#include <math.h>

using namespace OSG;

VRCharacter::VRCharacter (string name) : VRGeometry(name) {
    updateCb = VRUpdateCb::create("character-update", bind(&VRCharacter::update, this));
    VRScene::getCurrent()->addUpdateFkt(updateCb);
}

VRCharacter::~VRCharacter() {}

VRCharacterPtr VRCharacter::create(string name) { return VRCharacterPtr(new VRCharacter(name) ); }
VRCharacterPtr VRCharacter::ptr() { return dynamic_pointer_cast<VRCharacter>( shared_from_this() ); }

void VRCharacter::setSkeleton(VRSkeletonPtr s) { skeleton = s; }
VRSkeletonPtr VRCharacter::getSkeleton() { return skeleton; }

void VRCharacter::addBehavior(VRBehaviorPtr b) { behaviors[b->getName()] = b; }
//void VRCharacter::addAction(VRBehavior::ActionPtr a) { actions[a->getName()] = a; }

void VRCharacter::move(string endEffector, PosePtr pose) {
    if (!skeleton) return;
    //skeleton->move(endEffector, pose);
}

void VRCharacter::update() {
    skeleton->resolveKinematics();
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


struct WalkCycle {
    WalkCycle* other = 0;
    int state = 0;
    PosePtr t1;
    PosePtr t2;
    float a = 0;
    float x = 0;

    WalkCycle() {}

    WalkCycle(PosePtr t1, PosePtr t2, float x) {
        this->t1 = t1;
        this->t2 = t2;
        this->x = x;
    }

	void advance(float d) {
        if (state == 0) { // foot on ground
            t1->move(d); // move back
            t2->move(d); // move back
        }

        if (state == 1) {
            a += d*10; // step arc angle
            Vec3d tp = t1->pos();
            tp[0] = 0;
            Vec3d p0 = Vec3d(x, sin(a)*tp.length(), -cos(a)*tp.length());
            t1->setPos(p0);
            t2->setPos(p0 + Vec3d(0,0,0.2));
            if (a >= Pi) {
                a = 0;
                state = 0;
                if (other) other->state = 1;
            }
        }
    }

    void setStartPositions() {
        t1->setPos(Vec3d(x,0,0.2));
        t2->setPos(Vec3d(x,0,0.4));
    }
};

struct WalkMotion {
    WalkCycle leftCycle;
    WalkCycle rightCycle;
    PathPtr path;

    WalkMotion(VRSkeletonPtr s, PathPtr p) {
        PosePtr aL = s->getTarget("ankleL");
        PosePtr tL = s->getTarget("toesL");
        PosePtr aR = s->getTarget("ankleR");
        PosePtr tR = s->getTarget("toesR");

        leftCycle = WalkCycle(aL, tL, 0.2);
        rightCycle = WalkCycle(aR, tR, -0.2);
        leftCycle.other = &rightCycle;
        rightCycle.other = &leftCycle;
        leftCycle.state = 1;

        leftCycle.setStartPositions();
        rightCycle.setStartPositions();

        path = p;
    }
};

void VRCharacter::pathWalk(float t) {
    PosePtr p = motion->path->getPose(t);
    p->setDir(-p->dir());
    PosePtr p0 = getPose();
    setPose(p);
    Vec3d D = p->pos()-p0->pos();
    float d = D.length();
    motion->leftCycle.advance(d);
    motion->rightCycle.advance(d);
}

PathPtr VRCharacter::moveTo(Vec3d p1) {
    auto p0 = getPose();
    Vec3d d = -p0->dir();
    d.normalize();
    p0->setDir(d);
    Vec3d D = p1-p0->pos();
    Vec3d pm = p0->pos()+D.length()*0.5*p0->dir();
    D = p1-pm;
    D.normalize();

    auto path = Path::create();
    path->addPoint(*p0);
    path->addPoint(Pose(p1, D));
    path->compute(32);

    cout << "moveTo " << p1 << " " << D << endl;

    if (walkAnim) walkAnim->stop();
    if (motion) delete motion;
    motion = new WalkMotion(getSkeleton(), path); // TODO

    walkAnim = VRAnimation::create("walkAnim");
    walkAnim->setCallback(VRAnimCb::create("walkAnim", bind(&VRCharacter::pathWalk, this, placeholders::_1) ));
    walkAnim->setDuration(path->getLength()*2);
    walkAnim->start();

    return path;
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





