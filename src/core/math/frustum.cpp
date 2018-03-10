#include "frustum.h"

using namespace OSG;

frustum::frustum() {}

void frustum::runTest() {
    frustum f;

    f.setPose(Pose());
    f.setNearFar(Vec2d(0.1,10));
    f.addEdge(Vec3d(-1,0,-1));
    f.addEdge(Vec3d(0,1,-1));
    f.addEdge(Vec3d(1,0,-1));
    f.addEdge(Vec3d(0.1,-0.5,-1));
    f.addEdge(Vec3d(0,-1,-1));
    f.close();

    auto res = f.getConvexDecomposition();
    for (auto f : res) cout << "convex " << f.toString() << endl;
}

void frustum::setPose(Pose trans) { this->trans = trans; }
Pose frustum::getPose() { return trans; }
void frustum::addEdge(Vec3d dir) { directions.push_back(dir); }
void frustum::setNearFar(Vec2d nf) { near_far = nf; }
void frustum::close() { if (directions.size() > 0) directions.push_back(directions[0]); }
int frustum::size() { return directions.size(); }
void frustum::clear() { directions.clear(); profile.clear(); }
vector< Vec3d > frustum::getEdges() { return directions; }

void frustum::computeProfile() {
    profile.clear();
    Matrix4d m = trans.asMatrix();
    m.invert();

    for (auto d : directions) {
        Vec3d p;
        m.mult(d,p);
        profile.addPoint( Vec2d(p[0], p[1]) );
    }

    if (!profile.isCCW()) profile.reverseOrder();
}

frustum frustum::fromProfile(VRPolygon p, Pose t) {
    frustum res;
    res.setPose(t);
    auto prof3D = p.toSpace( t.asMatrix() );
    for (auto p : prof3D) res.addEdge( p );
    return res;
}

vector<Plane> frustum::getPlanes() {
    vector<Plane> res;
    for (uint i=1; i<directions.size(); i++) {
        Vec3d e1 = directions[i-1];
        Vec3d e2 = directions[i];
        Vec3d n = e2.cross( e1 );
        n.normalize();
        Plane p = Plane(Vec3f(n), Pnt3f(trans.pos()) );
        res.push_back(p);
    }
    return res;
}

vector<Plane> frustum::getNearFarPlanes() {
    vector<Plane> res;
    res.push_back( Plane(Vec3f(trans.pos() + trans.dir()*near_far[0]), Pnt3f(-trans.dir())) ); // Near plane
    res.push_back( Plane(Vec3f(trans.pos() + trans.dir()*near_far[1]), Pnt3f(trans.dir())) ); // far planse
    return res;
}

frustum frustum::getConvexHull() {
    computeProfile();
    VRPolygon cnvx = profile.getConvexHull();
    frustum res = fromProfile(cnvx, trans);
    res.convex = true;
    return res;
}

vector< frustum > frustum::getConvexDecomposition() {
    computeProfile();
    auto cnvxs = profile.getConvexDecomposition();
    vector< frustum > res;

    for (auto cnvx : cnvxs) {
        frustum f = fromProfile(cnvx, trans);
        f.convex = true;
        res.push_back(f);
    }

    return res;
}

string frustum::toString() {
    stringstream ss;
    ss << "frustum: ";
    for (auto p : directions) ss << p << " : ";
    return ss.str();
}




