#include "VRBRepEdge.h"

#include "core/math/pose.h"
#include "core/utils/isNan.h"
#include "core/utils/toString.h"
#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGMatrix.h>

using namespace OSG;

VRBRepEdge::VRBRepEdge() {}
VRBRepEdge::~VRBRepEdge() {}

VRBRepEdgePtr VRBRepEdge::create() { return VRBRepEdgePtr(new VRBRepEdge()); }

void VRBRepEdge::setLine(Vec3d point1, Vec3d point2) {
    EBeg = point1;
    EEnd = point2;
    etype = "Line";
}

void VRBRepEdge::setCircle(PosePtr c, double r, double a1, double a2) {
    radius = r;
    center = c;
    etype = "Circle";

    Pnt3d c1(cos(a1), sin(a1), 0);
    Pnt3d c2(cos(a2), sin(a2), 0);
    Matrix4d m = center->asMatrix();
    m.multFull(c1, EBeg);
    m.multFull(c2, EEnd);
}

Vec3d& VRBRepEdge::beg() { return points.size() > 0 ? points[0] : n; }
Vec3d& VRBRepEdge::end() { return points.size() > 0 ? points[points.size()-1] : n; }

vector<Vec3d> VRBRepEdge::getPoints() { return points; }

void VRBRepEdge::swap() {
    //cout << "VRBRepEdge::swap\n";
    std::swap(EBeg, EEnd);
    std::swap(a1, a2);
    reverse(points.begin(), points.end());
    swapped = !swapped;
}

bool VRBRepEdge::connectsTo(VRBRepEdgePtr e) { return ( sameVec(end(), e->beg()) ); }

double VRBRepEdge::compCircleDirection(PosePtr sTrans, Vec3d cN) {
    double ddd = sTrans->dir().dot(center->dir());
    double udu = sTrans->up().dot(center->up());
    double uangle = acos(udu);

    int cDir = orientation;
    if (ddd<0) cDir *= -1;
    return cDir;
}

static const double pi = 3.1415926535;

void VRBRepEdge::compute() {
    if (isNan(EBeg)) cout << "Error in VRBRepEdge::compute, EBeg contains NaN!" << endl;
    if (isNan(EEnd)) cout << "Error in VRBRepEdge::compute, EBeg contains NaN!" << endl;

    auto computeSplineRes = [&](vector<Vec3d>& cpoints) {
        double Lcurv = 0;
        for (int i=1; i<cpoints.size(); i++) {
            Vec3d p1 = cpoints[i-1];
            Vec3d p2 = cpoints[i];
            Lcurv += (p2-p1).length();
        }

        double K = 2*pi*30;
        //double K = 2*pi;
        int res = ceil(Ncurv*Lcurv/K);
        //cout << "computeSplineRes, res: " << res << ", L: " << Lcurv << endl;
        return res;
    };

    if (etype == "Line") {
        points.push_back(EBeg);
        points.push_back(EEnd);
        if (points.size() <= 1) cout << "Warning: No edge points of Line" << endl;
        return;
    }

    if (etype == "Circle") {
        float _r = 1/radius;
        Matrix4d m = center->asMatrix();
        Matrix4d mI = m; mI.invert();

        // get start and end angles
        Vec3d c1,c2;
        mI.mult(Pnt3d(EBeg), c1);
        mI.mult(Pnt3d(EEnd), c2);
        //cout << " circle ends: " << EBeg << " -> " << c1 << " , " << EEnd << " -> " << c2 << endl;
        c1 *= _r; c2*= _r;
        a1 = atan2(c1[1],c1[0]);
        a2 = atan2(c2[1],c2[0]);
        //cout << " circle edge: " << Vec2d(a1, a2) << endl;
        if (a1 == a2) fullCircle = true;
        //if (a1 > a2) a2 += 2*Pi;

        /*cout << "VRBRepEdge Circle ";
        //cout << radius << " " << center->toString();
        cout << " points: " << points.size() << " c1: " << c1 << " c2: " << c2;
        //cout << " EBeg: " << EBeg << " EEnd: " << EEnd;
        //cout << " a1: " << a1 << " a2: " << a2;
        cout << endl;*/

        angles = angleFrame(a1, a2);
        for (auto a : angles) {
            Pnt3d p(radius*cos(a),radius*sin(a),0);
            m.mult(p,p);
            if (isNan(p)) cout << "Error in VRBRepEdge::build, circle point contains NaN!" << endl;
            else points.push_back(Vec3d(p));
            //cout << " c point " << p << endl;
        }


        if (points.size() <= 1) cout << "Warning: No edge points of Circle" << endl;
        return;
    }

    if (etype == "B_Spline_Curve_With_Knots") {
        if (cpoints.size() <= 1) cout << "Warning: No control points of B_Spline_Curve_With_Knots" << endl;
        if (knots.size() < cpoints.size() + deg + 1) { cout << "B_Spline_Curve_With_Knots, not enough knots: " << knots.size() << endl; return; }

        bool doWeights = (weights.size() == cpoints.size());

        int res = computeSplineRes(cpoints);
        float T = knots[knots.size()-1] - knots[0];
        for (int i=0; i<=res; i++) {
            float t = knots[0]+i*T/res;
            Vec3d p = doWeights ? BSplineW(t, deg, cpoints, knots, weights) : BSpline(t, deg, cpoints, knots);
            if (isNan(p)) cout << "Error in VRBRepEdge::build, B spline curve point contains NaN! doWeights: " << doWeights << endl;
            else points.push_back(p);
        }

        if (points.size() <= 1) cout << "Warning: No edge points of B_Spline_Curve_With_Knots" << endl;
        return;
    }

    cout << "Error: edge geo type not handled " << etype << endl;
}
