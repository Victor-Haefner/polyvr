#include "pose.h"
#include "core/utils/isNan.h"
#include "core/utils/toString.h"
#include "core/objects/VRTransform.h"

#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGMatrix.h>

using namespace OSG;

Pose::Pose() { set(Vec3d(), Vec3d(0,0,-1), Vec3d(0,1,0), Vec3d(1,1,1)); }
Pose::Pose(const Pose& p) { *this = p; }
Pose::Pose(const Vec3d& p, const Vec3d& d, const Vec3d& u, const Vec3d& s) { set(p,d,u,s); }

Pose::Pose(const Matrix4d& m) {
    if (isNan(m)) return;
    float s1 = m[0].length();
    float s2 = m[1].length();
    float s3 = m[2].length();
    set(Vec3d(m[3]), Vec3d(-m[2])*1.0/s3, Vec3d(m[1])*1.0/s2, Vec3d(s1,s2,s3));
}

Pose::Pose(const Matrix& m) {
    if (isNan(m)) return;
    float s1 = m[0].length();
    float s2 = m[1].length();
    float s3 = m[2].length();
    set(Vec3d(m[3]), Vec3d(-m[2])*1.0/s3, Vec3d(m[1])*1.0/s2, Vec3d(s1,s2,s3));
}

PosePtr Pose::create() { return PosePtr( new Pose() ); }
PosePtr Pose::create(const Matrix4d& m) { return PosePtr( new Pose(m) ); }
PosePtr Pose::create(const Pose& p) { return PosePtr( new Pose(p) ); }

PosePtr Pose::create(const Vec3d& p, const Vec3d& d, const Vec3d& u, const Vec3d& s) {
    return PosePtr( new Pose(p,d,u,s) );
}

void Pose::set(const Vec3d& p, const Vec3d& d, const Vec3d& u, const Vec3d& s) {
    data.resize(4);
    data[0] = p;
    data[1] = d;
    data[2] = u;
    data[3] = s;
}

Vec3d Pose::transform(const Vec3d& p, bool doTranslate) {
    if (doTranslate) {
        Pnt3d P(p);
        asMatrix().mult(P,P);
        return Vec3d(P);
    } else {
        Vec3d P(p);
        asMatrix().mult(P,P);
        return P;
    }
}

Vec3d Pose::transformInv(const Vec3d& p) {
    Pnt3d P(p);
    auto m = asMatrix();
    m.invert();
    m.mult(P,P);
    return Vec3d(P);
}

PosePtr Pose::multLeft(PosePtr p) { auto m1 = asMatrix(); auto m2 = p->asMatrix(); m1.multLeft(m2); return Pose::create(m1); }
PosePtr Pose::multRight(PosePtr p) { auto m1 = asMatrix(); auto m2 = p->asMatrix(); m1.mult(m2); return Pose::create(m1); }

void Pose::setPos(const Vec3d& p) { data[0] = p; }
void Pose::setDir(const Vec3d& d) { data[1] = d; }
void Pose::setUp(const Vec3d& u) { data[2] = u; }
void Pose::setScale(const Vec3d& s) { data.resize(4); data[3] = s; }

Vec3d Pose::pos() { return data.size() > 0 ? data[0] : Vec3d(); }
Vec3d Pose::dir() { return data.size() > 1 ? data[1] : Vec3d(); }
Vec3d Pose::up() { return data.size() > 2 ? data[2] : Vec3d(); }
Vec3d Pose::x() { return data.size() > 2 ? data[1].cross(data[2]) : Vec3d(); } // vector to the right
Vec3d Pose::scale() { return data.size() > 3 ? data[3] : Vec3d(1,1,1); }

void Pose::translate(const Vec3d& p) { data[0] += p; }
void Pose::move(double x) { data[0] += data[1]*x; }

void Pose::rotate(double a, const Vec3d& d) {
    Vec3d v = d;
    v.normalize();
    Quaterniond q(v, a);
    q.multVec(data[1],data[1]);
    q.multVec(data[2],data[2]);
}

Matrix4d Pose::asMatrix() const {
    Matrix4d m;
    if (data.size() > 2) MatrixLookAt(m, data[0], data[0]+data[1], data[2]);
    if (data.size() > 3) {
        Matrix4d ms;
        ms.setScale(data[3]);
        m.mult(ms);
    }
    return m;
}

string Pose::toString() {
    return ::toString(*this);
    /*stringstream ss;
    ss << "pose " << data[0] << " : " << data[1] << " : " << data[2];
    return ss.str();*/
}

void Pose::invert() {
    Matrix4d m = asMatrix();
    m.invert();
    //set(Vec3d(m[3]), Vec3d(-m[2]), Vec3d(m[1]));
    float s1 = m[0].length();
    float s2 = m[1].length();
    float s3 = m[2].length();
    set(Vec3d(m[3]), Vec3d(-m[2])*1.0/s3, Vec3d(m[1])*1.0/s2, Vec3d(s1,s2,s3));
}

bool Pose::operator == (const Pose& other) const {
    return asMatrix() == other.asMatrix();
}

bool Pose::operator != (const Pose& other) const {
    return asMatrix() != other.asMatrix();
}

void Pose::makeUpOrthogonal() {
    Vec3d X = x();
    Vec3d u = up();
    Vec3d d = dir();

    if (X.squareLength() > 1e-2) u = X.cross(d);
    else {
        if      (abs(d[2]) > abs(d[0]) && abs(d[2]) > abs(d[1])) u = Vec3d( d[0], d[2],-d[1]);
        else if (abs(d[1]) > abs(d[0]) && abs(d[1]) > abs(d[2])) u = Vec3d( d[0],-d[2], d[1]);
        else if (abs(d[0]) > abs(d[1]) && abs(d[0]) > abs(d[2])) u = Vec3d(-d[1], d[0], d[2]);
    }

    u.normalize();
    setUp(u);
}

void Pose::makeDirOrthogonal() {
    Vec3d X = x();
    Vec3d u = up();
    Vec3d d = dir();

    if (X.squareLength() > 1e-2) d = u.cross(X);
    else {
        if      (abs(u[2]) > abs(u[0]) && abs(u[2]) > abs(u[1])) d = Vec3d( u[0],-u[2], u[1]);
        else if (abs(u[1]) > abs(u[0]) && abs(u[1]) > abs(u[2])) d = Vec3d( u[0], u[2],-u[1]);
        else if (abs(u[0]) > abs(u[1]) && abs(u[0]) > abs(u[2])) d = Vec3d(-u[1], u[0], u[2]);
    }

    setDir(d);
}


