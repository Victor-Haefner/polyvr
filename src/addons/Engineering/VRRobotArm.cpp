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
    ageo = VRAnalyticGeometry::create();
    ageo->setLabelParams(0.05, 1, 1, Color4f(1,1,1,1), Color4f(1,1,1,0));
    ageo->hide("SHADOW");

    animPtr = VRFunction<float>::create("animOnPath", bind(&VRRobotArm::animOnPath, this, placeholders::_1 ) );
    anim->setUnownedCallback(animPtr);

    updatePtr = VRUpdateCb::create("run engines", bind(&VRRobotArm::update, this) );
    VRScene::getCurrent()->addUpdateFkt(updatePtr, 999);
}

VRRobotArm::~VRRobotArm() {}

shared_ptr<VRRobotArm> VRRobotArm::create(string type) { return shared_ptr<VRRobotArm>(new VRRobotArm(type)); }

void VRRobotArm::setParts(vector<VRTransformPtr> parts) {
    this->parts.clear();
    for (auto p : parts) if (p) this->parts.push_back(p);
    ageo->switchParent(parts[0]->getParent());
}

void VRRobotArm::setEventCallback(VRMessageCbPtr mCb) { eventCb = mCb; }

void VRRobotArm::setAngleOffsets(vector<float> offsets) { angle_offsets = offsets; }
void VRRobotArm::setAngleDirections(vector<int> dirs) { angle_directions = dirs; }
void VRRobotArm::setAxis(vector<int> axis) { this->axis = axis; }
void VRRobotArm::setLengths(vector<float> lengths) { this->lengths = lengths; }
vector<float> VRRobotArm::getAngles() { return angles; }

double clamp(double f, double a = -1, double b = 1) { return f<a ? a : f>b ? b : f; }

void VRRobotArm::applyAngles() {
    for (int i=0; i<N; i++) {
        Vec3d euler;
        euler[axis[i]] = angles[i];
        parts[i]->setEuler(euler);
    }
}

double VRRobotArm::convertAngle(double a, int i) {
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
    auto ee = parts[parts.size()-1];
    obj->drag(ee);
    dragged = obj;
    cout << "VRRobotArm::grab obj " << obj << ", ee " << ee << endl;
    for (auto e : parts) cout << "  part " << e << endl;
}

void VRRobotArm::drop() {
    if (dragged) dragged->drop();
    dragged = 0;
}

/*

Analytic Kinematics Model

Bodies:
- body 0 -> upper base, rotates (Y) above the lower base
- body 1 -> first beam
- body 2 -> elbow part
- body 3 -> second beam,
- body 4 -> tool interface, last robot part without tool
- body 5 -> tool base
- body 6 -> tool finger 1
- body 7 -> tool finger 2

Angles:
- a[0] -> angle between lower and upper base parts, main Y rotation of the robot
- a[1] -> angle between upper base and first beam
- a[2] -> angle between first beam and elbow part
- a[3] -> angle between elbow part and second beam
- a[4] -> angle between second beam and tool interface

Lengths:
- l[0] -> height of robot base
- l[1] -> distance between joint A, B
- l[2] -> distance between joint B, C
- l[3] -> padding of end effector

Computation Parameters:
- L -> length from robot base joint to end effector
- r1/r2 -> lengths 1/2, see above
- b, a, f -> angle_targets
- e0, e1 -> elbow/wrist joint axis directions

*/

// TODO: accept world coordinates and convert to robot coordinates
void VRRobotArm::calcReverseKinematics(PosePtr p) {
    if (type == "kuka") calcReverseKinematicsKuka(p);
    else if (type == "aubo") calcReverseKinematicsAubo(p);
    else calcReverseKinematicsKuka(p); // default
    //applyAngles();
    lastPose = p;
}

void VRRobotArm::calcReverseKinematicsKuka(PosePtr p) {
    Vec3d pos = p->pos();
    Vec3d dir = p->dir();
    Vec3d up  = p->up();
    pos -= dir* lengths[3];

    pos[1] -= lengths[0];
    float r1 = lengths[1];
    float r2 = lengths[2];
    float L = pos.length();
    float b = acos( clamp( (L*L-r1*r1-r2*r2)/(-2*r1*r2) ) );
    angle_targets[2] = -b + Pi;

    float a = asin( clamp( r2*sin(b)/L ) ) + asin( clamp( pos[1]/L ) );
	angle_targets[1] = a - Pi*0.5;
	//angle_targets[1] = a;

    float f = pos[2] > 0 ? atan(pos[0]/pos[2]) : Pi - atan(-pos[0]/pos[2]);
    angle_targets[0] = f;

    // end effector
    float e = a+b; // counter angle
    Vec3d e0 = Vec3d(cos(-f),0,sin(-f));
    Vec3d av = Vec3d(-cos(e)*sin(f), -sin(e), -cos(e)*cos(f));
    Vec3d e1 = dir.cross(av);
    e1.normalize();

    float det = av.dot( e1.cross(e0) );
    e = min( max(-e1.dot(e0), -1.0), 1.0);
    angle_targets[3] = det < 0 ? -acos(e) : acos(e);
    angle_targets[4] = acos( av.dot(dir) );


    // analytics visualization ---------------------------------------------------------
    if (!showModel) return;
    float sA = 0.05;
    Vec3d pJ0 = Vec3d(0,lengths[0],0); // base joint
    Vec3d pJ1 = pJ0 + Vec3d(cos(a)*sin(f), sin(a), cos(a)*cos(f)) * lengths[1]; // elbow joint
    Vec3d pJ2 = pJ1 - Vec3d(cos(a+b)*sin(f), sin(a+b), cos(a+b)*cos(f)) * lengths[2]; // wrist joint

    // EE
    ageo->setVector(0, Vec3d(), pJ2, Color3f(0.6,0.8,1), "");
    ageo->setVector(1, pJ2, dir*0.1, Color3f(0,0,1), "");
    ageo->setVector(2, pJ2, up*0.1, Color3f(1,0,0), "");

    // rot axis
    ageo->setVector(3, pJ0 - Vec3d(0,sA,0), Vec3d(0,2*sA,0), Color3f(1,1,0.5), "");
    ageo->setVector(4, pJ0 - e0*sA, e0*2*sA, Color3f(1,1,0.5), "");
    ageo->setVector(5, pJ1 - e0*sA, e0*2*sA, Color3f(1,1,0.5), "");
    ageo->setVector(6, pJ2 - e1*sA, e1*2*sA, Color3f(1,1,0.5), "");

    // beams
    ageo->setVector(7, Vec3d(), pJ0, Color3f(1,1,1), "l0");
    ageo->setVector(8, pJ0, pJ1-pJ0, Color3f(1,1,1), "r1");
    ageo->setVector(9, pJ1, pJ2-pJ1, Color3f(1,1,1), "r2");
}

void VRRobotArm::calcReverseKinematicsAubo(PosePtr p) {
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

    // main arm
    pos = P3;
    pos[1] -= lengths[0]; // substract base offset
    float r1 = lengths[1];
    float r2 = lengths[2];
    float L = pos.length();
    float b = acos( clamp( (L*L-r1*r1-r2*r2)/(-2*r1*r2) ) );
    angle_targets[2] = -b + Pi;

    float a = asin( clamp( r2*sin(b)/L ) ) + asin( clamp( pos[1]/L ) );
	angle_targets[1] = a - Pi*0.5;
	//angle_targets[1] = a;

    float f = pos[2] > 0 ? atan(pos[0]/pos[2]) : Pi - atan(-pos[0]/pos[2]);
    angle_targets[0] = f;

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
        return a*boost::math::sign(k);
    };

    angle_targets[3] = getAngle(P1-P2, pJ1-pJ2, P3-P2) - Pi*0.5;
    angle_targets[4] = getAngle(dir, P3-P2, P2-P1) - Pi*0.5;


    // analytics visualization ---------------------------------------------------------
    if (!showModel) return;
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

void VRRobotArm::showAnalytics(bool b) { showModel = b; ageo->setVisible(b); }

void VRRobotArm::animOnPath(float t) {
    if (job_queue.size() == 0) { anim->stop(); return; }
    auto job = job_queue.front();
    anim->setDuration(job.d);

    t += job.t0;
    if (t >= job.t1 && !job.loop) { job_queue.pop_front(); anim->start(0); return; }
    if (t >= job.t1 && job.loop) { anim->start(0); return; }

    auto pose = job.p->getPose(t);
    if (job.po) {
        auto poseO = job.po->getPose(t);
        pose->set(pose->pos(), poseO->dir(), poseO->up());
    }
    calcReverseKinematics(pose);
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

void VRRobotArm::setMaxSpeed(float s) { maxSpeed = s; }

PosePtr VRRobotArm::getKukaPose() {
    if (parts.size() < 7) return 0;
    auto pose = parts[6]->getWorldPose();
    pose->setDir(-pose->dir());

    float r1 = lengths[1];
    float r2 = lengths[2];
    float b = Pi-angles[2];
    float a = angles[1] + Pi*0.5;
    float f = angles[0];
    float L = sqrt( (-2*r1*r2) * cos(b) + r1*r1 + r2*r2);

    float d = a - asin(r2/L*sin(b));
    Vec3d p( L*cos(d)*sin(f), L*sin(d), L*cos(d)*cos(f));
    p[1] += lengths[0]; // base
    //ageo->setVector(4, Vec3d(0,0,0), p, Color3f(0,0,0), "p");
    p += pose->dir()*lengths[3];

    pose->setPos(p);
    return pose;
}

PosePtr VRRobotArm::getAuboPose() {
    return lastPose;
    /*if (parts.size() < 5) return 0;
    auto pose = parts[4]->getWorldPose();
    pose->setDir(-pose->dir());
    Vec3d p = pose->pos() + pose->dir()*lengths[3];
    pose->setPos(p);
    return pose;*/
}

PosePtr VRRobotArm::getPose() {
    if (type == "kuka") return getKukaPose();
    if (type == "aubo") return getAuboPose();
    return getKukaPose(); // default
}

vector<VRTransformPtr> VRRobotArm::getParts() { return parts; }

void VRRobotArm::moveTo(PosePtr p2) {
    stop();
    auto p1 = getPose();
    if (!p1) return;

    //p1->setUp(-p1->up());
    //p1->setUp(Vec3d(0,1,0));

    //cout << "VRRobotArm::moveTo " << p1->toString() << "   ->   " << p2->toString() << endl;

    animPath->clear();
    animPath->addPoint( *p1 );
    animPath->addPoint( *p2 );
    animPath->compute(2);

    //addJob( job(animPath, 0, 1, 2*animPath->getLength()) ); // TODO
    addJob( job(animPath) );
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

