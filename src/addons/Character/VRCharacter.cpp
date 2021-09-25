#include "VRCharacter.h"
#include "VRSkeleton.h"
#include "VRSkin.h"
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

void VRCharacter::update() {
    if (skeleton) skeleton->resolveKinematics();
    if (skin) skin->updateBoneTexture();
}

void VRCharacter::simpleSetup() {
    auto s = VRSkeleton::create();
    s->setupSimpleHumanoid();
    setSkeleton(s);

    s->setupGeometry(); // visualize skeleton
    addChild(s);

    skin = VRSkin::create(s);

    VRGeoData hullData; // TODO: create test hull
    auto bones = s->getBones();
    for (size_t bID = 0; bID < bones.size(); bID++) {
        auto& bone = bones[bID];
        Vec3d p = (bone.p1+bone.p2)*0.5;
        Vec3d n = bone.up;
        Vec3d u = -bone.dir;
        Vec3d s = Vec3d(0.1, bone.length, 0.1);

        bool isFoot  = bool(bID == 0 || bID == 3);
        bool isAnkle = bool(bID == 2 || bID == 5);
        bool isHand  = bool(bID == 6 || bID == 9);
        bool isWrist = bool(bID == 8 || bID == 11);

        if (isFoot) { // feet
            Vec3d x = n.cross(u) * s[0]*0.5;
            u[1] = 0;
            u *= s[0]*0.5;

            Vec3d pH1 = bone.p1;
            Vec3d pH2 = bone.p1 - Vec3d(0,0.05,0);

            int v11 = hullData.pushVert(pH1 - x + u, n, Vec2d(0,0));
            int v12 = hullData.pushVert(pH1 - x - u, n, Vec2d(0,1));
            int v13 = hullData.pushVert(pH1 + x - u, n, Vec2d(1,1));
            int v14 = hullData.pushVert(pH1 + x + u, n, Vec2d(1,0));

            x *= 1.2;
            u *= 1.2;

            int v21 = hullData.pushVert(pH2 - x + u, n, Vec2d(0,0));
            int v22 = hullData.pushVert(bone.p2 - x, u, Vec2d(0,1));
            int v23 = hullData.pushVert(bone.p2 + x, u, Vec2d(1,1));
            int v24 = hullData.pushVert(pH2 + x + u, n, Vec2d(1,0));

            hullData.pushQuad(v11, v12, v13, v14);
            hullData.pushQuad(v21, v22, v23, v24);
            hullData.pushQuad(v11, v12, v22, v21);
            hullData.pushQuad(v12, v13, v23, v22);
            hullData.pushQuad(v13, v14, v24, v23);
            hullData.pushQuad(v14, v11, v21, v24);
        } else hullData.pushBox(p,n,u,s,true);
        cout << "   test hull quad: " << p << " / "  << n << " / " << u << " / " << s << "    " << u.dot(n) << endl;

        // create skin mapping
        size_t mI = skin->mapSize();

        float t1 = 1.0;
        float t2 = 1.0;
        if (!bone.isStart || isFoot  || isHand)  t1 = 0.5;
        if (!bone.isEnd   || isAnkle || isWrist) t2 = 0.5;

        for (int i=0; i<4; i++) skin->addMap(bone.ID, t1);
        for (int i=0; i<4; i++) skin->addMap(bone.ID, t2);

        if (isFoot)  for (int i : {0,1,2,3}) skin->addMap(bones[bID+2].ID, 0.5, mI+i);
        if (isHand)  for (int i : {0,1,2,3}) skin->addMap(bones[bID+2].ID, 0.5, mI+i);
        if (isAnkle) for (int i : {4,5,6,7}) skin->addMap(bones[bID-2].ID, 0.5, mI+i);
        if (isWrist) for (int i : {4,5,6,7}) skin->addMap(bones[bID-2].ID, 0.5, mI+i);

        if (!bone.isStart) for (int i : {0,1,2,3}) skin->addMap(bones[bID-1].ID, 0.5, mI+i);
        if (!bone.isEnd)   for (int i : {4,5,6,7}) skin->addMap(bones[bID+1].ID, 0.5, mI+i);
    }

    skin->updateBoneTexture();
    skin->updateMappingTexture();
    auto hull = hullData.asGeometry("hull");
    hull->updateNormals(true);
    addChild(hull);
    skin->applyMapping(hull);
    hull->setMaterial( skin->getMaterial() );
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

struct MoveTarget {
    PosePtr target;
    PathPtr path;
    VRAnimationPtr moveAnim;

    MoveTarget(PosePtr p1, PosePtr p2) : target(p1) {
        path = Path::create();
        if (p1 && p2) {
            path->addPoint(*p1.get());
            path->addPoint(*p2.get());
            path->compute(16);
        }
    }

    ~MoveTarget() {
        if (moveAnim) moveAnim->stop();
    }

    void start(VRAnimCbPtr animCb) {
        moveAnim = VRAnimation::create("moveAnim");
        moveAnim->setCallback(animCb);
        moveAnim->setDuration(1);
        moveAnim->start();
    }
};

void VRCharacter::moveEE(float t, string endEffector) {
    if (!target.count(endEffector)) return;
    MoveTarget* tgt = target[endEffector];
    if (!tgt->target) return;
    auto pos = tgt->path->getPose(t)->pos();
    tgt->target->setPos(pos);
}

void VRCharacter::pathWalk(float t) {
    if (!motion) return;
    PosePtr p = motion->path->getPose(t);
    p->setDir(-p->dir());
    PosePtr p0 = getPose();
    setPose(p);
    Vec3d D = p->pos()-p0->pos();
    float d = D.length();
    motion->leftCycle.advance(d);
    motion->rightCycle.advance(d);
}

PathPtr VRCharacter::moveTo(Vec3d p1, float speed) {
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
    if (abs(speed) < 1e-2) return 0;

    motion = new WalkMotion(getSkeleton(), path); // TODO

    walkAnim = VRAnimation::create("walkAnim");
    walkAnim->setCallback(VRAnimCb::create("walkAnim", bind(&VRCharacter::pathWalk, this, placeholders::_1) ));
    walkAnim->setDuration(path->getLength()/abs(speed));
    walkAnim->start();

    return path;
}

PathPtr VRCharacter::grab(Vec3d p1, float speed) {
    return 0;
}

void VRCharacter::move(string endEffector, PosePtr pose) {
    if (!skeleton) return;
    auto p0 = skeleton->getTarget(endEffector);

    if (target.count(endEffector)) delete target[endEffector];

    auto self = getPose();
    self->invert();
    pose = self->multRight(pose);
    target[endEffector] = new MoveTarget(p0, pose); // TODO
    target[endEffector]->start( VRAnimCb::create("moveAnim", bind(&VRCharacter::moveEE, this, placeholders::_1, endEffector) ) );
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





