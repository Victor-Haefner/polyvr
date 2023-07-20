#include "VRRobotArm.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/math/path.h"
#include "core/objects/VRAnimation.h"
#include "core/scene/VRScene.h"
#include "core/utils/VRFunction.h"
#include "core/utils/toString.h"
#include "core/utils/isNan.h"
#include "core/tools/VRAnalyticGeometry.h"
#include <OpenSG/OSGQuaternion.h>

using namespace OSG;

VRRobotArm::VRRobotArm(string type) : type(type) {
    lastPose = Pose::create();
    angles.resize(N,0);
    angle_targets.resize(N,0);
    animPath = Path::create();
    robotPath = Path::create();
    anim = VRAnimation::create("animOnPath");
#ifndef WASM
    ageo = VRAnalyticGeometry::create();
    ageo->setLabelParams(0.05, 1, 1, Color4f(1,1,1,1), Color4f(1,1,1,0));
    ageo->hide("SHADOW");
#endif

    animPtr = VRFunction<float>::create("animOnPath", bind(&VRRobotArm::animOnPath, this, placeholders::_1 ) );
    //anim->addUnownedCallback(animPtr);
    anim->addCallback(animPtr);

    updatePtr = VRUpdateCb::create("run engines", bind(&VRRobotArm::update, this) );
    VRScene::getCurrent()->addUpdateFkt(updatePtr, 999);
}

VRRobotArm::~VRRobotArm() {
    anim->stop();
}

shared_ptr<VRRobotArm> VRRobotArm::create(string type) { return shared_ptr<VRRobotArm>(new VRRobotArm(type)); }

void VRRobotArm::setParts(vector<VRTransformPtr> parts) {
    this->parts.clear();
    for (auto p : parts) if (p) this->parts.push_back(p);
    if (ageo && parts[0]) ageo->switchParent(parts[0]->getParent());
}

void VRRobotArm::setEventCallback(VRMessageCbPtr mCb) { eventCb = mCb; }

void VRRobotArm::setAngleOffsets(vector<float> offsets) { angle_offsets = offsets; }
void VRRobotArm::setAngleDirections(vector<int> dirs) { angle_directions = dirs; }
void VRRobotArm::setAxis(vector<int> axis) { this->axis = axis; }
void VRRobotArm::setLengths(vector<float> lengths) { this->lengths = lengths; }
void VRRobotArm::setAxisOffsets(vector<float> offsets) { this->axis_offsets = offsets; }
vector<float> VRRobotArm::getAngles() { return angles; }
vector<float> VRRobotArm::getTargetAngles() {
    vector<float> res;
    for (int i=0; i<N; i++) res.push_back(convertAngle(angle_targets[i], i));
    return res;
}

VRTransformPtr VRRobotArm::genKinematics() {
    auto base = VRTransform::create("base");
    auto baseUpper = VRTransform::create("baseUpper");
    auto beam1 = VRTransform::create("beam1");
    auto elbow = VRTransform::create("elbow");
    auto beam2 = VRTransform::create("beam2");
    auto wrist = VRTransform::create("wrist");

    base->addChild(baseUpper);
    baseUpper->addChild(beam1);
    beam1->addChild(elbow);
    elbow->addChild(beam2);
    beam2->addChild(wrist);

    //vector<VRTransformPtr> objs = { baseUpper, beam1, elbow, beam2 };

    baseUpper->setFrom(Vec3d(0,lengths[0],0));
    beam1->setFrom(Vec3d(0,0,axis_offsets[0]));
    elbow->setFrom(Vec3d(0,lengths[1],0));
    beam2->setFrom(Vec3d(0,0,0));
    wrist->setFrom(Vec3d(0,lengths[2],0));

    return base;
}

double clamp(double f, double a = -1, double b = 1) { return f<a ? a : f>b ? b : f; }

void VRRobotArm::applyAngles() {
    //cout << "VRRobotArm::applyAngles " << N << endl;
    for (int i=0; i<N; i++) {
        Vec3d euler;
        euler[axis[i]] = angles[i];
        if (parts[i]) parts[i]->setEuler(euler);
    }
}

double VRRobotArm::convertAngle(double a, int i) {
    if (i >= angle_directions.size() || i >= angle_offsets.size()) return 0;
    return angle_directions[i]*a + angle_offsets[i]*Pi;
}

void VRRobotArm::update() { // update robot joint angles
    bool m = false;

    for (int i=0; i<N; i++) {
        double a = convertAngle(angle_targets[i], i);
        double da = a - angles[i];
        if (isNan(da)) continue;
        while (da >  Pi) da -= 2*Pi;
        while (da < -Pi) da += 2*Pi;
        if (abs(da) > 1e-4) m = true;
        angles[i] += clamp( da, -maxSpeed, maxSpeed );
    }

    if (m) applyAngles();
    if (eventCb && moving && !m) (*eventCb)("stopped");
    if (eventCb && !moving && m) (*eventCb)("moving");
    moving = m;
}

bool VRRobotArm::isMoving() { return anim->isActive() || moving; }

void VRRobotArm::grab(VRTransformPtr obj) {
    if (dragged) drop();
    if (parts.size() == 0) return;
    auto ee = parts[parts.size()-1];
    if (!ee) return;
    obj->drag(ee);
    dragged = obj;
    //cout << "VRRobotArm::grab obj " << obj << ", ee " << ee << endl;
    //for (auto e : parts) cout << "  part " << e << endl;
}

VRTransformPtr VRRobotArm::drop() {
    auto d = dragged;
    if (dragged) dragged->drop();
    dragged = 0;
    return d;
}

/*

Analytic Kinematics Model

Bodies:
- body 0 -> upper base, rotates (Y) above the lower base
- body 1 -> first beam
- body 2 -> elbow part
- body 3 -> second beam,
- body 4 -> wrist, last robot part without tool
- body 5 -> tool base
- body 6 -> tool finger 1
- body 7 -> tool finger 2

Angles:
- a[0] -> angle between lower and upper base parts, main Y rotation of the robot
- a[1] -> angle between upper base and first beam
- a[2] -> angle between first beam and elbow part
- a[3] -> angle between elbow part and second beam
- a[4] -> angle between second beam and tool interface
- a[5] -> angle between tool interface and tool up

Lengths:
- l[0] -> height of robot base
- l[1] -> distance between joint A, B
- l[2] -> distance between joint B, C
- l[3] -> padding of end effector

Computation Parameters:
- L -> length from robot base joint to end effector
- r1/r2 -> lengths 1/2, see above
- b, a, f -> angle_targets
- av -> direction from elbow to wrist joints
- e0, e1 -> elbow/wrist joint axis directions

*/

PosePtr VRRobotArm::calcForwardKinematics(vector<float> angles) {
    PosePtr res;
    if (type == "kuka") res = calcForwardKinematicsKuka(angles);
    else if (type == "aubo") res = calcForwardKinematicsAubo(angles);
    else res = calcForwardKinematicsKuka(angles); // default
    return res;
}

vector<float> VRRobotArm::calcReverseKinematics(PosePtr p) {
    vector<float> resultingAngles;
    if (type == "kuka") resultingAngles = calcReverseKinematicsKuka(p);
    else if (type == "aubo") resultingAngles = calcReverseKinematicsAubo(p);
    else resultingAngles = calcReverseKinematicsKuka(p); // default
    lastPose = p;
    return resultingAngles;
}

PosePtr VRRobotArm::calcForwardKinematicsKuka(vector<float> angles) {
    float r1 = lengths[1];
    float r2 = lengths[2];
    //float b = Pi-angles[2];
    float a = angles[1] + Pi*0.5;
    float f = angles[0];

    float a2 = angles[2];
    float a3 = angles[3];
    float a4 = angles[4];
    float a5 = angles[5];

    Pose pB(Vec3d(0,lengths[0],0)); // ok
    Pose pA0(Vec3d()                        , Vec3d(-sin(f), 0, -cos(f))    , Vec3d(cos(f), 0, -sin(f))); // ok
    Pose pA01(Vec3d(0,0,axis_offsets[0])); // ok
    Pose pA1(Vec3d(-r1*sin(a),0,r1*cos(a))  , Vec3d(-sin(a), 0, cos(a))); // ok
    Pose pA2(Vec3d()                        , Vec3d( sin(a2), 0, cos(a2))); // ok
    Pose pA3(Vec3d(0,0,r2)                  , Vec3d(0,0,-1)                 , Vec3d(-sin(a3), cos(a3), 0)); // ok
    Pose pA4(Vec3d()                        , Vec3d(-sin(a4), 0, -cos(a4))); // ok
    Pose pA5(Vec3d(0,0,lengths[3])          , Vec3d(0,0,1)                 , Vec3d(cos(a5), -sin(a5), 0)); // ok

    auto mB = pB.asMatrix();
    mB.mult(pA0.asMatrix());
    mB.mult(pA01.asMatrix());
    mB.mult(pA1.asMatrix());
    mB.mult(pA2.asMatrix());
    mB.mult(pA3.asMatrix());
    mB.mult(pA4.asMatrix());
    mB.mult(pA5.asMatrix());
    //if (ageo) ageo->setVector(18, Vec3d(), Vec3d(mB[3]), Color3f(0,1,0), "a5");
    //if (ageo) ageo->setVector(19, Vec3d(mB[3]), Vec3d(mB[2])*0.2, Color3f(1,0,1), "a5d");
    //if (ageo) ageo->setVector(20, Vec3d(mB[3]), Vec3d(mB[1])*0.2, Color3f(1,0.5,0), "a5u");
    return Pose::create(mB);
}

PosePtr VRRobotArm::calcForwardKinematicsAubo(vector<float> angles) { // TODO
    return getLastPose();
    /*if (parts.size() < 5) return 0;
    auto pose = parts[4]->getWorldPose();
    pose->setDir(-pose->dir());
    Vec3d p = pose->pos() + pose->dir()*lengths[3];
    pose->setPos(p);
    return pose;*/
}

vector<float> VRRobotArm::calcReverseKinematicsKuka(PosePtr p) {
    Vec3d pos = p->pos();
    Vec3d dir = p->dir();
    Vec3d up  = p->up();
    dir.normalize();
    up.normalize();
    pos -= dir* lengths[3];

    vector<float> resultingAngles = angle_targets;

    pos[1] -= lengths[0];

    float f = atan2(pos[0], pos[2]);
    resultingAngles[0] = f;
    Vec3d e0 = Vec3d(cos(-f),0,sin(-f));

    Vec3d aD = Vec3d(sin(f), 0, cos(f));
    pos -= aD * axis_offsets[0];

    float r1 = lengths[1];
    float r2 = sqrt(lengths[2]*lengths[2] + axis_offsets[1]*axis_offsets[1]);
    float L = pos.length();
    float ba = atan(axis_offsets[1] / lengths[2]);
    float b = acos( clamp( (L*L-r1*r1-r2*r2)/(-2*r1*r2) ) );
    resultingAngles[2] = -b + Pi + ba;

    float a = asin( clamp( r2*sin(b)/L ) ) + asin( clamp( pos[1]/L ) );
	resultingAngles[1] = a - Pi*0.5;

    // end effector
    float e = a+b-ba; // counter angle
    Vec3d av = Vec3d(-cos(e)*sin(f), -sin(e), -cos(e)*cos(f));
    Vec3d e1 = dir.cross(av);
    e1.normalize();

    float det = av.dot( e1.cross(e0) );
    float g = clamp( -e1.dot(e0) );
    resultingAngles[3] = det < 0 ? -acos(g) : acos(g);
    resultingAngles[4] = acos( av.dot(dir) );

    if (resultingAngles.size() > 4) {
        float det = dir.dot( e1.cross(up) );
        resultingAngles[5] = det < 0 ? -acos( e1.dot(up) ) : acos( e1.dot(up) );
    }


    // analytics visualization ---------------------------------------------------------
    if (showModel && ageo) {
        float sA = 0.05;
        Vec3d ra2 = e0*2*sA; // rotation axis base joint and elbow
        Vec3d ra3 = e1*2*sA; // rotation axis wrist

        Vec3d pJ0 = Vec3d(0,lengths[0],0); // base joint
        Vec3d pJ01 = pJ0 + Vec3d(sin(f), 0, cos(f)) * axis_offsets[0]; // base joint
        Vec3d pJ1 = pJ01 + Vec3d(cos(a)*sin(f), sin(a), cos(a)*cos(f)) * lengths[1]; // elbow joint
        double oa = b+a-Pi*0.5-ba;
        Vec3d a10 = Vec3d(cos(oa)*sin(f), sin(oa), cos(oa)*cos(f));
        Vec3d pJ11 = pJ1 + a10 * axis_offsets[1];
        Vec3d pJ2 = pJ11 + av * r2; // wrist joint

        // EE
        ageo->setVector(0, Vec3d(), pJ2, Color3f(0.6,0.8,1), "");
        ageo->setVector(1, pJ2, dir*0.1, Color3f(0,0,1), "");
        ageo->setVector(2, pJ2, up*0.1, Color3f(1,0,0), "");

        // rot axis
        ageo->setVector(3, pJ01 - Vec3d(0,sA,0), Vec3d(0,2*sA,0), Color3f(1,1,0.5), "");
        ageo->setVector(4, pJ01 - e0*sA, ra2, Color3f(1,1,0.5), "");
        ageo->setVector(5, pJ1 - e0*sA, ra2, Color3f(1,1,0.5), "");
        ageo->setVector(6, pJ2 - e1*sA, ra3, Color3f(1,1,0.5), "");

        // beams
        ageo->setVector(7, Vec3d(), pJ0, Color3f(1,1,1), "l0");
        ageo->setVector(8, pJ01, pJ1-pJ01, Color3f(1,1,1), "r1");
        ageo->setVector(9, pJ11, pJ2-pJ11, Color3f(1,1,1), "r2");

        ageo->setVector(10, pJ0, pJ01-pJ0, Color3f(0.7,0.7,0.7), "");
        ageo->setVector(11, pJ1, pJ11-pJ1, Color3f(0.7,0.7,0.7), "");
    }

    //calcForwardKinematicsKuka(resultingAngles); // temp, remove once debugged!

    return resultingAngles;
}

vector<float> VRRobotArm::calcReverseKinematicsAubo(PosePtr p) {
    Vec3d pos = p->pos();
    Vec3d dir = p->dir();
    Vec3d up  = p->up();

    // front kinematics
    pos -= dir* lengths[3]; // yield space for tool

    Vec3d posXZ(pos[0], 0, pos[2]);
    float w = acos(lengths[6]/posXZ.length());
    Vec3d n = posXZ;
    n.normalize();
    Quaterniond q(Vec3d(0,1,0), w);
    q.multVec(n,n);
    //Vec3d n = Vec3d(cos(w),0,sin(w));  // -------------------------- n still WRONG!!

    Vec3d e = dir.cross(n); // TODO: handle n // d
    e.normalize();

    Vec3d P1 = p->pos()-dir*lengths[3];
    Vec3d P2 = P1-e*lengths[5];
    Vec3d P3 = P2-n*lengths[6];

    vector<float> resultingAngles = angle_targets;

    // main arm
    pos = P3;
    pos[1] -= lengths[0]; // substract base offset
    float r1 = lengths[1];
    float r2 = lengths[2];
    float L = pos.length();
    float b = acos( clamp( (L*L-r1*r1-r2*r2)/(-2*r1*r2) ) );
    resultingAngles[2] = -b + Pi;

    float a = asin( clamp( r2*sin(b)/L ) ) + asin( clamp( pos[1]/L ) );
	resultingAngles[1] = a - Pi*0.5;
	//resultingAngles[1] = a;

    float f = atan2(pos[0], pos[2]);
    resultingAngles[0] = f;

    Vec3d e0 = Vec3d(cos(-f),0,sin(-f));


    // last angle connecting both subsystems
    Vec3d pJ0 = Vec3d(0,lengths[0],0); // base joint
    Vec3d pJ1 = pJ0 + Vec3d(cos(a)*sin(f), sin(a), cos(a)*cos(f)) * lengths[1]; // elbow joint
    Vec3d pJ2 = pJ1 - Vec3d(cos(a+b)*sin(f), sin(a+b), cos(a+b)*cos(f)) * lengths[2]; // wrist joint (P3)

    // TODO: works only for a quarter of the angles!
    auto getAngle = [](Vec3d u, Vec3d v, Vec3d w) {
        float a = u.enclosedAngle(v);
        Vec3d d = u.cross(v);
        float k = w.dot(d);
        int s = k >= 0 ? 1 : -1;
        return a*s;
    };

    resultingAngles[3] = getAngle(P1-P2, pJ1-pJ2, P3-P2) - Pi*0.5;
    resultingAngles[4] = getAngle(dir, P3-P2, P2-P1) - Pi*0.5;


    // analytics visualization ---------------------------------------------------------
    if (showModel && ageo) {
        float sA = 0.05;

        // EE
        ageo->setVector(0, Vec3d(), pJ2, Color3f(0.6,0.8,1), "");
        ageo->setVector(1, P1, dir*0.1, Color3f(0,0,1), "");
        ageo->setVector(2, P1, up*0.1, Color3f(1,0,0), "");

        // rot axis
        ageo->setVector(3, pJ0 - Vec3d(0,sA,0), Vec3d(0,2*sA,0), Color3f(1,1,0.5), "");
        ageo->setVector(4, pJ0 - e0*sA, e0*2*sA, Color3f(1,1,0.5), "");
        ageo->setVector(5, pJ1 - e0*sA, e0*2*sA, Color3f(1,1,0.5), "");
        ageo->setVector(6, pJ2 - e0*sA, e0*2*sA, Color3f(1,1,0.5), "");

        // beams
        ageo->setVector(7, Vec3d(), pJ0, Color3f(1,1,1), "l0");
        ageo->setVector(8, pJ0, pJ1-pJ0, Color3f(1,1,1), "r1");
        ageo->setVector(9, pJ1, pJ2-pJ1, Color3f(1,1,1), "r2");

        // front kinematics
        ageo->setVector(10, P1, p->pos()-P1, Color3f(1,0,0), "");
        ageo->setVector(11, P2, P1-P2, Color3f(1,0,0), "");
        ageo->setVector(12, P3, P2-P3, Color3f(1,0,0), "");

        ageo->setVector(13, p->pos(), Vec3d(), Color3f(1,0,0), "P");
        ageo->setVector(14, P1, Vec3d(), Color3f(1,0,0), "P1");
        ageo->setVector(15, P2, Vec3d(), Color3f(1,0,0), "P2");
        ageo->setVector(16, P3, Vec3d(), Color3f(1,0,0), "P3");

        // initial rectangle structure
        ageo->setVector(17, Vec3d(), n*0.3, Color3f(0,1,0), "N");
        ageo->setVector(18, Vec3d(), posXZ, Color3f(0,1,0), "");
        ageo->setVector(19, posXZ, -n*lengths[6], Color3f(0,1,0), "l6");
    }

    return resultingAngles;
}

void VRRobotArm::showAnalytics(bool b) { showModel = b; if (ageo) ageo->setVisible(b); }

void VRRobotArm::animOnPath(float t) {
    if (job_queue.size() == 0) { anim->stop(); return; }
    auto job = job_queue.front();
    anim->setDuration(job.d);

    auto updateTargets = [&](float t) {
        auto pose = job.p->getPose(t);
        pose->normalizeOrientationVectors();
        if (job.po) {
            auto poseO = job.po->getPose(t);
            pose->set(pose->pos(), poseO->dir(), poseO->up());
        }
        angle_targets = calcReverseKinematics(pose);

        //auto fP = calcForwardKinematics(angle_targets);
        //cout << " updateTargets, P: " << pose->toString() << endl;
        //cout << "                F: " << fP->toString() << endl;
        //for (auto a : angle_targets) cout << " " << a << endl;
    };

    t += job.t0;

    if (t >= job.t1) { // finished
        updateTargets(job.t1);
        if (!job.loop) job_queue.pop_front(); // queue next job
        anim->start(0); // restart animation
        return;
    }

    updateTargets(t);
}

void VRRobotArm::addJob(job j) {
    job_queue.push_back(j);
    if (!anim->isActive()) anim->start(0);
}

void VRRobotArm::move() {}
void VRRobotArm::pause() {}
void VRRobotArm::stop() {
    job_queue.clear();
    anim->stop();
}

void VRRobotArm::setAngles(vector<float> angles, bool force) {
    this->angle_targets = angles;
    if (force) {
        for (int i=0; i<N; i++) this->angles[i] = convertAngle(angles[i], i);
        applyAngles();
    }
}

void VRRobotArm::setSpeed(float s) { animSpeed = s; }
void VRRobotArm::setMaxSpeed(float s) { maxSpeed = s; }

PosePtr VRRobotArm::getLastPose() { return lastPose; }
PosePtr VRRobotArm::getPose() { return getLastPose(); }

vector<VRTransformPtr> VRRobotArm::getParts() { return parts; }

bool VRRobotArm::canReach(PosePtr p, bool local) {
    if (!local) {
        auto root = dynamic_pointer_cast<VRTransform>( parts[0]->getParent() );
        auto wpI = root->getWorldPose();
        wpI->invert();
        p = p->multLeft(wpI);
    }

    auto angles = calcReverseKinematics(p);
    auto p2 = calcForwardKinematics(angles);
    float D = p2->pos().dist( p->pos() );
    if (D > 1e-3) return false;

    auto d = p->dir();
    auto d2 = p2->dir();
    d.normalize();
    d2.normalize();
    D = d2.dot(d);
    if (D < 1.0-1e-3) return false;

    return true;
}

void VRRobotArm::moveTo(PosePtr p2, bool local) {
    stop();
    auto p1 = getPose();
    if (!p1) return;

    if (!local) {
        auto root = dynamic_pointer_cast<VRTransform>( parts[0]->getParent() );
        auto wpI = root->getWorldPose();
        wpI->invert();
        p2 = p2->multLeft(wpI);
    }
    //p1->setUp(-p1->up());
    //p1->setUp(Vec3d(0,1,0));

    cout << "VRRobotArm::moveTo " << p1->toString() << "   ->   " << p2->toString() << endl;

    animPath->clear();
    animPath->addPoint( *p1 );
    animPath->addPoint( *p2 );
    animPath->compute(2);

    /*cout << "moveTo, p1: " << p1->toString() << endl;
    cout << "moveTo, p2: " << p2->toString() << endl;
    cout << "path pos:";  for (auto v : animPath->getPositions())  cout << " " << v; cout << endl;
    cout << "path dirs:"; for (auto v : animPath->getDirections()) cout << " " << v; cout << endl;
    cout << "path ups:"; for (auto v : animPath->getUpVectors()) cout << " " << v; cout << endl;*/


    //addJob( job(animPath, 0, 1, 2*animPath->getLength()) ); // TODO
    addJob( job(animPath, 0, 0, 1, 2*animPath->getLength()/animSpeed, false) );
}

void VRRobotArm::setGrab(float g) {
    grabDist = g;
    float l = lengths[4]*g;
    Vec3d p; p[0] = l;
    if (parts.size() >= 9) {
        if (parts[7] && parts[8]) {
            parts[7]->setFrom( p);
            parts[8]->setFrom(-p);
        }
    }
}

void VRRobotArm::moveOnPath(float t0, float t1, bool loop, float durationMultiplier) {
    moveTo( robotPath->getPose(t0) );
    addJob( job(robotPath, orientationPath, t0, t1, 2*robotPath->getLength() * durationMultiplier, loop) );
}

void VRRobotArm::toggleGrab() { setGrab(1-grabDist); }

void VRRobotArm::setPath(PathPtr p, PathPtr po) { robotPath = p; orientationPath = po; }
PathPtr VRRobotArm::getPath() { return robotPath; }
PathPtr VRRobotArm::getOrientationPath() { return orientationPath; }

