#include "pose.h"
#include "core/utils/isNan.h"
#include <OpenSG/OSGMatrixUtility.h>

using namespace OSG;

pose::pose() { set(Vec3f(), Vec3f(0,0,-1), Vec3f(0,1,0)); }
pose::pose(const pose& p) { *this = p; }
pose::pose(Vec3f p, Vec3f d, Vec3f u) { set(p,d,u); }
pose::pose(const Matrix& m) {
    if (isNan(m)) return;
    //float s1 = m[0].length();
    float s2 = m[1].length();
    float s3 = m[2].length();
    set(Vec3f(m[3]), Vec3f(-m[2])*1.0/s3, Vec3f(m[1])*1.0/s2);
}

posePtr pose::create() { return posePtr( new pose() ); }
posePtr pose::create(const Matrix& m) { return posePtr( new pose(m) ); }
posePtr pose::create(const pose& p) { return posePtr( new pose(p) ); }

posePtr pose::create(Vec3f p, Vec3f d, Vec3f u) {
    return posePtr( new pose(p,d,u) );
}

void pose::set(Vec3f p, Vec3f d, Vec3f u) {
    data.resize(3);
    data[0] = p;
    data[1] = d;
    data[2] = u;
}

void pose::setPos(Vec3f p) { data[0] = p; }
void pose::setDir(Vec3f d) { data[1] = d; }
void pose::setUp(Vec3f u) { data[2] = u; }

Vec3f pose::pos() const { return data.size() > 0 ? data[0] : Vec3f(); }
Vec3f pose::dir() const { return data.size() > 1 ? data[1] : Vec3f(); }
Vec3f pose::up() const { return data.size() > 2 ? data[2] : Vec3f(); }
Vec3f pose::x() const { return data.size() > 2 ? data[1].cross(data[2]) : Vec3f(); }

Matrix pose::asMatrix() const {
    Matrix m;
    MatrixLookAt(m, data[0], data[0]+data[1], data[2]);
    return m;
}

string pose::toString() {
    stringstream ss;
    ss << "pose " << data[0] << " : " << data[1] << " : " << data[2];
    return ss.str();
}

void pose::invert() {
    Matrix m;
    MatrixLookAt(m, data[0], data[0]+data[1], data[2]);
    m.invert();
    set(Vec3f(m[3]), Vec3f(-m[2]), Vec3f(m[1]));
}





