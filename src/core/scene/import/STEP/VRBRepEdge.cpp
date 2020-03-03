#include "VRBRepEdge.h"

#include "core/math/pose.h"
#include <OpenSG/OSGMatrix.h>

using namespace OSG;

VRBRepEdge::VRBRepEdge() {}

Vec3d& VRBRepEdge::beg() { return points.size() > 0 ? points[0] : n; }
Vec3d& VRBRepEdge::end() { return points.size() > 0 ? points[points.size()-1] : n; }

void VRBRepEdge::swap() {
    cout << "VRBRepEdge::swap\n";
    std::swap(EBeg, EEnd);
    std::swap(a1, a2);
    reverse(points.begin(), points.end());
}

bool VRBRepEdge::connectsTo(VRBRepEdge& e) { return ( sameVec(end(), e.beg()) ); }

void VRBRepEdge::build(string type) {
    etype = type;

    if (type == "Line") {
        points.push_back(EBeg);
        points.push_back(EEnd);
        if (points.size() <= 1) cout << "Warning: No edge points of Line" << endl;
        return;
    }

    if (type == "Circle") {
        float _r = 1/radius;
        Matrix4d m = center->asMatrix();
        Matrix4d mI = m; mI.invert();

        // get start and end angles
        Vec3d c1,c2;
        mI.mult(Pnt3d(EBeg), c1);
        mI.mult(Pnt3d(EEnd), c2);
        cout << " circle ends: " << EBeg << " -> " << c1 << " , " << EEnd << " -> " << c2 << endl;
        c1 *= _r; c2*= _r;
        a1 = atan2(c1[1],c1[0]);
        a2 = atan2(c2[1],c2[0]);
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
            points.push_back(Vec3d(p));
            //cout << " c point " << p << endl;
        }


        if (points.size() <= 1) cout << "Warning: No edge points of Circle" << endl;
        return;
    }

    if (type == "B_Spline_Curve_With_Knots") {
        if (cpoints.size() <= 1) cout << "Warning: No control points of B_Spline_Curve_With_Knots" << endl;
        if (knots.size() < cpoints.size() + deg + 1) { cout << "B_Spline_Curve_With_Knots, not enough knots: " << knots.size() << endl; return; }

        bool doWeights = (weights.size() == cpoints.size());

        int res = (Ncurv - 1)*0.5;
        float T = knots[knots.size()-1] - knots[0];
        for (int i=0; i<=res; i++) {
            float t = i*T/res;
            Vec3d p = doWeights ? BSplineW(t, deg, cpoints, knots, weights) : BSpline(t, deg, cpoints, knots);
            points.push_back(p);
        }

        if (points.size() <= 1) cout << "Warning: No edge points of B_Spline_Curve_With_Knots" << endl;
        return;
    }

    cout << "Error: edge geo type not handled " << type << endl;
}
