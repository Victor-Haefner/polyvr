#include "frustum.h"

using namespace OSG;

frustum::frustum() {}

void frustum::runTest() {
    frustum f;

    f.setPose( pose(Vec3f(0,0,0), Vec3f(0,0,-1), Vec3f(0,1,0)) );
    f.setNearFar(Vec2f(0.1,10));
    f.addEdge(Vec3f(-1,0,-1));
    f.addEdge(Vec3f(0,1,-1));
    f.addEdge(Vec3f(1,0,-1));
    f.addEdge(Vec3f(0.1,-0.5,-1));
    f.addEdge(Vec3f(0,-1,-1));
    f.close();

    auto res = f.getConvexDecomposition();
    for (auto f : res) cout << "convex " << f.toString() << endl;
}

void frustum::setPose(pose trans) { this->trans = trans; }
void frustum::addEdge(Vec3f dir) { directions.push_back(dir); }
void frustum::setNearFar(Vec2f nf) { near_far = nf; }
void frustum::close() { directions.push_back(directions[0]); }
int frustum::size() { return directions.size(); }
void frustum::clear() { directions.clear(); profile.clear(); }

void frustum::computeProfile() {
    profile.clear();
    Matrix m = trans.asMatrix();
    m.invert();

    for (auto d : directions) {
        Vec3f p;
        m.mult(d,p);
        profile.addPoint( Vec2f(p[0], p[1]) );
    }
}

vector<Plane> frustum::getPlanes() {
    vector<Plane> res;
    res.push_back( Plane(trans.pos() + trans.dir()*near_far[0], trans.dir()) ); // Near plane
    res.push_back( Plane(trans.pos() + trans.dir()*near_far[1], -trans.dir()) ); // far planse
    for (int i=1; i<directions.size(); i++) {
        Vec3f e1 = directions[i-1];
        Vec3f e2 = directions[i];
        Vec3f n = e2.cross( e1 );
        n.normalize();
        Plane p = Plane(n, trans.pos() );
        res.push_back(p);
    }
    return res;
}

frustum frustum::fromProfile(polygon p, pose t) {
    frustum res;
    res.setPose(t);
    auto prof3D = p.toSpace( t.asMatrix() );
    for (auto p : prof3D) res.addEdge( p - t.pos() );
    return res;
}

frustum frustum::getConvexHull() {
    computeProfile();
    polygon cnvx = profile.getConvexHull();
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




