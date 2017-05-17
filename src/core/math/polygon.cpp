#include "polygon.h"
#include <deque>
#include <list>

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/partition_2.h>
#include <CGAL/ch_graham_andrew.h>

typedef CGAL::Exact_predicates_inexact_constructions_kernel Kernel;
typedef CGAL::Partition_traits_2<Kernel> CGALTraits;
typedef CGALTraits::Point_2 CGALPoint;
typedef CGALTraits::Polygon_2 CGALPolygon;
typedef std::list<CGALPolygon> CGALPolyList;

using namespace OSG;

CGALPolygon toCGALPolygon(polygon p) {
    vector<CGALPoint> pnts;
    for (auto v : p.get()) pnts.push_back(CGALPoint(v[0], v[1]));
    return CGALPolygon( &pnts[0], &pnts[pnts.size()-1] );
}

polygon fromCGALPolygon(CGALPolygon cp) {
    polygon p;
    for (auto itr = cp.vertices_begin(); itr != cp.vertices_end(); itr++) {
        CGALPoint k = *itr;
    //for (int i=0; i<cp.size(); i++) {
        //CGALPoint k = cp[i];
        p.addPoint( Vec2f(k[0], k[1]) );
    }
    return p;
}

polygon::polygon() {}

bool polygon::isCCW() {
    float s = 0;
    auto tmp = points;
    if (!closed && tmp.size() > 0) tmp.push_back(tmp[0]);
    for (uint i=1; i<tmp.size(); i++) {
        auto v1 = tmp[i-1];
        auto v2 = tmp[i];
        s += (v2[0]-v1[0])*(v2[1]+v1[1]);
    }
    return (s <= 0);
}

void polygon::runTest() {
    polygon poly;

    /*poly.addPoint(Vec2f(-1,0));
    poly.addPoint(Vec2f(0,1));
    poly.addPoint(Vec2f(1,0));
    poly.addPoint(Vec2f(0.1,-0.5));
    poly.addPoint(Vec2f(0,-1));*/

    /*poly.addPoint(Vec2f(-0.00383841,0.0929055));
    poly.addPoint(Vec2f(-0.053288,0.0701582));
    poly.addPoint(Vec2f(-0.0117584,0.014947));
    poly.addPoint(Vec2f(-0.0658535,0.0389837));
    poly.addPoint(Vec2f(-0.152454,0.0274887));
    poly.addPoint(Vec2f(-0.108552,0.066948));
    poly.addPoint(Vec2f(-0.137846,0.109314));*/

    poly.addPoint(Vec2f(-0.137846,0.109314));
    poly.addPoint(Vec2f(-0.108552,0.066948));
    poly.addPoint(Vec2f(-0.152454,0.0274887));
    poly.addPoint(Vec2f(-0.0658535,0.0389837));
    poly.addPoint(Vec2f(-0.0117584,0.014947));
    poly.addPoint(Vec2f(-0.053288,0.0701582));
    poly.addPoint(Vec2f(-0.00383841,0.0929055));

    poly.close();

    //auto hull = poly.getConvexHull();
    //cout << hull.toString() << endl;

    auto res = poly.getConvexDecomposition();
    for (auto poly : res) cout << "convex " << poly.toString() << endl;
}

void polygon::addPoint(Vec2f p) { if (!closed) points.push_back(p); }
void polygon::addPoint(Vec3f p) { if (!closed) points3.push_back(p); }
Vec2f polygon::getPoint(int i) { return points[i]; }
Vec3f polygon::getPoint3(int i) { return points3[i]; }
int polygon::size() { return max( points.size(), points3.size() ); }
void polygon::set(vector<Vec2f> vec) { for (auto v : vec) addPoint(v); }

std::shared_ptr<polygon> polygon::create() { return std::shared_ptr<polygon>( new polygon() ); }

void polygon::clear() {
    points.clear();
    points3.clear();
    closed = false;
    convex = false;
}

void polygon::close() {
    if (closed) return;
    closed = true;
    if (points.size() > 0) points.push_back(points[0]);
    if (points3.size() > 0) points3.push_back(points3[0]);
}

bool polygon::isInside(Vec2f p) {
    if (points.size() <= 1) return false;
    // cast ray in +x from p and intersect all lines
    int K = 0;
    int N = points.size();
    for (int i=0; i<N; i++) {
        Vec2f p1 = points[i];
        Vec2f d = points[(i+1)%N] - p1;
        float a = (p[1] - p1[1]) / d[1];
        if (a < 0 || a > 1) continue;
        float b = p1[0] - p[0] + d[0]*a;
        if (b < 0) continue;
        K += 1;
    }
    return (K%2 == 1);
}

polygon polygon::sort() {
    if (points.size() == 0) return polygon();
    Vec2f p0 = points[0]; // rightmost lowest point
    for (uint i=0; i<points.size(); i++) {
        Vec2f p = points[i];
        if ( p[1] < p0[1] ) p0 = p;
        if ( p[1] == p0[1] && p[0] > p0[0]) p0 = p;
    }

    // sort fan
    polygon radial_sort;
    radial_sort.addPoint(p0);
    for (auto p : points) if (p != p0) radial_sort.addPoint(p);

    auto getSortTurn = [&](const Vec2f& p1, const Vec2f& p2) -> bool {
        return (p1[1]-p0[1])*(p2[0]-p0[0]) < (p1[0]-p0[0])*(p2[1]-p0[1]);
    };

    auto& vec = radial_sort.points;
    //std::sort(vec.begin(), vec.end(), getSortTurn);
    std::stable_sort(vec.begin(), vec.end(), getSortTurn);
    std::unique(vec.begin(), vec.end());
    return radial_sort;
}

vector<Vec2f> polygon::get() { return points; }
vector<Vec3f> polygon::get3() { return points3; }

polygon polygon::getConvexHull() { // graham scan algorithm TODO: TOO FUCKING UNRELIABLE!!!
    /*auto radial_sort = sort();
    if (radial_sort.size() < 3) return polygon();
    //cout << " polygon::getConvexHull points " << toString() << endl;
    //cout << " polygon::getConvexHull sort " << radial_sort.toString() << endl;

    auto getTurn = [](Vec2f p0, Vec2f p1, Vec2f p2) -> float {
        return (p1[0]-p0[0])*(p2[1]-p0[1])-(p1[1]-p0[1])*(p2[0]-p0[0]);
    };

    deque<Vec2f> omega;
    auto getOmegaSecond = [&]() -> Vec2f {
        auto top = omega.back(); omega.pop_back();
        auto sec = omega.back(); omega.push_back(top);
        return sec;
    };

    omega.push_back(radial_sort.getPoint(0));
    omega.push_back(radial_sort.getPoint(1));
    for (int i=2; i < radial_sort.size(); ) {
        Vec2f Pi = radial_sort.getPoint(i);
        Vec2f PT1 = omega.back();
        //cout << " ch Pi " << i << " p " << Pi << endl;
        //cout << " ch PT1 " << i << " p " << PT1 << endl;
        if ( PT1 == radial_sort.getPoint(0) ) {
                //cout << " ch push " << i << " p " << Pi << endl;
                omega.push_back( Pi ); i++;
        }
        Vec2f PT2 = getOmegaSecond();
        float t = getTurn(PT2, PT1, Pi);
        //cout << " ch PT2 " << i << " p " << PT2 << " t " << t << endl;
        if (t > 0) {
            //cout << " ch push " << i << " p " << Pi << endl;
            omega.push_back( Pi ); i++; }
        else {
            //cout << " ch pop " << i << " p " << omega.back() << endl;
            omega.pop_back();
        }
    }

    polygon res;
    res.convex = true;
    for (auto p : omega) res.addPoint(p);
    //cout << " polygon::getConvexHull res " << res.toString() << endl;
    return res;*/

    /*CGAL::set_ascii_mode(std::cin);
    CGAL::set_ascii_mode(std::cout);
    std::istream_iterator< Point_2 >  in_start( std::cin );
    std::istream_iterator< Point_2 >  in_end;
    std::ostream_iterator< Point_2 >  out( std::cout, "\n" );*/
    vector<Kernel::Point_2> pIn; for (auto p : points) pIn.push_back(Kernel::Point_2(p[0],p[1]));
    vector<Kernel::Point_2> pOut; for (auto p : points) pOut.push_back(Kernel::Point_2());
    auto pOutEnd = CGAL::ch_graham_andrew( pIn.begin(), pIn.end(), pOut.begin() );
    polygon res;
    for (auto pItr = pOut.begin(); pItr != pOutEnd; pItr++) {
        auto p = *pItr;
        res.addPoint(Vec2f(p[0],p[2]));
    }
    return res;
}

float polygon::getTurn(Vec2f p0, Vec2f p1, Vec2f p2) {
    return (p1[0]-p0[0])*(p2[1]-p0[1])-(p1[1]-p0[1])*(p2[0]-p0[0]);
}

bool polygon::isConvex() {
    if (size() <= 3) return true;

    for (int i=2; i<size(); i++) {
        if (getTurn(points[i-2], points[i-1], points[i]) >= 0) return false;
    }
    return true;
}

void polygon::turn() {
    reverse(points.begin(), points.end());
    reverse(points3.begin(), points3.end());
}

vector< polygon > polygon::getConvexDecomposition() {
    vector< polygon > res;

    if (isConvex()) { // allready convex?
        res.push_back(*this);
        return res;
    }

    CGALPolygon cgalpoly;
    if (!isCCW()) turn();
    cgalpoly = toCGALPolygon(*this);
    CGALPolyList partitions;
    CGALTraits traits;
    CGAL::optimal_convex_partition_2(cgalpoly.vertices_begin(), cgalpoly.vertices_end(), std::back_inserter(partitions), traits);
    for (auto p : partitions) res.push_back( fromCGALPolygon(p) );

    return res;
}

vector<Vec3f> polygon::toSpace(Matrix m) {
    vector<Vec3f> res;
    for (auto p : points) {
        Vec3f pp = Vec3f(p[0], p[1], -sqrt(1-(p[0]*p[0]+p[1]*p[1])));
        m.mult(pp,pp);
        res.push_back(pp);
    }
    return res;
}

string polygon::toString() {
    stringstream ss;
    ss << "poly: \n ";
    for (auto p : points) ss << p << " \n ";
    return ss.str();
}




