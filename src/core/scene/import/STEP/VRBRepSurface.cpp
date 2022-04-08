#include "VRBRepSurface.h"
#include "core/math/triangulator.h"
#include "core/math/pose.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/OSGGeometry.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/utils/isNan.h"
#include "core/utils/toString.h"
#include "addons/Algorithms/VRMeshSubdivision.h"
#include <OpenSG/OSGTriangleIterator.h>

using namespace OSG;

static const double pi = 3.1415926535;

VRBRepSurface::VRBRepSurface() {}

struct triangle {
    vector<Pnt3f> p; // vertex positions
    vector<Vec3f> v; // edge vectors
    vector<Vec2f> tcs; // uvs
    Vec3i i; // vertex indices
    double A = 0; // area
    Vec3f C;   // circumcenter
    double R;  // circumradius
    double R2; // circumradius square

    triangle(TriangleIterator it, bool computeCircumsphere = false, bool getTCs = false) : p(3), v(3) {
        i = Vec3i(it.getPositionIndex(0), it.getPositionIndex(1), it.getPositionIndex(2));
        for (int j=0; j<3; j++) p[j] = it.getPosition(j);
        v[0] = p[2]-p[1]; v[1] = p[2]-p[0]; v[2] = p[1]-p[0];
        A = v[0].cross(v[1]).squareLength();

        if (computeCircumsphere) {
            C = (Vec3f(p[0])+Vec3f(p[1])+Vec3f(p[2]))*1.0/3.0;
            R = C.dist(p[0]);
            R2 = R*R;
        }

        if (getTCs) {
            tcs.resize(3);
            for (int i=0; i<3; i++) tcs[i] = it.getTexCoords(i);
        }
    }

    Vec3d computeBaryCoords(Vec3f P, Vec3f n) {
        auto det = [&](Vec3f& A, Vec3f& B) {
            return A.cross(B).dot(n);
        };

        double d01 = det(v[2], v[1]);

        Vec3f p0(p[0]);
        double a =  (det(P, v[1]) - det(p0, v[1])) / d01;
        double b = -(det(P, v[2]) - det(p0, v[2])) / d01;
        double c = 1.0 - a - b;

        return Vec3d(a,b,c);
    }

    Vec2d computeBaryUV(Vec3d Pd) {
        Vec3f P = Vec3f(Pd)-Vec3f(p[0]);
        Vec3f n = v[2].cross(v[1]); n.normalize();
        P -= n * P.dot(n); // project on triangle plane
        P += Vec3f(p[0]);

        Vec2d uv;
        Vec3d abc = computeBaryCoords(P, n);
        if (tcs.size() == 3) uv = Vec2d( tcs[0]*abc[2] + tcs[1]*abc[0] + tcs[2]*abc[1] );

        //cout << "  computeBaryUV, P: " << Pd << ", P0: " << p[0] << ", P1: " << p[1] << ", P2: " << p[2] << ", abc: " << abc << endl;

        return uv;
    }

    double distToLine(Vec3f P, Vec3f A, Vec3f B) {
        Vec3f D = B-A;
        D.normalize();
        Vec3f V = P-A;
        float t = V.dot(D);
        Vec3f Pl = A+t*D;
        return Pl.dist(P);
    }

    Vec3f projected(Vec3d Pd) {
        Vec3f P = Vec3f(Pd)-Vec3f(p[0]);
        Vec3f n = v[2].cross(v[1]); n.normalize();
        P -= n * P.dot(n); // project on triangle plane
        return Vec3f(p[0])+P;
    }

    double isInside(Vec3d Pd, Vec2d& uv) {
        Vec3f P = Vec3f(Pd);
        if (P.dist2(C) > R2) return 1e6; // big distance

        P -= Vec3f(p[0]);
        Vec3f n = v[2].cross(v[1]); n.normalize();
        P -= n * P.dot(n); // project on triangle plane

        Vec3d abc = computeBaryCoords(Vec3f(p[0])+P, n);

        if (tcs.size() == 3) uv = Vec2d( tcs[0]*abc[2] + tcs[1]*abc[0] + tcs[2]*abc[1] );

        if (abc[0] >= 0 && abc[1] >= 0 && abc[2] >= 0) {
            cout << "  tri inside, P: " << Pd << ", P0: " << p[0] << ", P1: " << p[1] << ", P2: " << p[2] << ", abc: " << abc << endl;
            return 0;
        } else {
            //return 1e6;
            if (abc[0] < 0 && abc[1] < 0) return P.length();   // distance to p0
            if (abc[0] < 0 && abc[2] < 0) return v[1].dist(P); // distance to p2
            if (abc[1] < 0 && abc[2] < 0) return v[2].dist(P); // distance to p1
            //return 1e6;
            if (abc[0] < 0) return distToLine(P, Vec3f(), v[1]);
            if (abc[1] < 0) return distToLine(P, Vec3f(), v[2]);
            if (abc[2] < 0) return distToLine(P, v[1]   , v[2]);
            return 0;
        }

        return 1e6; // big distance
    }
};

VRGeometryPtr VRBRepSurface::build(string type, bool same_sense) {
    //cout << "VRSTEP::Surface build " << type << endl;

    Matrix4d m;
    Vec3d d, u;
    if (trans) {
        m = trans->asMatrix();
        d = trans->dir();
        u = trans->up();
    }
    Matrix4d mI = m;
    mI.invert();

    auto checkOrder = [&](Pnt3d p0, Pnt3d p1, Pnt3d p2, Vec3d n) {
        float cp = (p1-p0).cross(p2-p0).dot(n);
        return (cp >= 0);
    };

    auto pushTri = [&](VRGeoData& g, Pnt3d p1, Pnt3d p2, Pnt3d p3, Vec3d n) {
        //cout << " pushTri " << p1 << "   " << p2 << "   " << p3 << endl;
        int a = g.pushVert(p1,n);
        int b = g.pushVert(p2,n);
        int c = g.pushVert(p3,n);
        if (checkOrder(p1,p2,p3,n)) g.pushTri(a,b,c);
        else g.pushTri(a,c,b);
    };

    auto pushQuad = [&](VRGeoData& g, Pnt3d p1, Pnt3d p2, Pnt3d p3, Pnt3d p4, Vec3d n) {
        //cout << " pushQuad " << p1 << "   " << p2 << "   " << p3 << "   " << p4 << endl;
        int a = g.pushVert(p1,n);
        int b = g.pushVert(p2,n);
        int c = g.pushVert(p3,n);
        int d = g.pushVert(p4,n);
        if (checkOrder(p1,p2,p3,n)) g.pushTri(a,b,c);
        else g.pushTri(a,c,b);
        if (checkOrder(p2,p3,p4,n)) g.pushTri(b,c,d);
        else g.pushTri(b,d,c);
    };

    auto pushPen = [&](VRGeoData& g, Pnt3d p1, Pnt3d p2, Pnt3d p3, Pnt3d p4, Pnt3d p5, Vec3d n) {
        //cout << " pushPen " << p1 << "   " << p2 << "   " << p3 << "   " << p4 << "   " << p5 << endl;
        int a = g.pushVert(p1,n);
        int b = g.pushVert(p2,n);
        int c = g.pushVert(p3,n);
        int d = g.pushVert(p4,n);
        int e = g.pushVert(p5,n);
        if (checkOrder(p1,p2,p3,n)) g.pushTri(a,b,c);
        else g.pushTri(a,c,b);
        if (checkOrder(p2,p3,p4,n)) g.pushTri(b,c,d);
        else g.pushTri(b,d,c);
        if (checkOrder(p2,p4,p5,n)) g.pushTri(b,d,e);
        else g.pushTri(b,e,d);
    };

    auto sphericalUnproject = [&](Vec3d& p, double& lastTheta, double& lastPhi, int type, double cDir = 0, bool curveEnd = false) {
        mI.mult(Pnt3d(p),p);
        Vec3d n = p*1.0/R;
        //cout << " sphericalUnproject, p: " << p << ", lastTheta: " << lastTheta << ", lastPhi: " << lastPhi << ", type: " << type << ", cDir: " << cDir << ", curveEnd: " << curveEnd << endl;
        //cout << " -> R: " << R << ", n: " << n << " nL: " << n.length() << endl;
        if (n[2] <= -1.0) n[2] = -1.0;
        if (n[2] >=  1.0) n[2] =  1.0;
        double theta = asin(n[2]); // theta, angle to up axis z, -pi/2 -> pi/2
        double phi   = atan2(n[1], n[0]); // phi, angle in horizontal plane, -pi -> pi

        // ambigous points, when theta points up or down, phi can be any value-> take old one!
        if (abs(theta) > pi*0.5-1e-3) phi = lastPhi;

        if (abs(phi) > pi-1e-3) { // ambigous point on +- pi
            //cout << " !!! amb point?: " << phi << ", cDir: " << cDir << ", type: " << type << ", curveEnd: " << curveEnd << endl;
            if (type == 1) { // circle or B-Spline
                if (!curveEnd) phi = lastPhi;
                if (cDir > 0 && curveEnd) phi =  pi;
                if (cDir < 0 && curveEnd) phi = -pi;
                //cout << "   set circle phi: " << phi << endl;
            }

            if (type == 2) { // B-Spline, stay close to old value!
                if (!curveEnd) phi = lastPhi;
                if (lastPhi < 0 && curveEnd) phi = -pi;
                if (lastPhi > 0 && curveEnd) phi =  pi;
                //cout << "   set spline phi: " << phi << endl;
            }
        }

        //cout << "   -> theta, phi: " << theta << ", " << phi << endl;

        lastTheta = theta;
        lastPhi = phi;
        return Vec2d(theta, phi);
    };

    auto cylindricUnproject = [&](Vec3d& p, double& lastAngle, int type, double cDir = 0, bool circleEnd = false) {
        mI.mult(Pnt3d(p),p);
        //cout << " cylindricUnproject, p: " << p << ", lastAngle: " << lastAngle << ", type: " << type << ", cDir: " << cDir << ", circleEnd: " << circleEnd << endl;
        //cout << " -> R: " << R << ", r: " << p[0]*p[0]+p[1]*p[1] << endl;
        double h = p[2];
        double a = atan2(p[1]/R, p[0]/R);

        if (abs(a) > pi-1e-3) { // ambigous point on +- pi
            //cout << "  amb point?: " << a << ", cDir: " << cDir << endl;
            if (type == 0 && lastAngle != 1000) a = lastAngle; // next point on line

            if (type == 1) { // circle
                if (!circleEnd) a = lastAngle;
                if (cDir > 0 && circleEnd) a =  pi;
                if (cDir < 0 && circleEnd) a = -pi;
                //cout << "   set circle a: " << a << endl;
            }

            if (type == 2) { // B-Spline, stay close to old value!
                if (!circleEnd) a = lastAngle;
                if (lastAngle < 0 && circleEnd) a = -pi;
                if (lastAngle > 0 && circleEnd) a =  pi;
                //cout << "   set spline a: " << a << endl;
            }
        }

        lastAngle = a;
        return Vec2d(a,h);
    };

    // TODO: this may fail if inner polygons are not moved accrodingly
    //  maybe it would be better to make a border instead of moving points
    auto fixPolyJump = [&](VRPolygon& poly) {
        auto& points = poly.get();
        for (int i=1; i<points.size(); i++) {
            Vec2d& p1 = points[i-1];
            Vec2d& p2 = points[i];
            if (abs(p2[0] - p1[0]) > 1.5*pi) {
                if (p2[0] < 0) p2[0] += 2*pi;
                else p2[0] -= 2*pi;
            }
        }
    };

    auto checkPolyIntegrety = [&](VRPolygon& poly) { // for cylinder
        auto& points = poly.get();
        for (int i=1; i<points.size(); i++) {
            Vec2d p1 = points[i-1];
            Vec2d p2 = points[i];
            if (abs(p2[0] - p1[0]) > 1.5*pi) {
                //cout << "Warning! cylinder polygon jump detected: " << p1 << " -> " << p2 << endl;
                fixPolyJump(poly); // try to fix it
                return;
            }
        }
    };

    auto checkPolyOrientation = [&](VRPolygon& poly, VRBRepBound& bound) {
        bool isCCW = poly.isCCW();
        if (same_sense) {
            if (!isCCW && bound.outer) poly.reverseOrder();
            if (isCCW && !bound.outer) poly.reverseOrder();
        } else {
            if (isCCW && bound.outer) poly.reverseOrder();
            if (!isCCW && !bound.outer) poly.reverseOrder();
        }
    };

    auto computeSplineRes = [&](field<Vec3d>& cpoints) {
        double LcurvU = 0;
        double LcurvV = 0;
        for (int i=1; i<cpoints.width; i++) {
            Vec3d p1 = cpoints.get(i-1,0);
            Vec3d p2 = cpoints.get(i,0);
            LcurvV += (p2-p1).length();
        }
        for (int j=1; j<cpoints.height; j++) {
            Vec3d p1 = cpoints.get(0,j-1);
            Vec3d p2 = cpoints.get(0,j);
            LcurvU += (p2-p1).length();
        }

        double K = 2*pi*50;
        int resI = Ncurv*ceil(LcurvU/K);
        int resJ = Ncurv*ceil(LcurvV/K);
        //cout << "res: " << Vec2i(resI, resJ) << ", L: " << Vec2i(LcurvU, LcurvV) << endl;
        return Vec2i(resI, resJ);
    };

    //if (type != "Spherical_Surface") return 0;

    if (type == "Plane") {
        //return 0;
        //cout << "make Plane, N bounds: " << bounds.size() << endl;
        Triangulator t;
        if (bounds.size() == 0) cout << "Warning: No bounds!\n";

        vector<int> sortedBounds;
        for (int i=0; i<bounds.size(); i++) if ( bounds[i].outer) sortedBounds.push_back(i);
        for (int i=0; i<bounds.size(); i++) if (!bounds[i].outer) sortedBounds.push_back(i);

        //for (auto& b : bounds) {
        for (auto& bi : sortedBounds) {
            auto& b = bounds[bi];
            //if (b.points.size() == 0) cout << "Warning: No bound points for bound " << b.BRepType << endl;
            bool doPrint = b.containsNan();
            VRPolygon poly;

            for (auto pIn : b.points) {
                Vec3d pOut;
                if (doPrint) cout << " pIn: " << pIn << endl;
                mI.multFull(Pnt3d(pIn), pOut);
                if (doPrint) cout << " pOut: " << pOut << " " << mI[0] << ", " << mI[1] << ", " << mI[2] << ", " << mI[3] << endl;
                if (abs(pOut[2]) > 0.001) cout << " Error in VRBRepSurface::build Plane, p[2] not 0 (" << pOut[2] << ")! -> data loss" << endl;
                poly.addPoint(pOut);
            }

            checkPolyOrientation(poly, b);
            t.add(poly);
        }

        auto g = t.compute();
        if (!g) return 0;
        if (!g->getMesh()->geo->getPositions()) cout << "NO MESH!\n";
        g->setMatrix(m);

        Vec3d nP = Vec3d(0, 0, 1);
        if (!same_sense) nP *= -1;
        GeoVectorPropertyMTRecPtr norms = g->getMesh()->geo->getNormals();
        for (uint i=0; i<norms->size(); i++) norms->setValue(nP, i);

        return g;
    }

    if (type == "Cylindrical_Surface") {
        //return 0;
        //cout << "Surface: " << type << endl;
        Triangulator triangulator; // feed the triangulator with unprojected points

        for (auto b : bounds) {
            //cout << " Bound, outer: " << b.outer << endl;
            VRPolygon poly;
            double lastAngle = 1000;
            Vec3d cN(0,0,1);

            // shift edges so first edge not start on +-pi line
            auto eOnPiLine = [&](VRBRepEdge& e) {
                Vec3d p = e.points[0];
                mI.mult(Pnt3d(p),p);
                if (abs(p[1]) > 1e-3) return false;
                if (p[0] > 1e-3) return false;
                return true;
            };

            if (eOnPiLine(b.edges[0]))  {
                int i0 = -1;
                for (int i=1; i<b.edges.size(); i++) {
                    if (!eOnPiLine(b.edges[i])) {
                        i0 = i;
                        break;
                    }
                }

                b.shiftEdges(i0);
            }

            for (auto& e : b.edges) {
                //cout << "  edge " << e.etype << endl;

                if (e.etype == "Circle") {
                    double cDir = e.compCircleDirection(mI, cN);

                    if (poly.size() == 0) {
                        Vec2d p1 = cylindricUnproject(e.EBeg, lastAngle, 1, cDir, false);
                        poly.addPoint(p1);
                        //cout << "   p0: " << p1 << endl;
                    }

                    Vec2d p2 = cylindricUnproject(e.EEnd, lastAngle, 1, cDir, true);
                    poly.addPoint(p2);
                    //cout << "   p: " << p2 << endl;
                    continue;
                }

                if (e.etype == "Line") {
                    if (poly.size() == 0) {
                        Vec2d p1 = cylindricUnproject(e.EBeg, lastAngle, 0);
                        poly.addPoint(p1);
                        //cout << "   p0: " << p1 << endl;
                    }

                    Vec2d p2 = cylindricUnproject(e.EEnd, lastAngle, 0);
                    poly.addPoint(p2);
                    //cout << "   p: " << p2 << endl;
                    continue;
                }

                if (e.etype == "B_Spline_Curve_With_Knots") {
                    int i0 = 1;
                    if (poly.size() == 0) i0 = 0;
                    for (int i=i0; i<e.points.size(); i++) {
                        auto& p = e.points[i];
                        Vec2d pc = cylindricUnproject(p, lastAngle, 2, 0, i>0);
                        poly.addPoint(pc);
                        //cout << "   p: " << pc << endl;
                    }
                    continue;
                }

                cout << "Unhandled edge on cylinder of type " << e.etype << endl;
            }

            checkPolyIntegrety(poly);

            //cout << "  poly: " << toString(poly.get()) << endl;
            checkPolyOrientation(poly, b);
            triangulator.add(poly);
        }

        auto g = triangulator.compute();
        if (!g) return 0;
        if (auto gg = g->getMesh()->geo) { if (!gg->getPositions()) cout << "VRBRepSurface::build: Triangulation failed, no mesh positions!\n";
        } else cout << "VRBRepSurface::build: Triangulation failed, no mesh generated!\n";

        /* intersecting the cylinder rays with a triangle (2D)

        - get triangle min/max in x
        - get the cylinder rays that will intersect the triangle
        - get for each segment the intersection points and map them to a ray ID
        - create list of sides
        - check if all vertices on the same side

        - 3 possible cases for a cylinder side:
            + triangle
            + quad -> 2 triangles
            + pentagon -> 3 triangles
        - possible cases:
            + only one cylinder side (no ray intersections)
                -> create only one triangle
            + one ray intersection
                -> one triangle and one quad = 3 triangles (strip)
            + multiple ray intersections
                - two triangle vertices on the same side
                    -> one triangle and then only quads
                - all three vertices on different sides
                    -> two triangles and the side of the middle vertex has a pentagon!!!

        - special cases:
            + the triangle segment is parallel and on top of a ray
            + a triangle vertex is on a ray
            -> test triangle vertex on ray

        */

        auto getXsize = [](vector<Pnt3f>& pnts) {
            Vec2d res(pnts[0][0], pnts[0][0]);
            for (auto& p : pnts) {
                if (p[0] < res[0]) res[0] = p[0];
                if (p[0] > res[1]) res[1] = p[0];
            }
            return res;
        };

        // tesselate the result while projecting it back on the surface
        if (g) if (auto gg = g->getMesh()) {
            TriangleIterator it;
            VRGeoData nMesh;
            Vec3d n(0,-1,0);
            if (!same_sense) n *= -1;

            for (it = TriangleIterator(gg->geo); !it.isAtEnd() ;++it) {
                triangle tri(it);
                if (tri.A < 1e-6) continue; // ignore flat triangles

                Vec2d xs = getXsize(tri.p); // triangle width
                vector<float> rays = angleFrame(xs[0], xs[1]);

                vector<Vec2d> sides;
                Vec3i pSides;
                //cout << " triangle size in x " << xs*180/Pi << " " << rays.size() << endl;
                //cout << " triangle: " << tri.p[0] << "   " << tri.p[1] << "   " << tri.p[2] << endl;
                for (uint i=1; i<rays.size(); i++) {
                    auto s = Vec2d(rays[i-1], rays[i]);
                    //cout << "  side " << s*180/Pi << endl;
                    sides.push_back( s ); // get all cylinder faces
                    for (int j=0; j<3; j++) { // find out on what cylinder face each vertex lies
                        if (tri.p[j][0] >= rays[i-1] && tri.p[j][0] <= rays[i]) {
                            pSides[j] = i-1;
                        }
                    }
                }

                VRMeshSubdivision subdiv;
                subdiv.segmentTriangle(nMesh, pSides, tri.p, n, sides);
            }

            nMesh.apply(g);

            // project the points back into 3D space
            GeoVectorPropertyMTRecPtr pos = gg->geo->getPositions();
            GeoVectorPropertyMTRecPtr norms = gg->geo->getNormals();
            if (pos) {
                for (uint i=0; i<pos->size(); i++) {
                    Pnt3d p = Pnt3d(pos->getValue<Pnt3f>(i));
                    Vec3d n = Vec3d(norms->getValue<Vec3f>(i));
                    double a = p[0];
                    double h = p[2];
                    n = Vec3d(cos(a), sin(a), 0);

                    /*Vec2d side = getSide(p[0]);
                    Vec3d A = Vec3d(R*cos(side[0]), R*sin(side[0]), 0);
                    Vec3d B = Vec3d(R*cos(side[1]), R*sin(side[1]), 0);
                    Vec3d D = B-A;
                    D.normalize();*/

                    float t = R;//(A[0]*D[1] - A[1]*D[0]) / (n[0]*D[1] - n[1]*D[0]);

                    //cout << "p: " << a/Pi*180 << " AB: " << side*(180/Pi) << " s: " << getSideN(a) << endl;
                    //if (a < side[0] || a > side[1]) cout << "   AAAH\n"; // TODO: check this out!

                    p[2] = h;
                    p[1] = n[1]*t;
                    p[0] = n[0]*t;

                    pos->setValue(p, i);

                    if (!same_sense) n *= -1;
                    norms->setValue(n, i);
                }
            }
        }

        if (g) g->setMatrix(m);
        if (g && g->getMesh() && g->getMesh()->geo->getPositions() && g->getMesh()->geo->getPositions()->size() > 0) return g;
        return 0;
    }

    if (type == "B_Spline_Surface") {
        //return 0;
        cout << "B_Spline_Surface" << endl;

        // ROADMAP
        //  first idea:
        //   - tesselate whole BSpline surface (lots of quads)
        //   - keep uv map of the resulting mesh
        //   - for each edge point:
        //      - get nearest quad to point
        //      - get UV koords of the point on that quad
        //      - add point UV to polyline
        //   - triangulate in UV space
        //   - reuse first tesselation
        //      - cut quads traversed by edges
        //      - ignore quads outside of the triangulation


        Vec3d n(0,0,1);
        VRGeoData nMesh;

        map<int, map<int, int> > ids;

        cout << "B_Spline_Surface du " << degu << " dv " << degv << "  pw " << cpoints.width << " ph " << cpoints.height << endl;

        Vec2i res = computeSplineRes(cpoints);
        float T = 1; //knots[knots.size()-1] - knots[0];
        for (int i=0; i<=res[0]; i++) {
            float u = i*T/res[0];
            for (int j=0; j<=res[1]; j++) {
                float v = j*T/res[1];
                Vec3d p = BSpline(u,v, degu, degv, cpoints, knotsu, knotsv);
                ids[i][j] = nMesh.pushVert(p,n);

                if (i > 0 && j%2 == 0) nMesh.pushTri(ids[i][j], ids[i-1][j], ids[i-1][j+1]);
                if (i > 0 && j%2 == 1) nMesh.pushTri(ids[i][j], ids[i][j-1], ids[i-1][j]);
            }
        }

        auto g = nMesh.asGeometry("BSpline");


        // feed the triangulator with unprojected points
        /*Triangulator t;
        for (auto b : bounds) {
            VRPolygon poly;
            for(auto p : b.points) {
                mI.mult(Pnt3d(p),p);
                float u = p[0];
                float v = p[1];
                poly.addPoint(Vec2d(u, v));
            }
            if (!poly.isCCW()) poly.turn();
            t.add(poly);
        }
        auto g = t.compute();*/

        // tesselate the result while projecting it back on the surface
        /*if (g) if (auto gg = g->getMesh()) {
            TriangleIterator it;
            VRGeoData nMesh;
            Vec3d n(0,0,1);

            auto checkOrder = [&](Pnt3d p0, Pnt3d p1, Pnt3d p2) {
                float cp = (p1-p0).cross(p2-p0).dot(n);
                return (cp >= 0);
            };

            auto pushTri = [&](Pnt3d p1, Pnt3d p2, Pnt3d p3) {
                int a = nMesh.pushVert(p1,n);
                int b = nMesh.pushVert(p2,n);
                int c = nMesh.pushVert(p3,n);
                if (checkOrder(p1,p2,p3)) nMesh.pushTri(a,b,c);
                else nMesh.pushTri(a,c,b);
            };

            auto pushQuad = [&](Pnt3d p1, Pnt3d p2, Pnt3d p3, Pnt3d p4) {
                int a = nMesh.pushVert(p1,n);
                int b = nMesh.pushVert(p2,n);
                int c = nMesh.pushVert(p3,n);
                int d = nMesh.pushVert(p4,n);
                if (checkOrder(p1,p2,p3)) nMesh.pushTri(a,b,c);
                else nMesh.pushTri(a,c,b);
                if (checkOrder(p2,p3,p4)) nMesh.pushTri(b,c,d);
                else nMesh.pushTri(b,d,c);
            };

            for (it = TriangleIterator(gg); !it.isAtEnd() ;++it) {
                triangle t(it);
                if (t.A < 1e-6) continue; // ignore flat triangles

                Vec3i pOrder(0,1,2); // get the order of the vertices
                for (int i=0; i<3; i++) { // max 3 sort steps
                    if (pSides[pOrder[0]] > pSides[pOrder[1]]) swap(pOrder[0], pOrder[1]);
                    else if (pSides[pOrder[0]] > pSides[pOrder[2]]) swap(pOrder[0], pOrder[2]);
                    else if (pSides[pOrder[1]] > pSides[pOrder[2]]) swap(pOrder[1], pOrder[2]);
                }

                if (pSides[0] == pSides[1] && pSides[0] == pSides[2]) {
                    pushTri(t.p[0],t.p[1],t.p[2]);
                    continue;
                }

                //cout << " unhandled triangle " << endl;
            }

            nMesh.apply(g);

            // project the points back into 3D space
            GeoVectorPropertyMTRecPtr pos = gg->getPositions();
            GeoVectorPropertyMTRecPtr norms = gg->getNormals();
            if (pos) {
                for (int i=0; i<pos->size(); i++) {
                    Pnt3d p = pos->getValue<Pnt3f>(i);
                    Vec3d n = norms->getValue<Vec3f>(i);
                    n = Vec3d(cos(p[0]), sin(p[0]), 0);

                    Vec2d side = getSide(p[0]);
                    Vec3d A = Vec3d(R*cos(side[0]), R*sin(side[0]), 0);
                    Vec3d B = Vec3d(R*cos(side[1]), R*sin(side[1]), 0);
                    Vec3d D = B-A;
                    D.normalize();

                    float t = (A[0]*D[1] - A[1]*D[0]) / (n[0]*D[1] - n[1]*D[0]);

                    //cout << "p: " << p[0]/Pi*180 << " AB: " << side*(180/Pi) << " s: " << getSideN(p[0]) << endl;
                    //if (p[0] < side[0] || p[0] > side[1]) cout << "   AAAH\n"; // TODO: check this out!

                    p[2] = p[1];
                    p[1] = n[1]*t;
                    p[0] = n[0]*t;

                    pos->setValue(p, i);
                    norms->setValue(n, i);
                }
            }
        }*/

        if (g) g->setMatrix(m);
        if (g && g->getMesh() && g->getMesh()->geo->getPositions() && g->getMesh()->geo->getPositions()->size() > 0) return g;
        return 0;
    }

    if (type == "B_Spline_Surface_With_Knots") {
        //return 0;
        cout << "B_Spline_Surface_With_Knots" << endl;
        // ROADMAP
        //  first idea:
        //   - tesselate whole BSpline surface (lots of quads)
        //   - keep uv map of the resulting mesh
        //   - for each edge point:
        //      - get nearest quad to point
        //      - get UV koords of the point on that quad
        //      - add point UV to polyline
        //   - triangulate in UV space
        //   - reuse first tesselation
        //      - cut quads traversed by edges
        //      - ignore quads outside of the triangulation

        bool isWeighted = (weights.height == cpoints.height && weights.width == cpoints.width);

        Vec3d n(0,0,1);
        VRGeoData nMesh;

        if (knotsu.size() == 0 || knotsv.size() == 0) return 0;

        // BSpline mesh
        map<int, map<int, int> > ids;
        Vec2i res = computeSplineRes(cpoints);
        float Tu = knotsu[knotsu.size()-1] - knotsu[0];
        float Tv = knotsv[knotsv.size()-1] - knotsv[0];
        for (int i=0; i<=res[0]; i++) {
            float u = knotsu[0]+i*Tu/res[0];
            //cout << " i: " << i << " Tu: " << Tu << " u: " << u << " knotsu[0]: " << knotsu[0] << " knotsu[-1]: " << knotsu[knotsu.size()-1] << endl;
            for (int j=0; j<=res[1]; j++) {
                float v = knotsv[0]+j*Tv/res[1];
                Vec3d p = isWeighted ? BSpline(u,v, degu, degv, cpoints, knotsu, knotsv, weights) : BSpline(u,v, degu, degv, cpoints, knotsu, knotsv);
                Vec3d n = isWeighted ? BSplineNorm(u,v, degu, degv, cpoints, knotsu, knotsv, weights) : BSplineNorm(u,v, degu, degv, cpoints, knotsu, knotsv);
                if (same_sense) n *= -1;
                ids[i][j] = nMesh.pushVert(p,n,Vec2d(u,v));

                if (same_sense) {
                    if (i > 0 && j > 0) nMesh.pushQuad(ids[i][j], ids[i-1][j], ids[i-1][j-1], ids[i][j-1]);
                } else {
                    if (i > 0 && j > 0) nMesh.pushQuad(ids[i][j], ids[i][j-1], ids[i-1][j-1], ids[i-1][j]);
                }
            }
        }

        // compute bounds on surface
        auto g = nMesh.asGeometry("BSpline");
        if (g) if (auto gg = g->getMesh()) {
            Triangulator triangulator;

            for (auto b : bounds) {

                cout << " BSpline Bound, outer: " << b.outer << " " << b.points.size() << endl;

                /*for (auto& e : b.edges) {
                    cout << "  edge " << e.etype << ", Np: " << e.points.size() << ", BE: " << e.EBeg << " -> " << e.EEnd << endl;
                }*/


                VRPolygon poly;

                vector<triangle> triangles;
                TriangleIterator it;
                for (it = TriangleIterator(gg->geo); !it.isAtEnd() ;++it) {
                    triangles.push_back( triangle(it, true, true ));
                }

                for (auto p : b.points) {
                    if (p[0] > 75) continue; // for testing

                    mI.multFull(p, p);
                    //cout << "bound point: " << p << endl;
                    double dmin = 1e5;
                    int imin = -1;
                    for (int i=0; i<triangles.size(); i++) {
                        auto& tri = triangles[i];
                        Vec2d uv;
                        double d = tri.isInside(p, uv);

                        if (d < dmin) {
                            dmin = d;
                            imin = i;
                        }

                        if (d < 1e-3) break; // is inside
                    }

                    if (imin >= 0) {
                        auto& tri = triangles[imin];
                        Vec2d uv = tri.computeBaryUV(p);
                        poly.addPoint(uv);
                        Vec3d p2 = isWeighted ? BSpline(uv[0],uv[1], degu, degv, cpoints, knotsu, knotsv, weights) : BSpline(uv[0],uv[1], degu, degv, cpoints, knotsu, knotsv);
                        //if (p2.dist(p) > 10)
                        //if (p2.length() < 10)
                        //cout << "imin: " << imin<< ", dmin: " << dmin << endl;
                        //    cout << " bound point unprojected, p -> uv -> p: " << p << " -> " << uv << " -> " << p2 << ", D: " << p2.dist(p) << endl;

                        /*auto pj = tri.projected(p);
                        boundPoints.pushVert(p,Vec3d(),Color3f(1,0,0));
                        boundPoints.pushVert(Vec3d(pj),Vec3d(),Color3f(0,1,0));
                        boundPoints.pushLine();
                        boundPoints.pushVert(p,Vec3d(),Color3f(1,0,0));
                        boundPoints.pushVert(p2,Vec3d(),Color3f(0,0,1));
                        boundPoints.pushLine();*/
                    }
                }

                //checkPolyOrientation(poly, b);
                triangulator.add(poly);
                cout << "  BSpline bounds poly: " << toString(poly.get()) << endl;
            }

            g = triangulator.compute();

            VRMeshSubdivision subdiv;
            //subdiv.subdivideTriangles( g, Vec3d(Tu/res[0], -1, Tv/res[1]) );
            //subdiv.subdivideGrid( g, Vec3d(Tu/res[0], -1, Tv/res[1]) );

            if (1) {
                VRPolygon ply;
                ply.addPoint(Vec2d(0,0));
                ply.addPoint(Vec2d(5,0));
                ply.addPoint(Vec2d(0,5));
                Triangulator tt;
                tt.add(ply);
                auto G = tt.compute();
                subdiv.subdivideGrid( G, Vec3d(0.4, -1, 0.5) );
                G->getMaterial()->setLit(0);
                G->getMaterial()->setWireFrame(true);
                return G;
            }


            if (g && 0) if (auto gg = g->getMesh()) {
                VRGeoData nMesh;

                VRGeoData tmp(g);
                cout << "unproject: " << tmp.size() << endl;

                for (auto it = TriangleIterator(gg->geo); !it.isAtEnd() ;++it) {
                    triangle tri(it);

                    for (int i=0; i<3; i++) {
                        double u = tri.p[i][0];
                        double v = tri.p[i][2];
                        Vec3d p = isWeighted ? BSpline(u,v, degu, degv, cpoints, knotsu, knotsv, weights) : BSpline(u,v, degu, degv, cpoints, knotsu, knotsv);
                        Vec3d n = isWeighted ? BSplineNorm(u,v, degu, degv, cpoints, knotsu, knotsv, weights) : BSplineNorm(u,v, degu, degv, cpoints, knotsu, knotsv);
                        if (same_sense) n *= -1;
                        nMesh.pushVert(p,n);
                        //cout << "bound point unprojected, uv: " << Vec2d(u,v) << " -> " << p << endl;
                    }

                    nMesh.pushTri();
                }
                nMesh.apply(g);
            }

            for (auto& b : bounds) g->addChild(b.asGeometry());
        }


        if (g) g->setMatrix(m);
        if (g && g->getMesh() && g->getMesh()->geo->getPositions() && g->getMesh()->geo->getPositions()->size() > 0) return g;
        return 0;
    }

    if (type == "Conical_Surface") {
        //cout << "Conical_Surface" << endl;
        return 0;
    }

    if (type == "Spherical_Surface") {
        cout << "Spherical_Surface" << endl;
        Triangulator triangulator; // feed the triangulator with unprojected points

        for (auto b : bounds) {
            VRPolygon poly;
            double lastTheta = 1000;
            double lastPhi   = 1000;
            Vec3d cN = Vec3d(0,0,1);

            // shift edges so first edge not start on +-pi line
            auto eOnPiLine = [&](VRBRepEdge& e) {
                Vec3d p = e.points[0];
                mI.mult(Pnt3d(p),p);
                if (abs(p[1]) > 1e-3) return false;
                if (p[0] > 1e-3) return false;
                return true;
            };

            if (eOnPiLine(b.edges[0]))  {
                int i0 = -1;
                for (int i=1; i<b.edges.size(); i++) {
                    if (!eOnPiLine(b.edges[i])) {
                        i0 = i;
                        break;
                    }
                }

                b.shiftEdges(i0);
            }

            for (auto& e : b.edges) {
                cout << " edge on sphere " << e.etype << endl;
                if (e.etype == "Circle") {
                    double cDir = e.compCircleDirection(mI, cN);

                    int i0 = 1;
                    if (poly.size() == 0) i0 = 0;
                    for (int i=i0; i<e.points.size(); i++) {
                        auto& p = e.points[i];
                        Vec2d pc = sphericalUnproject(p, lastTheta, lastPhi, 1, cDir, i>0);
                        cout << " ---- " << p << " -> " << pc << endl;
                        poly.addPoint(pc);
                    }
                    continue;
                }

                if (e.etype == "B_Spline_Curve_With_Knots") {
                    int i0 = 1;
                    if (poly.size() == 0) i0 = 0;
                    for (int i=i0; i<e.points.size(); i++) {
                        auto& p = e.points[i];
                        Vec2d pc = sphericalUnproject(p, lastTheta, lastPhi, 2, 0, i>0);
                        cout << " ---- " << p << " -> " << pc << endl;
                        poly.addPoint(pc);
                    }
                    continue;
                }

                cout << "Unhandled edge on sphere of type " << e.etype << endl;
            }

            checkPolyOrientation(poly, b);
            triangulator.add(poly);
            cout << "  poly: " << toString(poly.get()) << endl;
        }

        auto g = triangulator.compute();
        if (!g) return 0;
        if (auto gg = g->getMesh()->geo) { if (!gg->getPositions()) cout << "VRBRepSurface::build: Triangulation failed, no mesh positions!\n";
        } else cout << "VRBRepSurface::build: Triangulation failed, no mesh generated!\n";

        // tesselate the result while projecting it back on the surface
        if (g) if (auto gg = g->getMesh()) {
            TriangleIterator it;
            VRGeoData nMesh;
            Vec3d n(0,1,0);

            for (it = TriangleIterator(gg->geo); !it.isAtEnd() ;++it) {
                triangle tri(it);
                //if (tri.A < 1e-6) continue; // ignore flat triangles

                // TODO: subdivide the triangle and add sub triangles to nMesh
                /*int Nt =
                int Np =
                for (int j=0; j<3; j++) {
                    tri.p[j];
                }*/

                // test
                pushTri(nMesh, Pnt3d(tri.p[0]), Pnt3d(tri.p[1]), Pnt3d(tri.p[2]), n);
            }

            nMesh.apply(g);

            // project the points back into 3D space
            GeoVectorPropertyMTRecPtr pos = gg->geo->getPositions();
            GeoVectorPropertyMTRecPtr norms = gg->geo->getNormals();
            if (pos) {
                for (uint i=0; i<pos->size(); i++) {
                    Pnt3d p = Pnt3d(pos->getValue<Pnt3f>(i));
                    Vec3d n = Vec3d(norms->getValue<Vec3f>(i));
                    double theta = p[0];
                    double phi   = p[2];
                    n = Vec3d(cos(phi)*cos(theta), sin(phi)*cos(theta), sin(theta));

                    /*Vec2d side = getSide(p[0]);
                    Vec3d A = Vec3d(R*cos(side[0]), R*sin(side[0]), 0);
                    Vec3d B = Vec3d(R*cos(side[1]), R*sin(side[1]), 0);
                    Vec3d D = B-A;
                    D.normalize();

                    float t = (A[0]*D[1] - A[1]*D[0]) / (n[0]*D[1] - n[1]*D[0]);*/

                    //cout << "p: " << a/Pi*180 << " AB: " << side*(180/Pi) << " s: " << getSideN(a) << endl;
                    //if (a < side[0] || a > side[1]) cout << "   AAAH\n"; // TODO: check this out!

                    p = Pnt3d(n*R);

                    cout << "    sphere theta: " << theta << ", phi: " << phi << ", pos: " << p << ", R: " << R << endl;

                    pos->setValue(p, i);
                    norms->setValue(n, i);
                }
            }
        }

        if (g) g->setMatrix(m);
        if (g && g->getMesh() && g->getMesh()->geo->getPositions() && g->getMesh()->geo->getPositions()->size() > 0) return g;
        return 0;
    }

    if (type == "Toroidal_Surface") {
        //cout << "Toroidal_Surface" << endl;
        return 0;
    }

    cout << "VRBRepSurface::build Error: unhandled surface type " << type << endl;

    // wireframe
    auto geo = VRGeometry::create("face");
    GeoPnt3fPropertyMTRecPtr pos = GeoPnt3fProperty::create();
    GeoVec3fPropertyMTRecPtr norms = GeoVec3fProperty::create();
    GeoUInt32PropertyMTRecPtr inds = GeoUInt32Property::create();

    for (auto b : bounds) {
        for (uint i=0; i<b.points.size(); i+=2) {
            Pnt3d p1 = b.points[i];
            Pnt3d p2 = b.points[i+1];
            pos->addValue(p1);
            pos->addValue(p2);
            norms->addValue(Vec3d(0,1,0));
            norms->addValue(Vec3d(0,1,0));
            inds->addValue(pos->size()-2);
            inds->addValue(pos->size()-1);
        }
    }

    geo->setType(GL_LINES);
    geo->setPositions(pos);
    geo->setNormals(norms);
    geo->setIndices(inds);

    VRMaterialPtr mat = VRMaterial::create("face");
    mat->setLit(0);
    mat->setLineWidth(3);
    geo->setMaterial(mat);
    return geo;
}

