#include "VRRobotArm.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/math/path.h"
#include "core/objects/VRAnimation.h"
#include "core/utils/VRFunction.h"
#include "core/tools/VRAnalyticGeometry.h"


using namespace OSG;

VRRobotArm::VRRobotArm() {
    angles.resize(N,0);
    animPath = new path();
    robotPath = new path();
    anim = new VRAnimation("animOnPath");
    ageo = new VRAnalyticGeometry();
    ageo->setLabelSize(0.03);

    auto fkt = new VRFunction<float>("animOnPath", boost::bind(&VRRobotArm::animOnPath, this, _1 ) );
    anim->setSimpleCallback(fkt, 1);
}

void VRRobotArm::setParts(vector<VRTransform*> parts) {
    this->parts = parts;
    ageo->switchParent(parts[0]->getParent());
}

void VRRobotArm::setAngleOffsets(vector<float> offsets) { angle_offsets = offsets; }
void VRRobotArm::setAngleDirections(vector<int> dirs) { angle_directions = dirs; }
void VRRobotArm::setAxis(vector<int> axis) { this->axis = axis; }
void VRRobotArm::setLengths(vector<float> lengths) { this->lengths = lengths; }
vector<float> VRRobotArm::getAngles() { return angles; }

void VRRobotArm::applyAngles() {
    for (int i=0; i<N; i++) {
        Vec3f euler;
        euler[axis[i]] = angle_directions[i]*angles[i] + angle_offsets[i]*Pi;
        parts[i]->setEuler(euler);
    }
}

void VRRobotArm::calcReverseKinematics(Vec3f pos, Vec3f dir, Vec3f up) {
    pos -= dir* lengths[3];

    pos[1] -= lengths[0];
    float r1 = lengths[1];
    float r2 = lengths[2];
    float L = pos.length();
    float b = acos((L*L-r1*r1-r2*r2)/(-2*r1*r2));
    angles[2] = -b + Pi;

    float a = asin(r2*sin(b)/L) + asin(pos[1]/L);
	angles[1] = a - Pi*0.5;

    float f = pos[2] > 0 ? atan(pos[0]/pos[2]) : Pi - atan(-pos[0]/pos[2]);
    angles[0] = f;

    float e = a+b; // counter angle
    Vec3f e0 = Vec3f(cos(-f),0,sin(-f));
    Vec3f av = Vec3f(-cos(e)*sin(f), -sin(e), -cos(e)*cos(f));
    Vec3f e1 = dir.cross(av);
    e1.normalize();

    float det = av.dot( e1.cross(e0) );
    e = min( max(-e1.dot(e0), -1.f), 1.f);
    angles[3] = det < 0 ? -acos(e) : acos(e);
    angles[4] = acos( av.dot(dir) );

    ageo->setVector(0, pos, dir, Vec3f(0,1,0), "dir");
    ageo->setVector(1, Vec3f(0,0.6,0), e0, Vec3f(1,1,0), "e0");
    ageo->setVector(2, pos, av, Vec3f(1,0,0), "av");
    ageo->setVector(3, pos, e1, Vec3f(0,1,1), "e1");
}

void VRRobotArm::animOnPath(float t) {
    if (job_queue.size() == 0) { anim->stop(); return; }
    auto job = job_queue.front();
    anim->setDuration(job.d);

    t += job.t0;
    if (t >= job.t1 and !job.loop) { job_queue.pop_front(); anim->start(); return; }
    if (t >= job.t1 and job.loop) { anim->start(); return; }

    Vec3f pos, dir, up;
    job.p->getOrientation(t, dir, up);
    pos = job.p->getPosition(t);
    calcReverseKinematics(pos, dir, up);
    applyAngles();
}

void VRRobotArm::addJob(job j) {
    job_queue.push_back(j);
    if (!anim->isActive()) anim->start();
}

void VRRobotArm::setAngles(vector<float> angles) {
    this->angles = angles;
    applyAngles(); // TODO: animate from current pose
}

void VRRobotArm::getPose(Vec3f& pos, Vec3f& dir, Vec3f& up) {
    dir = parts[5]->getWorldDirection();
    up = parts[5]->getWorldUp();

    float r1 = lengths[1];
    float r2 = lengths[2];
    float b = Pi-angles[2];
    float a = angles[1] + Pi*0.5;
    float f = angles[0];
    float L = sqrt( (-2*r1*r2) * cos(b) + r1*r1 + r2*r2);

    float d = a - asin(r2/L*sin(b));
    Vec3f p( L*cos(d)*sin(f), L*sin(d), L*cos(d)*cos(f));
    p[1] += lengths[0]; // base
    ageo->setVector(4, Vec3f(0,0,0), p, Vec3f(0,0,0), "p");
    p += dir*lengths[3];
    pos = p;
}

void VRRobotArm::moveTo(Vec3f pos, Vec3f dir, Vec3f up) {
    Vec3f cpos, cdir, cup;
    getPose(cpos, cdir, cup);

    animPath->clear();
    animPath->addPoint(cpos, cdir, Vec3f(0,0,0), cup);
    animPath->addPoint(pos, dir, Vec3f(0,0,0), up);
    animPath->compute(2);

    addJob( job(animPath) );
}

void VRRobotArm::setGrab(float g) {
    float l = lengths[4]*g;
    Vec3f p; p[0] = l;
    parts[5]->setFrom(p);
    parts[6]->setFrom(-p);
    grab = g;
}

void VRRobotArm::moveOnPath(float t0, float t1, bool loop) {
    Vec3f p,d,u;
    p = robotPath->getPosition(t0);
    robotPath->getOrientation(t0,d,u);
    moveTo(p,d,u);
    addJob( job(robotPath, t0, t1, robotPath->size(), loop) );
}

void VRRobotArm::toggleGrab() { setGrab(1-grab); }

void VRRobotArm::setPath(path* p) { robotPath = p; }
path* VRRobotArm::getPath() { return robotPath; }
