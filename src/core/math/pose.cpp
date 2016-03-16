#include "pose.h"
#include <OpenSG/OSGMatrixUtility.h>

using namespace OSG;

pose::pose() { set(Vec3f(), Vec3f(0,0,-1), Vec3f(0,1,0)); }
pose::pose(Vec3f p, Vec3f d, Vec3f u) { set(p,d,u); }

shared_ptr<pose> pose::create(Vec3f p, Vec3f d, Vec3f u) {
    return shared_ptr<pose>( new pose(p,d,u) );
}

void pose::set(Vec3f p, Vec3f d, Vec3f u) {
    data.resize(3);
    data[0] = p;
    data[1] = d;
    data[2] = u;
}

Vec3f pose::pos() { return data.size() > 0 ? data[0] : Vec3f(); }
Vec3f pose::dir() { return data.size() > 1 ? data[1] : Vec3f(); }
Vec3f pose::up() { return data.size() > 2 ? data[2] : Vec3f(); }

Matrix pose::asMatrix() {
    Matrix m;
    MatrixLookAt(m, data[0], data[0]+data[1], data[2]);
    return m;
}

string pose::toString() {
    stringstream ss;
    ss << "pose " << data[0] << " : " << data[1] << " : " << data[2];
    return ss.str();
}
