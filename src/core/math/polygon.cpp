#include "polygon.h"
#include "path.h"
#include "core/utils/toString.h"
#include <deque>
#include <list>

#ifndef WITHOUT_CGAL
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/partition_2.h>
#include <CGAL/ch_graham_andrew.h>

typedef CGAL::Exact_predicates_inexact_constructions_kernel Kernel;
typedef CGAL::Partition_traits_2<Kernel> CGALTraits;
typedef CGALTraits::Point_2 CGALPoint;
typedef CGALTraits::Polygon_2 CGALPolygon;
typedef std::list<CGALPolygon> CGALPolyList;
#endif

using namespace OSG;

template<> int toValue(stringstream& ss, VRPolygon& p) { return 0; }

#ifndef WITHOUT_CGAL
CGALPolygon toCGALVRPolygon(VRPolygon p) {
    vector<CGALPoint> pnts;
    for (auto v : p.get()) pnts.push_back(CGALPoint(v[0], v[1]));
    return CGALPolygon( &pnts[0], &pnts[pnts.size()-1] );
}

VRPolygon fromCGALPolygon(CGALPolygon cp) {
    VRPolygon p;
    for (auto itr = cp.vertices_begin(); itr != cp.vertices_end(); itr++) {
        CGALPoint k = *itr;
    //for (int i=0; i<cp.size(); i++) {
        //CGALPoint k = cp[i];
        p.addPoint( Vec2d(k[0], k[1]) );
    }
    return p;
}
#endif

VRPolygon::VRPolygon() {}

bool VRPolygon::isCCW() {
    double s = 0;
    auto tmp = points;
    if (!closed && tmp.size() > 0) tmp.push_back(tmp[0]);
    for (unsigned int i=1; i<tmp.size(); i++) {
        auto v1 = tmp[i-1];
        auto v2 = tmp[i];
        s += (v2[0]-v1[0])*(v2[1]+v1[1]);
    }
    return (s <= 0);
}

void VRPolygon::runTest() {
    VRPolygon poly;

    /*poly.addPoint(Vec2d(-1,0));
    poly.addPoint(Vec2d(0,1));
    poly.addPoint(Vec2d(1,0));
    poly.addPoint(Vec2d(0.1,-0.5));
    poly.addPoint(Vec2d(0,-1));*/

    /*poly.addPoint(Vec2d(-0.00383841,0.0929055));
    poly.addPoint(Vec2d(-0.053288,0.0701582));
    poly.addPoint(Vec2d(-0.0117584,0.014947));
    poly.addPoint(Vec2d(-0.0658535,0.0389837));
    poly.addPoint(Vec2d(-0.152454,0.0274887));
    poly.addPoint(Vec2d(-0.108552,0.066948));
    poly.addPoint(Vec2d(-0.137846,0.109314));*/

    poly.addPoint(Vec2d(-0.137846,0.109314));
    poly.addPoint(Vec2d(-0.108552,0.066948));
    poly.addPoint(Vec2d(-0.152454,0.0274887));
    poly.addPoint(Vec2d(-0.0658535,0.0389837));
    poly.addPoint(Vec2d(-0.0117584,0.014947));
    poly.addPoint(Vec2d(-0.053288,0.0701582));
    poly.addPoint(Vec2d(-0.00383841,0.0929055));

    poly.close();

    //auto hull = poly.getConvexHull();
    //cout << hull.toString() << endl;

    auto res = poly.getConvexDecomposition();
    for (auto poly : res) cout << "convex " << poly.toString() << endl;
}

void VRPolygon::addPoint(Vec2d p) { if (!closed) points.push_back(p); }
void VRPolygon::addPoint(Vec3d p) { if (!closed) points3.push_back(p); }
Vec2d VRPolygon::getPoint(int i) { return points[i]; }
Vec3d VRPolygon::getPoint3(int i) { return points3[i]; }
int VRPolygon::size() { return max( points.size(), points3.size() ); }
int VRPolygon::size2() { return points.size(); }
int VRPolygon::size3() { return points3.size(); }
void VRPolygon::set(vector<Vec2d> vec) { for (auto v : vec) addPoint(v); }

std::shared_ptr<VRPolygon> VRPolygon::create() { return std::shared_ptr<VRPolygon>( new VRPolygon() ); }

void VRPolygon::translate(Vec3d v) {
    for (auto& p : points) p += Vec2d(v[0], v[2]);
    for (auto& p : points3) p += v;
}

void VRPolygon::scale(Vec3d s) {
    for (auto& p : points) { p[0] *= s[0]; p[1] *= s[2]; }
    for (auto& p : points3) { p[0] *= s[0]; p[1] *= s[1]; p[2] *= s[2]; }
}

void VRPolygon::clear() {
    points.clear();
    points3.clear();
    closed = false;
    convex = false;
}

VRPolygonPtr VRPolygon::shrink(double amount) {
    VRPolygonPtr area = VRPolygon::create();
    *area = *this;
    if (amount == 0) return area;

    auto intersect = [](const Vec2d& p1, const Vec2d& n1, const Vec2d& p2, const Vec2d& n2) {
        auto cross = [](const Vec2d& p1, const Vec2d& p2) { return p1[0]*p2[1]-p1[1]*p2[0]; };
        double k = cross(n1,n2);
        if (k == 0) k = 1.0;
        double s = cross(p2-p1,n2)/k;
        return p1 + n1*s;
    };

    int N = area->points.size();
    vector<Vec2d> newPositions;
    for (int i=0; i<N; i++) {
        Vec2d& p1 = area->points[(i+N-1)%N];
        Vec2d& p2 = area->points[i];
        Vec2d& p3 = area->points[(i+1)%N];
        Vec2d d1 = p2-p1; d1.normalize();
        Vec2d d2 = p3-p2; d2.normalize();
        Vec2d n1 = Vec2d(-d1[1], d1[0]);
        Vec2d n2 = Vec2d(-d2[1], d2[0]);
        newPositions.push_back( intersect(p2+n1*amount, d1, p2+n2*amount, d2) );
    }

    for (int i=0; i<N; i++) area->points[i] = newPositions[i];

    return area;
}

void VRPolygon::remPoint(int i) { points.erase(points.begin() + i); }
void VRPolygon::remPoint3(int i) { points3.erase(points3.begin() + i); }

void VRPolygon::removeDoubles(float d) {
    float d2 = d*d;

    auto process = [&]() {
        for (unsigned int i=1; i<points.size(); i++) {
            auto p1 = points[i-1];
            auto p2 = points[i];
            if ((p2-p1).squareLength() < d2) { remPoint(i); return true; }
        }
        if (points.size() > 1) if ((points[points.size()-1]-points[0]).squareLength() < d2) { remPoint(points.size()-1); return true; }
        return false;
    };

    while (process()) {;}
}

vector<Vec3d> VRPolygon::getRandomPoints(double density, double padding, double spread) {
    vector<Vec3d> res;
    auto area = shrink(padding);
    if (!area) return res;

    auto bb = area->getBoundingBox();
    Vec3i gridN;
    Vec3d gridO = bb.min();
    Vec3d gridD;
    for (int i=0; i<3; i++) gridN[i] = ceil(bb.size()[i]*density)+1;
    for (int i=0; i<3; i++) gridD[i] = bb.size()[i]/(gridN[i]-1);

    for (int i=0; i<gridN[0]; i++) {
        for (int j=0; j<gridN[2]; j++) {
            Vec3d p = Vec3d(gridO[0] + i*gridD[0], 0, gridO[2] + j*gridD[2]);
            if (area->isInside(Vec2d(p[0], p[2]))) {
                p[0] += spread*gridD[0]*(float(rand())/RAND_MAX-0.5);
                p[2] += spread*gridD[0]*(float(rand())/RAND_MAX-0.5);
                res.push_back( p );
            }
        }
    }

    return res;
}

vector< VRPolygonPtr > VRPolygon::gridSplit(double G) {
    //cout << "VRPolygon::gridSplit " << G << endl;
    auto inSquare = [&](Vec2d p, Vec2i s) {
        const double eps = 1e-4;
        if (p[0] < s[0]*G - eps) return false;
        if (p[1] < s[1]*G - eps) return false;
        if (p[0] > (s[0]+1)*G + eps) return false;
        if (p[1] > (s[1]+1)*G + eps) return false;
        return true;
    };

    auto squareToVRPolygon = [&](Vec2i s) {
        auto p = create();
        p->addPoint(Vec2d(s[0]  , s[1])*G);
        p->addPoint(Vec2d(s[0]+1, s[1])*G);
        p->addPoint(Vec2d(s[0]+1, s[1]+1)*G);
        p->addPoint(Vec2d(s[0]  , s[1]+1)*G);
        if (p->isCCW()) p->reverseOrder();
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

    auto getSegmentGridPnts = [&](Vec2d p1, Vec2d p2) {
        auto d = p2-p1;
        Boundingbox bb;
        bb.update(Vec3d(p1[0], 0, p1[1]));
        bb.update(Vec3d(p2[0], 0, p2[1]));

        map<double, Vec2d> iPnts;
        vector<double> X, Y;
        for (auto j : getBBGridPnts(bb)) { for (auto i : j.second) X.push_back(i.first*G); break; }
        for (auto j : getBBGridPnts(bb)) Y.push_back(j.first*G);

        if (abs(d[1]) > 0) {
            for (auto y : Y) { // intersection with vertical grid lines
                double t = (y - p1[1]) /d[1];
                if (t <= 0 || t >= 1) continue;
                iPnts[t] = p1 + d*t;
            }
        }

        if (abs(d[0]) > 0) {
            for (auto x : X) { // intersection with horizontal grid lines
                double t = (x - p1[0]) /d[0];
                if (t <= 0 || t >= 1) continue;
                iPnts[t] = p1 + d*t;
            }
        }

        vector<Vec2d> res;
        for (auto p : iPnts) res.push_back(p.second);
        return res;
    };

    auto isOnGrid = [&](Vec2d p) {
        p *= 1.0/G;
        if (abs(p[0] - round(p[0])) < 1e-6) return true;
        if (abs(p[1] - round(p[1])) < 1e-6) return true;
        return false;
    };

    auto samePoint = [&](Vec2d p1, Vec2d p2) {
        Vec2d d = p2-p1;
        if (abs(d[0]) > 1e-6) return false;
        if (abs(d[1]) > 1e-6) return false;
        return true;
    };

    vector<VRPolygonPtr> res;

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
        Vec2d pl = p1;
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
    for (unsigned int i=0; i<self->points.size(); i++) {
        auto point = self->points[i];
        for (unsigned int j=0; j<squares.size(); j++) {
            auto square = squares[j];
            if (inSquare(point, square)) pointSquaresMap[i].push_back(j);
            if (inSquare(point, square)) squarePointsMap[j].push_back(i);
        }
    }

    // get all grid squares fully inside of polygon
    for (unsigned int i=0; i<squares.size(); i++) {
        if (squarePointsMap.count(i)) continue; // intersects polygon, skip
        auto s = Vec2d(squares[i]);
        if (self->areInside({s*G,s*G+Vec2d(G,0),s*G+Vec2d(G,G),s*G+Vec2d(0,G)})) { // at least one corner in area
            /*if (G == 10) cout << " isInside " << s*G << "  " << isInside(s*G) << endl;
            if (G == 10) cout << " isInside " << s*G+Vec2d(G,0) << "  " << isInside(s*G+Vec2d(G,0)) << endl;
            if (G == 10) cout << " isInside " << s*G+Vec2d(G,G) << "  " << isInside(s*G+Vec2d(G,G)) << endl;
            if (G == 10) cout << " isInside " << s*G+Vec2d(0,G) << "  " << isInside(s*G+Vec2d(0,G)) << endl;*/
            res.push_back( squareToVRPolygon(squares[i]) ); // add square to chunks
        }
    }

    map<int, Vec2d> cornerPoints;
    cornerPoints[-1] = Vec2d(0,0);
    cornerPoints[-2] = Vec2d(G,0);
    cornerPoints[-3] = Vec2d(0,G);
    cornerPoints[-4] = Vec2d(G,G);

    bool verbose = false;

    // get all grid squares partly in polygon
    if (verbose) cout << "GridSplit N squares: " << squares.size() << endl;
    for (auto sItr : squarePointsMap) {
        int squareID = sItr.first;
        vector<int>& squarePoints = sItr.second;
        if (verbose) cout << "Square: " << squareID << " --------------------------" << endl;
        Vec2d square = Vec2d(squares[squareID]) * G;
        map<double, int> borderPnts; // key from -pi to pi
        vector<int> processedSquarePoints;

        auto compAngle = [&](Vec2d pnt) {
            double a = atan2(pnt[0]-square[0]-0.5*G, pnt[1]-square[1]-0.5*G); // angle
            if (verbose) cout << "      compAngle p " << pnt << " a " << a << endl;
            return a;
        };

        auto getSquarePoint = [&](int i) {
            if (i == -5) { if (verbose) cout << "AAARGH a -5!" << endl; return Vec2d(); };
            if (i < 0) return cornerPoints[i] + square;
            else return self->getPoint(i);
        };

        auto getNextBorderPoint = [&](double t, int i_1, int i) {
            int p1 = -5;
            int p2 = -5;

            for(auto it = borderPnts.begin(); it != borderPnts.end(); it++) {
                if (abs(it->first-t) < 1e-6) { // found point
                    auto it1 = it;
                    auto it2 = it;
                    if (verbose) cout << "       borderPnts itrs " << (it == borderPnts.begin()) << " " << (it == borderPnts.end()) << " itt " << it->first << " t " << t << endl;
                    if (it == borderPnts.begin()) { it1 = borderPnts.end(); it1--; } else it1--;
                    it2++; if (it2 == borderPnts.end()) it2 = borderPnts.begin();
                    p1 = it1->second;
                    p2 = it2->second;
                    break;
                }
            }

            auto pnt1 = getSquarePoint(p1);
            auto pnt2 = getSquarePoint(p2);

            if (verbose) cout << "   getNextBorderPoint t " << t << "  p1 " << p1 << "  p2 " << p2 << " p1Inside " << self->isInside(pnt1) << " p2Inside " << self->isInside(pnt2) << endl;
            if (self->isInside(pnt1) && i_1 != p1 && i != p1) {
                if (verbose) cout << "    found p1 " << p1 << " p " << getSquarePoint(p1) << endl;
                processedSquarePoints.push_back(p1);
                return p1;
            }
            if (self->isInside(pnt2) && i_1 != p2 && i != p2) {
                if (verbose) cout << "    found p2 " << p2 << " p " << getSquarePoint(p2) << endl;
                processedSquarePoints.push_back(p2);
                return p2;
            }
            return -5; // invalid pnt
        };

        auto getNextPoint = [&](int i_1, int i) {
            if (verbose) cout << "   getNextPoint " << i_1 << " " << i << endl;
            int j = -5;
            auto pnt = getSquarePoint(i);
            auto pnt_1 = getSquarePoint(i_1);

            if ((i != i_1 && isOnGrid(pnt) && isOnGrid(pnt_1)) || i == i_1 || !isOnGrid(pnt)) { // not the first point
                for (auto k : squarePoints) {
                    if (j == i && k == i+1) {
                        if (verbose) cout << "    found k " << k << " p " << getSquarePoint(k) << endl;
                        processedSquarePoints.push_back(k);
                        return k; // last point is the one searched!
                    }
                    j = k;
                }
            }
            return getNextBorderPoint( compAngle(pnt), i_1, i );
        };

        for (auto i : squarePoints) {
            auto pnt = self->getPoint(i);
            if (isOnGrid(pnt)) borderPnts[ compAngle(pnt) ] = i;
        }

        // add corner points to borderpoints
        for (int i=-1; i>=-4; i--) {
            auto pnt = cornerPoints[i];
            //if (!self->isInside(pnt)) continue;
            double a = compAngle(pnt + square);
            bool skip = false;
            for (auto A : borderPnts) if (abs(A.first-a) < 1e-6) { skip = true; break; }
            if (skip) continue;
            borderPnts[ a ] = i;
        }

        auto getUnprocessedSquarePoint = [&]() {
            for (auto sp : squarePoints) {
                int p = sp;
                for (auto psp : processedSquarePoints) {
                    if (sp == psp) { p = -5; break; }
                }
                if (p != -5) return p;
            }
            return -5;
        };

        for (int i0 = squarePoints[0]; i0 != -5; i0 = getUnprocessedSquarePoint()) {
            processedSquarePoints.push_back(i0);
            auto p = create();
            int iMax = 0;
            int i_1 = i0;
            int i = i0;
            do {
                auto pnt = getSquarePoint(i);
                if (verbose) cout << "     add point " << i << endl;
                p->addPoint(pnt);
                int newI = getNextPoint(i_1,i);
                if (newI == -5) break; // invalid point
                i_1 = i;
                i = newI;
                iMax++;
            } while (i != i0 && iMax < 1000);

            if ( p->isCCW() != isCCW() ) p->reverseOrder();

            auto allEdgesInPolygon = [&]() {
                for (int i=0; i< p->size(); i++) { // check if all edges part of polygon
                    auto pMid = (p->getPoint(i) + p->getPoint((i+1)%p->size()))*0.5;
                    if (verbose) cout << " mid " << pMid << " " << isInside(pMid) << endl;
                    if (!isInside(pMid)) return false;
                }
                return true;
            };

            double pA = p->computeArea();

            if (verbose) {
                cout << " borderPnts" << endl;
                for (auto i : borderPnts) cout << "  t " << i.first << "  pi " << i.second << " p " << getSquarePoint(i.second) << endl;
                cout << " squarePnts" << endl;
                for (auto i : squarePoints) cout << "  pnt " << i << "  " << getSquarePoint(i) << endl;
                cout << " polygon A " << pA << endl;
                for (auto pnt : p->points) cout << "  pnt " << pnt << endl;
            }

            if ( pA <= 1e-6 && pA > G*G+1e-6 ) continue;
            if ( !allEdgesInPolygon() ) continue;
            if (p->size() <= 2) continue; // TODO
            res.push_back(p);
        }
    }

    return res;
}

Vec3d VRPolygon::getRandomPoint() {
    auto bb = getBoundingBox();
    Vec3d p;
    do p = bb.getRandomPoint();
    while( !isInside(Vec2d(p[0], p[2])) );
    return p;
}

double VRPolygon::computeArea() {
    double area = 0;
    for (unsigned int i=0; i<points.size(); i++) {
        Vec2d p1 = points[i];
        Vec2d p2 = points[(i+1)%points.size()];
        area += p1[0]*p2[1] - p1[1]*p2[0];
    }
    for (unsigned int i=0; i<points3.size(); i++) {
        Vec3d p1 = points3[i];
        Vec3d p2 = points3[(i+1)%points3.size()];
        area += p1.cross(p2).length();
    }
    return 0.5*abs(area);
}

void VRPolygon::close() {
    if (closed) return;
    closed = true;
    if (points.size() > 0) points.push_back(points[0]);
    if (points3.size() > 0) points3.push_back(points3[0]);
}

double isLeft( const Vec2d& P0, const Vec2d& P1, const Vec2d& P2 ){
    return ( (P1[0] - P0[0]) * (P2[1] - P0[1]) - (P2[0] -  P0[0]) * (P1[1] - P0[1]) );
};

bool onSegment( const Vec2d& p, const Vec2d& p1, const Vec2d& p2 ) {
    double A = p1[0]*(p2[1] - p[1]) + p2[0]*(p[1] - p1[1]) + p[0]*(p1[1] - p2[1]);
    if (abs(A) > 1e-6) return false; // check for collinear
    double d = (p2-p1).dot(p-p1);
    if (d < -1e-6) return false;
    if (d > (p2-p1).squareLength()+1e-6) return false;
    return true;
}

bool VRPolygon::isInside(Vec2d p) { // winding number algorithm
    int wn = 0;
    int N = points.size();
    for (int i=0; i<N; i++) {
        Vec2d p1 = points[i];
        Vec2d p2 = points[(i+1)%N];
        if (p1 == p2) continue;
        Vec2d d = p2 - p1;
        if (onSegment(p,p1,p2)) return true;
        if (p1[1] <= p[1]) {
            if (p2[1]  > p[1]) if (isLeft( p, p1, p2) > 0) ++wn;
        } else {
            if (p2[1]  <= p[1]) if (isLeft( p, p1, p2) < 0) --wn;
        }
    }

    if (N == 0) {
        N = points3.size();
        for (int i=0; i<N; i++) {
            Vec3d p13 = points3[i];
            Vec3d p23 = points3[(i+1)%N];
            Vec2d p1 = Vec2d(p13[0], p13[2]);
            Vec2d p2 = Vec2d(p23[0], p23[2]);
            if (p1 == p2) continue;
            Vec2d d = p2 - p1;
            if (onSegment(p,p1,p2)) return true;
            if (p1[1] <= p[1]) {
                if (p2[1]  > p[1]) if (isLeft( p, p1, p2) > 0) ++wn;
            } else {
                if (p2[1]  <= p[1]) if (isLeft( p, p1, p2) < 0) --wn;
            }
        }
    }

    return wn;
}

bool VRPolygon::areInside(vector<Vec2d> pv) {
    for (auto p : pv) if (!isInside(p)) return false;
    return true;
}

double squareDistToSegment(const Vec2d& p1, const Vec2d& p2, const Vec2d& p) {
    auto d = p2-p1;
    auto L2 = d.squareLength();
    auto t = -(p1-p).dot(d)/L2;
    auto ps = p1+d*t;
    if (t<0) ps = p1;
    if (t>1) ps = p2;
    return (ps-p).squareLength();
}

bool VRPolygon::isInside(Vec2d p, double& dist) { // winding number algorithm
    dist = 1e12;
    int wn = 0;
    int N = points.size();
    for (int i=0; i<N; i++) {
        Vec2d p1 = points[i];
        Vec2d p2 = points[(i+1)%N];
        Vec2d d = p2 - p1;
        if (p1[1] <= p[1]) {
            if (p2[1]  > p[1]) if (isLeft( p1, p2, p) > 0) ++wn;
        } else {
            if (p2[1]  <= p[1]) if (isLeft( p1, p2, p) < 0) --wn;
        }

        double D = squareDistToSegment(p1,p2,p);
        if (D < dist) dist = D;
    }
    dist = sqrt(dist);
    return wn;
}

Boundingbox VRPolygon::getBoundingBox() {
    Boundingbox bb;
    for (auto p : points) bb.update(Vec3d(p[0], 0, p[1]));
    for (auto p : points3) bb.update(p);
    return bb;
}

VRPolygon VRPolygon::sort() {
    if (points.size() == 0) return VRPolygon();
    Vec2d p0 = points[0]; // rightmost lowest point
    for (unsigned int i=0; i<points.size(); i++) {
        Vec2d p = points[i];
        if ( p[1] < p0[1] ) p0 = p;
        if ( p[1] == p0[1] && p[0] > p0[0]) p0 = p;
    }

    // sort fan
    VRPolygon radial_sort;
    radial_sort.addPoint(p0);
    for (auto p : points) if (p != p0) radial_sort.addPoint(p);

    auto getSortTurn = [&](const Vec2d& p1, const Vec2d& p2) -> bool {
        return (p1[1]-p0[1])*(p2[0]-p0[0]) < (p1[0]-p0[0])*(p2[1]-p0[1]);
    };

    auto& vec = radial_sort.points;
    //std::sort(vec.begin(), vec.end(), getSortTurn);
    std::stable_sort(vec.begin(), vec.end(), getSortTurn);
    std::unique(vec.begin(), vec.end());
    return radial_sort;
}

vector<Vec2d>& VRPolygon::get() { return points; }
vector<Vec3d>& VRPolygon::get3() { return points3; }

VRPolygon VRPolygon::getConvexHull() { // graham scan algorithm TODO: TOO FUCKING UNRELIABLE!!!
    /*auto radial_sort = sort();
    if (radial_sort.size() < 3) return VRPolygon();
    //cout << " VRPolygon::getConvexHull points " << toString() << endl;
    //cout << " VRPolygon::getConvexHull sort " << radial_sort.toString() << endl;

    auto getTurn = [](Vec2d p0, Vec2d p1, Vec2d p2) -> double {
        return (p1[0]-p0[0])*(p2[1]-p0[1])-(p1[1]-p0[1])*(p2[0]-p0[0]);
    };

    deque<Vec2d> omega;
    auto getOmegaSecond = [&]() -> Vec2d {
        auto top = omega.back(); omega.pop_back();
        auto sec = omega.back(); omega.push_back(top);
        return sec;
    };

    omega.push_back(radial_sort.getPoint(0));
    omega.push_back(radial_sort.getPoint(1));
    for (int i=2; i < radial_sort.size(); ) {
        Vec2d Pi = radial_sort.getPoint(i);
        Vec2d PT1 = omega.back();
        //cout << " ch Pi " << i << " p " << Pi << endl;
        //cout << " ch PT1 " << i << " p " << PT1 << endl;
        if ( PT1 == radial_sort.getPoint(0) ) {
                //cout << " ch push " << i << " p " << Pi << endl;
                omega.push_back( Pi ); i++;
        }
        Vec2d PT2 = getOmegaSecond();
        double t = getTurn(PT2, PT1, Pi);
        //cout << " ch PT2 " << i << " p " << PT2 << " t " << t << endl;
        if (t > 0) {
            //cout << " ch push " << i << " p " << Pi << endl;
            omega.push_back( Pi ); i++; }
        else {
            //cout << " ch pop " << i << " p " << omega.back() << endl;
            omega.pop_back();
        }
    }

    VRPolygon res;
    res.convex = true;
    for (auto p : omega) res.addPoint(p);
    //cout << " VRPolygon::getConvexHull res " << res.toString() << endl;
    return res;*/

    /*CGAL::set_ascii_mode(std::cin);
    CGAL::set_ascii_mode(std::cout);
    std::istream_iterator< Point_2 >  in_start( std::cin );
    std::istream_iterator< Point_2 >  in_end;
    std::ostream_iterator< Point_2 >  out( std::cout, "\n" );*/

    VRPolygon res;
#ifndef WITHOUT_CGAL
    vector<Kernel::Point_2> pIn; for (auto p : points) pIn.push_back(Kernel::Point_2(p[0],p[1]));
    vector<Kernel::Point_2> pOut; for (auto p : points) pOut.push_back(Kernel::Point_2());
    auto pOutEnd = CGAL::ch_graham_andrew( pIn.begin(), pIn.end(), pOut.begin() );
    for (auto pItr = pOut.begin(); pItr != pOutEnd; pItr++) {
        auto p = *pItr;
        res.addPoint(Vec2d(p[0],p[2]));
    }
#endif
    return res;
}

double VRPolygon::getTurn(Vec2d p0, Vec2d p1, Vec2d p2) {
    return (p1[0]-p0[0])*(p2[1]-p0[1])-(p1[1]-p0[1])*(p2[0]-p0[0]);
}

bool VRPolygon::isConvex() {
    for (unsigned int i=2; i<points.size(); i++) {
        if (getTurn(points[i-2], points[i-1], points[i]) >= 0) return false;
    }

    for (unsigned int i=2; i<points3.size(); i++) {
        Vec3d& p0 = points3[i-2];
        Vec3d& p1 = points3[i-1];
        Vec3d& p2 = points3[i-0];
        if (getTurn(Vec2d(p0[0],p0[2]), Vec2d(p1[0],p1[2]), Vec2d(p2[0],p2[2])) >= 0) return false;
    }

    return true;
}

void VRPolygon::reverseOrder() {
    reverse(points.begin(), points.end());
    reverse(points3.begin(), points3.end());
}

vector< VRPolygon > VRPolygon::getConvexDecomposition() {
    vector< VRPolygon > res;

    if (isConvex()) { // allready convex?
        res.push_back(*this);
        return res;
    }

#ifndef WITHOUT_CGAL
    CGALPolygon cgalpoly;
    if (!isCCW()) reverseOrder();
    cgalpoly = toCGALVRPolygon(*this);
    CGALPolyList partitions;
    CGALTraits traits;
    CGAL::optimal_convex_partition_2(cgalpoly.vertices_begin(), cgalpoly.vertices_end(), std::back_inserter(partitions), traits);
    for (auto p : partitions) res.push_back( fromCGALPolygon(p) );
#endif

    return res;
}

vector<Vec3d> VRPolygon::toSpace(Matrix4d m) {
    vector<Vec3d> res;
    for (auto p : points) {
        Vec3d pp = Vec3d(p[0], p[1], -sqrt(1-(p[0]*p[0]+p[1]*p[1])));
        m.mult(pp,pp);
        res.push_back(pp);
    }
    return res;
}

string VRPolygon::toString() {
    stringstream ss;
    ss << "poly: \n ";
    for (auto p : points) ss << p << " \n ";
    return ss.str();
}

double VRPolygon::getDistance(Vec3d p) { // TODO
    double dist2 = 1.0e20;
    return sqrt(dist2);
}

PathPtr VRPolygon::toPath() {
    auto res = Path::create();
    for (int i=0; i<int(points.size()); i++) {
        Vec2d& p1 = points[(i-1)%points.size()];
        Vec2d& p2 = points[i];
        Vec2d& p3 = points[(i+1)%points.size()];
        Vec2d d1 = p2-p1; d1.normalize();
        Vec2d d2 = p3-p2; d2.normalize();
        Vec2d n = d1+d2; n.normalize();
        res->addPoint( Pose( Vec3d(p2[0], 0, p2[1]), Vec3d(n[0], 0, n[1]), Vec3d(0,1,0) ) );
    }
    res->close();
    res->compute(2);
    return res;
}



