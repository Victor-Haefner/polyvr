#include "pose.h"
#include "core/utils/isNan.h"
#include "core/utils/toString.h"
#include "core/objects/VRTransform.h"

using namespace OSG;

template<> string typeName(const posePtr& p) { return "Pose"; }

pose::pose() { set(Vec3d(), Vec3d(0,0,-1), Vec3d(0,1,0)); }
pose::pose(const pose& p) { *this = p; }
pose::pose(Vec3d p, Vec3d d, Vec3d u) { set(p,d,u); }
pose::pose(const Matrix4d& m) {
    if (isNan(m)) return;
    //float s1 = m[0].length();
    float s2 = m[1].length();
    float s3 = m[2].length();
    set(Vec3d(m[3]), Vec3d(-m[2])*1.0/s3, Vec3d(m[1])*1.0/s2);
}

posePtr pose::create() { return posePtr( new pose() ); }
posePtr pose::create(const Matrix4d& m) { return posePtr( new pose(m) ); }
posePtr pose::create(const pose& p) { return posePtr( new pose(p) ); }

posePtr pose::create(Vec3d p, Vec3d d, Vec3d u) {
    return posePtr( new pose(p,d,u) );
}

void pose::set(Vec3d p, Vec3d d, Vec3d u) {
    data.resize(3);
    data[0] = p;
    data[1] = d;
    data[2] = u;
}

void pose::setPos(Vec3d p) { data[0] = p; }
void pose::setDir(Vec3d d) { data[1] = d; }
void pose::setUp(Vec3d u) { data[2] = u; }

Vec3d pose::pos() const { return data.size() > 0 ? data[0] : Vec3d(); }
Vec3d pose::dir() const { return data.size() > 1 ? data[1] : Vec3d(); }
Vec3d pose::up() const { return data.size() > 2 ? data[2] : Vec3d(); }
Vec3d pose::x() const { return data.size() > 2 ? data[1].cross(data[2]) : Vec3d(); }

Matrix4d pose::asMatrix() const {
    Matrix4d m;
    MatrixLookAt(m, data[0], data[0]+data[1], data[2]);
    return m;
}

string pose::toString() {
    stringstream ss;
    ss << "pose " << data[0] << " : " << data[1] << " : " << data[2];
    return ss.str();
}

void pose::invert() {
    Matrix4d m;
    MatrixLookAt(m, data[0], data[0]+data[1], data[2]);
    m.invert();
    set(Vec3d(m[3]), Vec3d(-m[2]), Vec3d(m[1]));
}





