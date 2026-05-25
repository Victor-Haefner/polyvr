#include "frustum.h"
#include "core/utils/toString.h"
#include "core/math/pose.h"
#include "core/math/polygon.h"

using namespace OSG;

Frustum::Frustum() {
    trans = Pose::create();
    profile = VRPolygon::create();
}

Frustum::~Frustum() {}

FrustumPtr Frustum::create() { return FrustumPtr(new Frustum()); }

void Frustum::runTest() {
    Frustum f;

    f.setPose(Pose::create());
    f.setNearFar(Vec2d(0.1,10));
    f.addEdge(Vec3d(-1,0,-1));
    f.addEdge(Vec3d(0,1,-1));
    f.addEdge(Vec3d(1,0,-1));
    f.addEdge(Vec3d(0.1,-0.5,-1));
    f.addEdge(Vec3d(0,-1,-1));
    f.close();

    auto res = f.getConvexDecomposition();
    for (auto f : res) cout << "convex " << f->toString() << endl;
}

bool Frustum::inFrustumPlanes(Vec3f p) {
    auto planes = getPlanes();
    for (unsigned int i=0; i<planes.size(); i++) {
        float d = planes[i].distance(p);
        if ( d < 0 ) return false;
    }
    return true;
}

bool Frustum::isInside(Vec3d p) {
    Vec3f pf(p);
    if (inFrustumPlanes(pf)) return true;

    auto convex_hull = getConvexHull();
    if (!convex_hull->inFrustumPlanes(pf)) return false;
    auto convex_decomposition = getConvexDecomposition();
    for (auto f : convex_decomposition ) if (f->inFrustumPlanes(pf)) return true;
    return false;
}

void Frustum::setPose(PosePtr trans) { this->trans = trans; }
PosePtr Frustum::getPose() { return trans; }
void Frustum::addEdge(Vec3d dir) { directions.push_back(dir); }
void Frustum::setNearFar(Vec2d nf) { near_far = nf; }
void Frustum::close() { if (directions.size() > 0) directions.push_back(directions[0]); }
int Frustum::size() { return directions.size(); }
void Frustum::clear() { directions.clear(); profile->clear(); }
vector< Vec3d > Frustum::getEdges() { return directions; }

void Frustum::computeProfile() {
    profile->clear();
    Matrix4d m = trans->asMatrix();
    m.invert();

    for (auto d : directions) {
        Vec3d p;
        m.mult(d,p);
        profile->addPoint( Vec2d(p[0], p[1]) );
    }

    if (!profile->isCCW()) profile->reverseOrder();
}

FrustumPtr Frustum::fromProfile(VRPolygonPtr p, PosePtr t) {
    auto res = Frustum::create();
    res->setPose(t);
    auto prof3D = p->toSpace( t->asMatrix() );
    for (auto p : prof3D) res->addEdge( p );
    return res;
}

vector<Plane> Frustum::getPlanes() {
    vector<Plane> res;
    for (unsigned int i=1; i<directions.size(); i++) {
        Vec3d e1 = directions[i-1];
        Vec3d e2 = directions[i];
        Vec3d n = e2.cross( e1 );
        n.normalize();
        Plane p = Plane(Vec3f(n), Pnt3f(trans->pos()) );
        res.push_back(p);
    }
    return res;
}

vector<Plane> Frustum::getNearFarPlanes() {
    vector<Plane> res;
    res.push_back( Plane(Vec3f(trans->pos() + trans->dir()*near_far[0]), Pnt3f(-trans->dir())) ); // Near plane
    res.push_back( Plane(Vec3f(trans->pos() + trans->dir()*near_far[1]), Pnt3f( trans->dir())) ); // far planse
    return res;
}

FrustumPtr Frustum::getConvexHull() {
    computeProfile();
    auto cnvx = profile->getConvexHull();
    auto res = fromProfile(cnvx, trans);
    res->convex = true;
    return res;
}

vector< FrustumPtr > Frustum::getConvexDecomposition() {
    computeProfile();
    auto cnvxs = profile->getConvexDecomposition();
    vector< FrustumPtr > res;

    for (auto cnvx : cnvxs) {
        FrustumPtr f = fromProfile(cnvx, trans);
        f->convex = true;
        res.push_back(f);
    }

    return res;
}

string Frustum::toString() {
    stringstream ss;
    ss << "Frustum: ";
    for (auto p : directions) ss << p << " : ";
    return ss.str();
}




