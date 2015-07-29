#include "VRRobotArm.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/math/path.h"
#include "core/objects/VRAnimation.h"
#include "core/utils/VRFunction.h"


using namespace OSG;

VRRobotArm::VRRobotArm() {
    angles.resize(N,0);
    Path = new path();
    anim = new VRAnimation("animOnPath");

    auto fkt = new VRFunction<float>("animOnPath", boost::bind(&VRRobotArm::animOnPath, this, _1 ) );
    anim->setSimpleCallback(fkt, 1);
}

void VRRobotArm::setParts(vector<VRTransform*> parts) { this->parts = parts; }
void VRRobotArm::setAngleOffsets(vector<float> offsets) { angle_offsets = offsets; }
void VRRobotArm::setAxis(vector<int> axis) { this->axis = axis; }
void VRRobotArm::setLengths(vector<float> lengths) { this->lengths = lengths; }

void VRRobotArm::applyAngles() {
    for (int i=0; i<N; i++) {
        Vec3f euler;
        euler[axis[i]] = angles[i];
        parts[i]->setEuler(euler);
    }
}

void VRRobotArm::calcReverseKinematics(Vec3f pos, Vec3f dir) {
    pos += dir* lengths[3];

    pos[1] -= lengths[0];
    float r1 = lengths[1];
    float r2 = lengths[2];
    float L = pos.length();
    float b = acos((L*L-r1*r1-r2*r2)/(-2*r1*r2));
    angles[2] = b;

    float a = asin(r2*sin(b)/L) + asin(pos[1]/L) -Pi*0.5;
	angles[1] = a;

    float f = pos[0] > 0 ? atan(-pos[2]/pos[0]) : Pi - atan(pos[2]/pos[0]);
    angles[0] = f;

    float e = (Pi*0.5+(a+b)); // counter angle
    Vec3f e0 = Vec3f(sin(f),0,cos(f));
    Vec3f av = Vec3f(-cos(e)*cos(-f), -sin(e), -cos(e)*sin(-f));
    Vec3f e1 = dir.cross(av);
    e1.normalize();

    float det = av.dot( e1.cross(e0) );
    e = min( max(-e1.dot(e0), -1.f), 1.f);
    angles[3] = det < 0 ? acos(e) : -acos(e);

    angles[4] = acos( av.dot(dir) );
}

void VRRobotArm::setAngles(vector<float> angles) { this->angles = angles; applyAngles(); }
vector<float> VRRobotArm::getAngles() { return angles; }

void VRRobotArm::animOnPath(float t) {
    Vec3f dir, up;
    Path->getOrientation(t, dir, up);
    auto pos = Path->getPosition(t);

    calcReverseKinematics(pos, dir);
    applyAngles();
}

Vec3f VRRobotArm::getPosition() { return parts[4]->getWorldPosition(); }
Vec3f VRRobotArm::getDirection() { return parts[5]->getWorldDirection(); }

void VRRobotArm::moveTo(Vec3f pos, Vec3f dir) {
    Path->clear();

    if (Path->size() == 0) { // initial position
        Vec3f d = getDirection();
        Vec3f u = d[1] > 0.9 ? Vec3f(1,0,0) : Vec3f(0,1,0);
        Vec3f p = getPosition(); p += d*lengths[3];
        Path->addPoint(p, -d, Vec3f(0,0,0), u);
    }

    Vec3f up = dir[1] > 0.9 ? Vec3f(1,0,0) : Vec3f(0,1,0);
    Path->addPoint(pos, dir, Vec3f(0,0,0), up);
    Path->compute(2);
    anim->start();
}

void VRRobotArm::setGrab(float g) {
    float l = lengths[4]*g;
    Vec3f p; p[0] = l;
    parts[5]->setFrom(p);
    parts[6]->setFrom(-p);
    grab = g;
}

void VRRobotArm::toggleGrab() { setGrab(1-grab); }
