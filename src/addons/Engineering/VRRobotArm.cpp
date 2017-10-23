#include "VRRobotArm.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/math/path.h"
#include "core/objects/VRAnimation.h"
#include "core/utils/VRFunction.h"
#include "core/tools/VRAnalyticGeometry.h"
#include <boost/bind.hpp>

using namespace OSG;

VRRobotArm::VRRobotArm() {
    angles.resize(N,0);
    animPath = path::create();
    robotPath = path::create();
    anim = VRAnimation::create("animOnPath");
    ageo = VRAnalyticGeometry::create();
    ageo->setLabelParams(0.03);

    animPtr = VRFunction<float>::create("animOnPath", boost::bind(&VRRobotArm::animOnPath, this, _1 ) );
    anim->setUnownedCallback(animPtr);
}

VRRobotArm::~VRRobotArm() {}

shared_ptr<VRRobotArm> VRRobotArm::create() { return shared_ptr<VRRobotArm>(new VRRobotArm()); }

void VRRobotArm::setParts(vector<VRTransformPtr> parts) {
    this->parts = parts;
    ageo->switchParent(parts[0]->getParent());
}

void VRRobotArm::setAngleOffsets(vector<float> offsets) { angle_offsets = offsets; }
void VRRobotArm::setAngleDirections(vector<int> dirs) { angle_directions = dirs; }
void VRRobotArm::setAxis(vector<int> axis) { this->axis = axis; }
void VRRobotArm::setLengths(vector<float> lengths) { this->lengths = lengths; }
vector<float> VRRobotArm::getAngles() { return angles; }

void VRRobotArm::applyAngles() {
    //cout << "applyAngles";
    for (int i=0; i<N; i++) {
        Vec3d euler;
        euler[axis[i]] = angle_directions[i]*angles[i] + angle_offsets[i]*Pi;
        //cout << " " << euler[axis[i]];
        parts[i]->setEuler(euler);
    }
    //cout << endl;
}

float clamp(float f) { return f<-1 ? -1 : f>1 ? 1 : f; }

void VRRobotArm::calcReverseKinematics(Vec3d pos, Vec3d dir, Vec3d up) {
    pos -= dir* lengths[3];

    pos[1] -= lengths[0];
    float r1 = lengths[1];
    float r2 = lengths[2];
    float L = pos.length();
    float b = acos( clamp( (L*L-r1*r1-r2*r2)/(-2*r1*r2) ) );
    angles[2] = -b + Pi;

    float a = asin( clamp( r2*sin(b)/L ) ) + asin( clamp( pos[1]/L ) );
	angles[1] = a - Pi*0.5;
	//angles[1] = a;

    float f = pos[2] > 0 ? atan(pos[0]/pos[2]) : Pi - atan(-pos[0]/pos[2]);
    angles[0] = f;

    // end effector
    float e = a+b; // counter angle
    Vec3d e0 = Vec3d(cos(-f),0,sin(-f));
    Vec3d av = Vec3d(-cos(e)*sin(f), -sin(e), -cos(e)*cos(f));
    Vec3d e1 = dir.cross(av);
    e1.normalize();

    float det = av.dot( e1.cross(e0) );
    e = min( max(-e1.dot(e0), -1.0), 1.0);
    angles[3] = det < 0 ? -acos(e) : acos(e);
    angles[4] = acos( av.dot(dir) );

    // vector visualization
    ageo->setVector(0, pos, dir, Color3f(0,1,0), "dir");
    ageo->setVector(4, pos, up, Color3f(0,0,1), "up");
    ageo->setVector(1, Vec3d(0,0.6,0), e0, Color3f(1,1,0), "e0");
    ageo->setVector(2, pos, av, Color3f(1,0,0), "av");
    ageo->setVector(3, pos, e1, Color3f(0,1,1), "e1");
}

void VRRobotArm::animOnPath(float t) {
    if (job_queue.size() == 0) { anim->stop(); return; }
    auto job = job_queue.front();
    anim->setDuration(job.d);

    t += job.t0;
    if (t >= job.t1 and !job.loop) { job_queue.pop_front(); anim->start(0); return; }
    if (t >= job.t1 and job.loop) { anim->start(0); return; }

    Vec3d pos, dir, up;
    job.p->getOrientation(t, dir, up);
    pos = job.p->getPosition(t);
    calcReverseKinematics(pos, dir, up);
    applyAngles();
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

void VRRobotArm::setAngles(vector<float> angles) {
    this->angles = angles;
    applyAngles(); // TODO: animate from current pose
}

PosePtr VRRobotArm::getPose() {
    Vec3d dir = -parts[5]->getWorldDirection();
    Vec3d up = parts[5]->getWorldUp();

    float r1 = lengths[1];
    float r2 = lengths[2];
    float b = Pi-angles[2];
    float a = angles[1] + Pi*0.5;
    float f = angles[0];
    float L = sqrt( (-2*r1*r2) * cos(b) + r1*r1 + r2*r2);

    float d = a - asin(r2/L*sin(b));
    Vec3d p( L*cos(d)*sin(f), L*sin(d), L*cos(d)*cos(f));
    p[1] += lengths[0]; // base
    ageo->setVector(4, Vec3d(0,0,0), p, Color3f(0,0,0), "p");
    p += dir*lengths[3];

    return Pose::create(p, dir, up);
}

void VRRobotArm::moveTo(PosePtr p2) {
    stop();
    auto p1 = getPose();

    //p1->setUp(-p1->up());
    p1->setUp(Vec3d(0,1,0));

    //cout << "VRRobotArm::moveTo " << p1->toString() << "   ->   " << p2->toString() << endl;

    animPath->clear();
    animPath->addPoint( *p1 );
    animPath->addPoint( *p2 );
    animPath->compute(2);

    addJob( job(animPath) );
}

void VRRobotArm::setGrab(float g) {
    float l = lengths[4]*g;
    Vec3d p; p[0] = l;
    parts[5]->setFrom(p);
    parts[6]->setFrom(-p);
    grab = g;
}

void VRRobotArm::moveOnPath(float t0, float t1, bool loop) {
    moveTo( robotPath->getPose(t0) );
    addJob( job(robotPath, t0, t1, robotPath->size(), loop) );
}

void VRRobotArm::toggleGrab() { setGrab(1-grab); }

void VRRobotArm::setPath(pathPtr p) { robotPath = p; }
pathPtr VRRobotArm::getPath() { return robotPath; }

bool VRRobotArm::isMoving(){return anim->isActive(); }
