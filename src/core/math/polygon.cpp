#include "polygon.h"
#include <deque>

using namespace OSG;

polygon::polygon() {}

void polygon::runTest() {
    polygon poly;

    poly.addPoint(Vec2f(-1,0));
    poly.addPoint(Vec2f(0,1));
    poly.addPoint(Vec2f(1,0));
    poly.addPoint(Vec2f(0.1,-0.5));
    poly.addPoint(Vec2f(0,-1));
    poly.close();

    //auto hull = getConvexHull();
    //cout << hull.toString() << endl;

    auto res = poly.getConvexDecomposition();
    for (auto poly : res) cout << "convex " << poly.toString() << endl;
}

void polygon::addPoint(Vec2f p) { points.push_back(p); }
Vec2f polygon::getPoint(int i) { return points[i]; }
void polygon::close() { points.push_back(points[0]); }
int polygon::size() { return points.size(); }
void polygon::clear() { points.clear(); }

vector<Vec2f> polygon::sort() {
    Vec2f p0 = points[0]; // rightmost lowest point
    for (int i=0; i<points.size(); i++) {
        Vec2f p = points[i];
        if ( p[1] < p0[1] ) p0 = p;
        if ( p[1] == p0[1] && p[0] > p0[0]) p0 = p;
    }

    // sort fan
    vector<Vec2f> radial_sort;
    radial_sort.push_back(p0);
    for (auto p : points) if (p != p0) radial_sort.push_back(p);
    auto getSortTurn = [&](const Vec2f& p1, const Vec2f& p2) -> bool {
        float z = (p1[0]-p0[0])*(p2[1]-p0[1])-(p1[1]-p0[1])*(p2[0]-p0[0]);
        return z > 0;
    };
    std::sort(radial_sort.begin(), radial_sort.end(), getSortTurn);
    return radial_sort;
}

vector<Vec2f> polygon::get() { return points; }

polygon polygon::getConvexHull() { // graham scan algorithmus
    auto radial_sort = sort();

    auto getTurn = [](Vec2f p0, Vec2f p1, Vec2f p2) -> float {
        return (p1[0]-p0[0])*(p2[1]-p0[1])-(p1[1]-p0[1])*(p2[0]-p0[0]);
    };

    deque<Vec2f> omega;
    auto getOmegaSecond = [&]() -> Vec2f {
        auto top = omega.back(); omega.pop_back();
        auto sec = omega.back(); omega.push_back(top);
        return sec;
    };

    omega.push_back(radial_sort[0]);
    omega.push_back(radial_sort[1]);
    for (int i=2; i < radial_sort.size(); ) {
        Vec2f Pi = radial_sort[i];
        Vec2f PT1 = omega.back();
        if ( PT1 == radial_sort[0] ) { omega.push_back( Pi ); i++; }
        Vec2f PT2 = getOmegaSecond();
        float t = getTurn(PT2, PT1, Pi);
        if (t > 0) { omega.push_back( Pi ); i++; }
        else omega.pop_back();
    }

    polygon res;
    res.convex = true;
    for (auto p : omega) res.addPoint(p);
    return res;
}

vector< polygon > polygon::getConvexDecomposition() {
    vector< polygon > res;

    auto getTurn = [](Vec2f p0, Vec2f p1, Vec2f p2) -> float {
        return (p1[0]-p0[0])*(p2[1]-p0[1])-(p1[1]-p0[1])*(p2[0]-p0[0]);
    };

    polygon poly;
    for (Vec2f p : points) {
        int N = poly.size();
        if (N < 3) {
            poly.addPoint(p);
            continue;
        }

        Vec2f p0 = poly.getPoint(N-2);
        Vec2f p1 = poly.getPoint(N-1);

        if (getTurn(p0,p1,p) < 0) {
            poly.addPoint(p);
            continue;
        } else {
            poly.convex = true;
            res.push_back(poly);
            poly = polygon();
            poly.addPoint(p1);
            poly.addPoint(p);
        }
    }
    poly.convex = true;
    res.push_back(poly);

    return res;
}

vector<Vec3f> polygon::toSpace(Matrix m) {
    vector<Vec3f> res;
    for (auto p : points) {
        Vec3f pp = Vec3f(p[0], p[1], 0);
        m.mult(pp,pp);
        res.push_back(pp);
    }
    return res;
}

string polygon::toString() {
    stringstream ss;
    ss << "poly: ";
    for (auto p : points) ss << p << " : ";
    return ss.str();
}




