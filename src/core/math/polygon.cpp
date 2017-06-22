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

CGALPolygon toCGALPolygon(Polygon p) {
    vector<CGALPoint> pnts;
    for (auto v : p.get()) pnts.push_back(CGALPoint(v[0], v[1]));
    return CGALPolygon( &pnts[0], &pnts[pnts.size()-1] );
}

Polygon fromCGALPolygon(CGALPolygon cp) {
    Polygon p;
    for (auto itr = cp.vertices_begin(); itr != cp.vertices_end(); itr++) {
        CGALPoint k = *itr;
    //for (int i=0; i<cp.size(); i++) {
        //CGALPoint k = cp[i];
        p.addPoint( Vec2f(k[0], k[1]) );
    }
    return p;
}

Polygon::Polygon() {}

bool Polygon::isCCW() {
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

void Polygon::runTest() {
    Polygon poly;

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

void Polygon::addPoint(Vec2f p) { if (!closed) points.push_back(p); }
void Polygon::addPoint(Vec3f p) { if (!closed) points3.push_back(p); }
Vec2f Polygon::getPoint(int i) { return points[i]; }
Vec3f Polygon::getPoint3(int i) { return points3[i]; }
int Polygon::size() { return max( points.size(), points3.size() ); }
void Polygon::set(vector<Vec2f> vec) { for (auto v : vec) addPoint(v); }

std::shared_ptr<Polygon> Polygon::create() { return std::shared_ptr<Polygon>( new Polygon() ); }

void Polygon::translate(Vec3f v) {
    for (auto& p : points) p += Vec2f(v[0], v[2]);
    for (auto& p : points3) p += v;
}

void Polygon::clear() {
    points.clear();
    points3.clear();
    closed = false;
    convex = false;
}

PolygonPtr Polygon::shrink(float amount) {
    PolygonPtr area = Polygon::create();
    *area = *this;
    if (amount == 0) return area;

    for (uint i=0; i<area->points.size(); i++) {
        Vec2f& p1 = area->points[i];
        Vec2f& p2 = area->points[(i+1)%area->points.size()];
        Vec2f d = p2-p1;
        Vec2f n = Vec2f(-d[1], d[0]);
        n.normalize();
        p1 += n*amount;
        p2 += n*amount;
    }

    return area;
}

vector<Vec3f> Polygon::getRandomPoints(float density, float padding) {
    auto area = shrink(padding);
    vector<Vec3f> res;
    //for (auto& area : area1->getConvexDecomposition()) {
        auto bb = area->getBoundingBox();
        int N = density*area->computeArea();
        for (int i=0; i<N; i++) {
            Vec3f p;
            do p = bb.getRandomPoint();
            while( !area->isInside(Vec2f(p[0], p[2])) );
            res.push_back( p );
        }
    //}
    return res;
}

vector< PolygonPtr > Polygon::gridSplit(float G) {
    auto inSquare = [&](Vec2f p, Vec2i s) {
        if (p[0] < s[0]*G - 1e-6) return false;
        if (p[1] < s[1]*G - 1e-6) return false;
        if (p[0] > (s[0]+1)*G + 1e-6) return false;
        if (p[1] > (s[1]+1)*G + 1e-6) return false;
        return true;
    };

    auto squareToPolygon = [&](Vec2i s) {
        auto p = create();
        p->addPoint(Vec2f(s[0]  , s[1])*G);
        p->addPoint(Vec2f(s[0]+1, s[1])*G);
        p->addPoint(Vec2f(s[0]+1, s[1]+1)*G);
        p->addPoint(Vec2f(s[0]  , s[1]+1)*G);
        return p;
    };

    auto getBBGridPnts = [&](Boundingbox& bb) {
        map<int, map<int, Vec2i>> pnts;
        int i0 = floor( bb.min()[0] / G);
        int j0 = floor( bb.min()[2] / G);
        int i1 = ceil( bb.max()[0] / G);
        int j1 = ceil( bb.max()[2] / G);
        if (i0 == i1) i1++;
        if (j0 == j1) j1++;
        for (int j=j0; j<j1; j++) {
            for (int i=i0; i<i1; i++) {
                pnts[j][i] = Vec2i(i,j);
            }
        }
        return pnts;
    };

    auto getSegmentGridPnts = [&](Vec2f p1, Vec2f p2) {
        auto d = p2-p1;
        Boundingbox bb;
        bb.update(Vec3f(p1[0], 0, p1[1]));
        bb.update(Vec3f(p2[0], 0, p2[1]));

        map<float, Vec2f> iPnts;
        vector<float> X, Y;
        for (auto j : getBBGridPnts(bb)) { for (auto i : j.second) X.push_back(i.first*G); break; }
        for (auto j : getBBGridPnts(bb)) Y.push_back(j.first*G);

        if (abs(d[1]) > 0) {
            for (auto y : Y) { // intersection with vertical grid lines
                float t = (y - p1[1]) /d[1];
                if (t <= 0 || t >= 1) continue;
                iPnts[t] = p1 + d*t;
            }
        }

        if (abs(d[0]) > 0) {
            for (auto x : X) { // intersection with horizontal grid lines
                float t = (x - p1[0]) /d[0];
                if (t <= 0 || t >= 1) continue;
                iPnts[t] = p1 + d*t;
            }
        }

        vector<Vec2f> res;
        for (auto p : iPnts) res.push_back(p.second);
        return res;
    };

    auto isOnGrid = [&](Vec2f p) {
        p *= 1.0/G;
        if (abs(p[0] - round(p[0])) < 1e-6) return true;
        if (abs(p[1] - round(p[1])) < 1e-6) return true;
        return false;
    };

    auto samePoint = [&](Vec2f p1, Vec2f p2) {
        Vec2f d = p2-p1;
        if (abs(d[0]) > 1e-6) return false;
        if (abs(d[1]) > 1e-6) return false;
        return true;
    };

    vector<PolygonPtr> res;

    vector<Vec2i> squares; // defined by square corner left bottom, in grid lengths
    map<int, vector<int>> pointSquaresMap; // up to 4 squares when on a corner
    map<int, vector<int>> squarePointsMap;

    // compute intersections of polygon edges with grid and insert in copy of this polygon
    auto self = create();
    for (int i=0; i<size(); i++) {
        auto p1 = points[i];
        auto p2 = points[(i+1)%size()];
        //cout << " process segment " << p1 << "  to  " << p2 << endl;
        if (i == 0) {
            //cout << "  add segment pnt " << p1 << endl;
            self->addPoint(p1);
        }
        Vec2f pl = p1;
        for (auto p : getSegmentGridPnts(p1, p2) ) {
            if (!samePoint(p,p1) && !samePoint(p,p2) && !samePoint(p,pl)) {
                //cout << "  add segment grid pnt " << p << endl;
                self->addPoint(p);
                pl = p;
            }
        }
        if (i != size()-1) {
            //cout << "  add segment pnt " << p2 << endl;
            self->addPoint(p2);
        }
    }

    // get all grid squares touching the area bounding box
    auto bb = self->getBoundingBox();
    //cout << " poly BB min " << bb.min() << " max " << bb.max() << endl;
    for (auto j : getBBGridPnts(bb)) {
        for (auto i : j.second) {
            //cout << "  add square " << squares.size() << " at " << i.second << endl;
            squares.push_back(i.second);
        }
    }

    // get all grid squares touching each point
    for (int i=0; i<self->points.size(); i++) {
        auto point = self->points[i];
        for (int j=0; j<squares.size(); j++) {
            auto square = squares[j];
            if (inSquare(point, square)) pointSquaresMap[i].push_back(j);
            if (inSquare(point, square)) squarePointsMap[j].push_back(i);
        }
    }

    // get all grid squares fully inside of polygon
    for (int i=0; i<squares.size(); i++) {
        if (squarePointsMap.count(i)) continue; // intersects polygon, skip
        auto s = squares[i];
        if (self->isInside(Vec2f(s)*G)) { // at least one corner in area
            res.push_back( squareToPolygon(s) ); // add square to chunks
        }
    }

    map<int, Vec2f> cornerPoints;
    cornerPoints[-1] = Vec2f(0,0);
    cornerPoints[-2] = Vec2f(G,0);
    cornerPoints[-3] = Vec2f(0,G);
    cornerPoints[-4] = Vec2f(G,G);

    // get all grid squares partly in polygon
    //cout << "GridSplit N squares: " << squares.size() << endl;
    for (auto s : squarePointsMap) {
        //cout << "Square: " << s.first << " --------------------------" << endl;
        auto p = create();
        Vec2f square = Vec2f(squares[s.first]) * G;
        map<float, int> borderPnts; // key from -pi to pi

        auto compAngle = [&](Vec2f pnt) {
            float a = atan2(pnt[0]-square[0]-0.5*G, pnt[1]-square[1]-0.5*G); // angle
            //cout << "      compAngle p " << pnt << " a " << a << endl;
            return a;
        };

        auto getSquarePoint = [&](int i) {
            if (i == -5) { cout << "AAARGH a -5!" << endl; return Vec2f(); };
            if (i < 0) return cornerPoints[i] + square;
            else return self->getPoint(i);
        };

        auto getNextBorderPoint = [&](float t, int i_1, int i) {
            int p1 = -5;
            int p2 = -5;

            for(auto it = borderPnts.begin(); it != borderPnts.end(); it++) {
                if (abs(it->first-t) < 1e-6) { // found point
                    auto it1 = it;
                    auto it2 = it;
                    //cout << "borderPnts itrs " << (it == borderPnts.begin()) << " " << (it == borderPnts.end()) << " itt " << it->first << " t " << t << endl;
                    if (it == borderPnts.begin()) { it1 = borderPnts.end(); it1--; } else it1--;
                    it2++; if (it2 == borderPnts.end()) it2 = borderPnts.begin();
                    p1 = it1->second;
                    p2 = it2->second;
                    break;
                }
            }

            auto pnt1 = getSquarePoint(p1);
            auto pnt2 = getSquarePoint(p2);

            //cout << "   getNextBorderPoint t " << t << "  p1 " << p1 << "  p2 " << p2 << " p1Inside " << self->isInside(pnt1) << " p2Inside " << self->isInside(pnt2) << endl;
            if (self->isInside(pnt1) && i_1 != p1 && i != p1) {
                //cout << "    found p1 " << p1 << " p " << getSquarePoint(p1) << endl;
                return p1;
            }
            if (self->isInside(pnt2) && i_1 != p2 && i != p2) {
                //cout << "    found p2 " << p2 << " p " << getSquarePoint(p2) << endl;
                return p2;
            }
            return -5; // invalid pnt
        };

        auto getNextPoint = [&](int i_1, int i) {
            //cout << "   getNextPoint " << i_1 << " " << i << endl;
            int j = -5;
            auto pnt = getSquarePoint(i);
            auto pnt_1 = getSquarePoint(i_1);

            if (i != i_1) { // not the first point
                if (isOnGrid(pnt) && !isOnGrid(pnt_1)) return getNextBorderPoint( compAngle(pnt), i_1, i );
                if (isOnGrid(pnt) && isOnGrid(pnt_1)) {
                    for (auto k : s.second) {
                        if (j == i && k == i+1) {
                            //cout << "    found k " << k << " p " << getSquarePoint(k) << endl;
                            return k; // last point is the one searched!
                        }
                        j = k;
                    }
                    return getNextBorderPoint( compAngle(pnt), i_1, i );
                }
            }

            for (auto k : s.second) {
                if (j == i) {
                    //cout << "    found k " << k << " p " << getSquarePoint(k) << endl;
                    return k; // last point is the one searched!
                }
                j = k;
            }

            return getNextBorderPoint( compAngle(pnt), i_1, i );
        };

        for (auto i : s.second) {
            auto pnt = self->getPoint(i);
            if (isOnGrid(pnt)) borderPnts[ compAngle(pnt) ] = i;
        }

        // add corner points to borderpoints
        for (int i=-1; i>=-4; i--) {
            auto pnt = cornerPoints[i];
            //if (!self->isInside(pnt)) continue;
            float a = compAngle(pnt + square);
            bool skip = false;
            for (auto A : borderPnts) if (abs(A.first-a) < 1e-6) { skip = true; break; }
            if (skip) continue;
            borderPnts[ a ] = i;
        }

        int iMax = 0;
        int i0 = s.second[0]; // first point
        int i_1 = i0;
        int i = i0;
        do {
            auto pnt = getSquarePoint(i);
            p->addPoint(pnt);
            int newI = getNextPoint(i_1,i);
            if (newI == -5) break; // invalid point
            i_1 = i;
            i = newI;
            iMax++;
        } while (i != i0 && iMax < 10);

        float pA = p->computeArea();

        /*cout << " borderPnts" << endl;
        for (auto i : borderPnts) cout << "  t " << i.first << "  pi " << i.second << " p " << getSquarePoint(i.second) << endl;

        cout << " squarePnts" << endl;
        for (auto i : s.second) cout << "  pnt " << i << "  " << getSquarePoint(i) << endl;

        cout << " polygon A " << pA << endl;
        for (auto pnt : p->points) cout << "  pnt " << pnt << endl;*/

        if ( pA > 1e-6 && pA <= G*G+1e-6) res.push_back(p);
    }

    return res;
}

Vec3f Polygon::getRandomPoint() {
    auto bb = getBoundingBox();
    Vec3f p;
    do p = bb.getRandomPoint();
    while( !isInside(Vec2f(p[0], p[2])) );
    return p;
}

float Polygon::computeArea() {
    float area = 0;
    for (uint i=0; i<points.size(); i++) {
        Vec2f p1 = points[i];
        Vec2f p2 = points[(i+1)%points.size()];
        area += p1[0]*p2[1] - p1[1]*p2[0];
    }
    for (uint i=0; i<points3.size(); i++) {
        Vec3f p1 = points3[i];
        Vec3f p2 = points3[(i+1)%points3.size()];
        area += p1.cross(p2).length();
    }
    return 0.5*abs(area);
}

void Polygon::close() {
    if (closed) return;
    closed = true;
    if (points.size() > 0) points.push_back(points[0]);
    if (points3.size() > 0) points3.push_back(points3[0]);
}

bool Polygon::isInside(Vec2f p) {
    if (points.size() <= 1) return false;
    // cast ray from p and intersect all lines
    int K = 0;
    int N = points.size();
    Vec2f r = Vec2f(1.0/17, 1.0/29);
    r.normalize();
    float eps = 1e-6; // avoid problems at corners and edges

    for (int i=0; i<N; i++) {
        Vec2f p1 = points[i];
        Vec2f p2 = points[(i+1)%N];
        Vec2f d = p2 - p1;

        // check if on the line
        float cross = (p[1]-p1[1])*d[0] - (p[0]-p1[0])*d[1];
        if ( abs( cross ) < eps ) {
            float dot = (p-p1).dot(d);
            if (dot >= -eps && dot < d.squareLength()+eps) return true;
        }

        // lines defined as (a,b,c) where ax + by + c = 0
        Vec3f Us(d[1], -d[0], d[0]*p1[1] - d[1]*p1[0]);
        Vec3f Ur(r[1], -r[0], r[0]*p[1] - r[1]*p[0]);
        Vec3f I3 = Us.cross(Ur);
        if (abs(I3[2]) < eps) continue; // lines parallel ?
        Vec2f I(I3[0]/I3[2], I3[1]/I3[2]); // intersection of lines

        // check if intersecting segment
        float a = (I-p1).dot(d);
        if (a < eps || a > d.squareLength()-eps) continue;

        // check if point in front of p (dont intersect behind!)
        if ((I-p).dot(r) < 0) continue;

        /*if (d[1] == 0) continue;
        float a = (p[1] - p1[1]) / d[1];
        if (a < 0 || a >= 1) continue;
        float b = p1[0] - p[0] + d[0]*a;
        if (b < 0) continue;*/
        K += 1;
    }

    return (K%2 == 1);
}

Boundingbox Polygon::getBoundingBox() {
    Boundingbox bb;
    for (auto p : points) bb.update(Vec3f(p[0], 0, p[1]));
    for (auto p : points3) bb.update(p);
    return bb;
}

Polygon Polygon::sort() {
    if (points.size() == 0) return Polygon();
    Vec2f p0 = points[0]; // rightmost lowest point
    for (uint i=0; i<points.size(); i++) {
        Vec2f p = points[i];
        if ( p[1] < p0[1] ) p0 = p;
        if ( p[1] == p0[1] && p[0] > p0[0]) p0 = p;
    }

    // sort fan
    Polygon radial_sort;
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

vector<Vec2f> Polygon::get() { return points; }
vector<Vec3f> Polygon::get3() { return points3; }

Polygon Polygon::getConvexHull() { // graham scan algorithm TODO: TOO FUCKING UNRELIABLE!!!
    /*auto radial_sort = sort();
    if (radial_sort.size() < 3) return Polygon();
    //cout << " Polygon::getConvexHull points " << toString() << endl;
    //cout << " Polygon::getConvexHull sort " << radial_sort.toString() << endl;

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

    Polygon res;
    res.convex = true;
    for (auto p : omega) res.addPoint(p);
    //cout << " Polygon::getConvexHull res " << res.toString() << endl;
    return res;*/

    /*CGAL::set_ascii_mode(std::cin);
    CGAL::set_ascii_mode(std::cout);
    std::istream_iterator< Point_2 >  in_start( std::cin );
    std::istream_iterator< Point_2 >  in_end;
    std::ostream_iterator< Point_2 >  out( std::cout, "\n" );*/
    vector<Kernel::Point_2> pIn; for (auto p : points) pIn.push_back(Kernel::Point_2(p[0],p[1]));
    vector<Kernel::Point_2> pOut; for (auto p : points) pOut.push_back(Kernel::Point_2());
    auto pOutEnd = CGAL::ch_graham_andrew( pIn.begin(), pIn.end(), pOut.begin() );
    Polygon res;
    for (auto pItr = pOut.begin(); pItr != pOutEnd; pItr++) {
        auto p = *pItr;
        res.addPoint(Vec2f(p[0],p[2]));
    }
    return res;
}

float Polygon::getTurn(Vec2f p0, Vec2f p1, Vec2f p2) {
    return (p1[0]-p0[0])*(p2[1]-p0[1])-(p1[1]-p0[1])*(p2[0]-p0[0]);
}

bool Polygon::isConvex() {
    if (size() <= 3) return true;

    for (int i=2; i<size(); i++) {
        if (getTurn(points[i-2], points[i-1], points[i]) >= 0) return false;
    }
    return true;
}

void Polygon::turn() {
    reverse(points.begin(), points.end());
    reverse(points3.begin(), points3.end());
}

vector< Polygon > Polygon::getConvexDecomposition() {
    vector< Polygon > res;

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

vector<Vec3f> Polygon::toSpace(Matrix m) {
    vector<Vec3f> res;
    for (auto p : points) {
        Vec3f pp = Vec3f(p[0], p[1], -sqrt(1-(p[0]*p[0]+p[1]*p[1])));
        m.mult(pp,pp);
        res.push_back(pp);
    }
    return res;
}

string Polygon::toString() {
    stringstream ss;
    ss << "poly: \n ";
    for (auto p : points) ss << p << " \n ";
    return ss.str();
}




