#include "VRBRepSurface.h"
#include "core/math/triangulator.h"
#include "core/math/pose.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/OSGGeometry.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/utils/isNan.h"
#include "core/utils/toString.h"
#include <OpenSG/OSGTriangleIterator.h>

using namespace OSG;

VRBRepSurface::VRBRepSurface() {}

struct triangle {
    vector<Pnt3f> p; // vertex positions
    vector<Vec3f> v; // edge vectors
    Vec3i i; // vertex indices
    float A = 0; // area

    triangle(TriangleIterator it) : p(3), v(3) {
        i = Vec3i(it.getPositionIndex(0), it.getPositionIndex(1), it.getPositionIndex(2));
        for (int i=0; i<3; i++) p[i] = it.getPosition(i);
        v[0] = p[2]-p[1]; v[1] = p[2]-p[0]; v[2] = p[1]-p[0];
        A = v[0].cross(v[1]).squareLength();
    }
};

static const double pi = 3.1415926535;

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

    auto rebaseAngle = [&](double& a, double& la) {
        if (la > -1000 && abs(a - la) > Pi) {
            if (a - la > Pi) a -= 2*Pi;
            else a += 2*Pi;
        }
    };

    auto cylindricUnproject = [&](Vec3d& p, double& lastAngle, int type, double cDir = 0) {
        mI.mult(Pnt3d(p),p);
        cout << " cylindricUnproject, p: " << p << ", lastAngle: " << lastAngle << ", type: " << type << ", cDir: " << cDir << endl;
        //cout << " -> R: " << R << ", r: " << p[0]*p[0]+p[1]*p[1] << endl;
        double h = p[2];
        double a = atan2(p[1]/R, p[0]/R);
        // rebaseAngle(a, lastAngle)

        if (a > lastAngle) cDir *= -1;

        if (abs(a) > pi-1e-3) { // ambigous point on +- pi
            if (type == 0 && lastAngle != 1000) a = lastAngle; // next point on line
            if (type == 1) { // circle
                if (cDir > 0) {
                    a *= -1;
                    cout << "  swap a: " << a << endl;
                }
            }
        }

        lastAngle = a;
        return Vec2d(a,h);
    };

    auto containsNan = [&](VRBRepBound& b) {
        for (auto p : b.points) if (isNan(p)) return true;
        return false;
    };

    if (type == "Plane") {
        //return 0;
        //cout << "make Plane, N bounds: " << bounds.size() << endl;
        Triangulator t;
        if (bounds.size() == 0) cout << "Warning: No bounds!\n";
        for (auto& b : bounds) {
            //if (b.points.size() == 0) cout << "Warning: No bound points for bound " << b.BRepType << endl;
            bool doPrint = containsNan(b);
            VRPolygon poly;

            for (auto pIn : b.points) {
                Vec3d pOut;
                if (doPrint) cout << " pIn: " << pIn << endl;
                mI.multFull(Pnt3d(pIn), pOut);
                if (doPrint) cout << " pOut: " << pOut << " " << mI[0] << ", " << mI[1] << ", " << mI[2] << ", " << mI[3] << endl;
                if (abs(pOut[2]) > 0.001) cout << " Error in VRBRepSurface::build Plane, p[2] not 0 (" << pOut[2] << ")! -> data loss" << endl;
                poly.addPoint(pOut);
            }
            if (!poly.isCCW()) poly.reverseOrder();
            t.add(poly);
        }

        auto g = t.compute(); // TODO: check about g??
        if (!g) return 0;
        if (!g->getMesh()->geo->getPositions()) cout << "NO MESH!\n";
        g->setMatrix(m);

        Vec3d nP = Vec3d(0, 0, -1+2*int(same_sense));
        GeoVectorPropertyMTRecPtr norms = g->getMesh()->geo->getNormals();
        for (uint i=0; i<norms->size(); i++) norms->setValue(nP, i);

        return g;
    }

    if (type == "Cylindrical_Surface") {
        // still old problem!
        //  IDEA: solve it in cylindricUnproject! the problem is for points on +/- pi, maybe solve it there!
        //          maybe move the point a bit left or right depending on the circle direction?
        //          but what with the lines?


        //cout << "Cylindrical_Surface" << endl;
        // feed the triangulator with unprojected points
        Triangulator triangulator;

        for (auto b : bounds) {
            VRPolygon poly;
            double lastAngle = 1000;
            //cout << " Bound" << endl << b.edgeEndsToString() << endl;

            //for (auto p : b.points) cout << "bound point: " << p << " -> " << cylindricUnproject(p) << endl;

            // TODO: this fails for any closed bounds around the cylinder!
            // idea:
            //  dont use the bound points but go through the edges
            //  use edge angle values instead of cartesian points
            //  if closed bound around cylinder detected, clip bound with lines at +/- PI, generating new bound on cylinder!

            // edge points rules:
            //  - first edge, push two points: push the point not found in next edge
            //  - push the point not found in previous edge

            // shift edges to have as first edge not a line
            if (b.edges[0].etype == "Line")  {
                int i0 = -1;
                for (int i=1; i<b.edges.size(); i++) {
                    if (b.edges[i].etype != "Line") {
                        i0 = i;
                        break;
                    }
                }

                vector<VRBRepEdge> edges;
                for (int i=i0; i<b.edges.size(); i++) edges.push_back(b.edges[i]);
                for (int i=0; i<i0; i++) edges.push_back(b.edges[i]);
                b.edges = edges;
            }

            //cout << " poly\n";
            for (auto& e : b.edges) {
                if (e.etype == "Circle") {
                    Vec3d cd = e.center->dir();
                    Vec3d cu = e.center->up();
                    double cDir = cu.dot(u);
                    /*a1 = e.a1; a2 = e.a2;
                    //cout << " circle " << Vec2d(a1,a2) << " -> ";
                    if (a2 < a1 && cd.dot(d) < 0) a2 += 2*Pi;*/

                    //if (a2 < a1) a2 += 2*Pi;

                    //cout << " circle " << p1 << " -> " << p2 << ",  " << cd << " " << cu << endl;
                    //rebaseAngle(a1, la);
                    //rebaseAngle(a2, la);
                    //cout << " -> " << cd.dot(d) << " " << cu.dot(u) << endl;

                    if (poly.size() == 0) {
                        Vec2d p1 = cylindricUnproject(e.EBeg, lastAngle, 1, cDir);
                        poly.addPoint(p1);
                    }

                    Vec2d p2 = cylindricUnproject(e.EEnd, lastAngle, 1, cDir);
                    poly.addPoint(p2);
                    /*cout << "  cp1 " << Vec2d(a1,h);
                    cout << " -> cp2 " << Vec2d(a2,h);
                    cout << ", EBeg: " << e.EBeg << " -> " << e.EEnd << endl;*/
                    //la = a2;
                    continue;
                }

                if (e.etype == "Line") {
                    Vec2d p1 = cylindricUnproject(e.EBeg, lastAngle, 0);
                    Vec2d p2 = cylindricUnproject(e.EEnd, lastAngle, 0);

                    //cout << " line " << Vec3d(p1[0],p2[0],la) << endl;
                    //rebaseAngle(p1[0], la);
                    //rebaseAngle(p2[0], la);
                    //cout << " -> " << Vec3d(p1[0],p2[0],la) << endl;

                    if (poly.size() == 0) poly.addPoint(p1);
                    poly.addPoint(p2);
                    /*cout << "  lp1 " << p1;
                    cout << " -> lp2 " << p2;
                    cout << ", EBeg: " << e.EBeg << " -> " << e.EEnd << endl;*/
                    //la = p2[0];
                    continue;
                }

                if (e.etype == "B_Spline_Curve_With_Knots") {
                    for (auto& p : e.points) {
                        Vec2d pc = cylindricUnproject(p, lastAngle, 2);
                        rebaseAngle(pc[0], lastAngle);

                        poly.addPoint(pc);
                        //cout << "  bsp " << pc << endl;
                        //la = pc[0];
                    }
                    continue;
                }

                cout << "Unhandled edge of type " << e.etype << endl;
            }
            //cout << endl << poly.toString() << endl;

            /*for(auto p : b.points) {
                Vec2d pc = cylindricUnproject(p);
                rebaseAngle(pc[0], la);
                //cout << " h " << h << " a " << a << " p " << p << " +2pi " << (la > -1000 && abs(a - la)>Pi) << endl;
                la = pc[0];
                poly.addPoint(pc);
            }*/

            /*VRPolygon poly2;
            for (int i=3; i<poly.size(); i+=2) { // start at second edge, increment every two points
                Vec2d e1p1 = poly.getPoint(i-3);
                Vec2d e1p2 = poly.getPoint(i-2);
                Vec2d e2p1 = poly.getPoint(i-1);
                Vec2d e2p2 = poly.getPoint(i);

                bool same11 = (sameVec(e1p1, e2p1) || sameVec(e1p1, e2p2));
                bool same12 = (sameVec(e1p2, e2p1) || sameVec(e1p2, e2p2));
                bool same21 = (sameVec(e2p1, e1p1) || sameVec(e2p1, e1p2));
                bool same22 = (sameVec(e2p2, e1p1) || sameVec(e2p2, e1p2));

                if (i == 3) {
                    if (same11) { poly2.addPoint(e1p2); poly2.addPoint(e1p1); }
                    else if (same12) { poly2.addPoint(e1p1); poly2.addPoint(e1p2); }
                    else cout << " Warning in Poly filter, same points: " << sameVec(e1p1, e2p1) << " " << sameVec(e1p1, e2p2) << " " << sameVec(e1p2, e2p1) << " " << sameVec(e1p2, e2p2) << endl;
                }

                if (same21) poly2.addPoint(e2p2);
                else if (same22) poly2.addPoint(e2p1);
                else cout << " Warning in Poly filter, same points: " << sameVec(e1p1, e2p1) << " " << sameVec(e1p1, e2p2) << " " << sameVec(e1p2, e2p1) << " " << sameVec(e1p2, e2p2) << endl;
            }*/

            //cout << " polygon: " << toString(poly2.get()) << endl;

            if (!poly.isCCW()) poly.reverseOrder();
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


        //GeoVectorPropertyMTRecPtr pos = g->getMesh()->geo->getPositions();
        //for (size_t i=0; i<pos->size(); i++) cout << " tesselated position: " << pos->getValue<Pnt3f>(i) << endl;

        // tesselate the result while projecting it back on the surface
        if (g) if (auto gg = g->getMesh()) { // TODO
            TriangleIterator it;
            VRGeoData nMesh;
            Vec3d n(0,-1,0);

            auto checkOrder = [&](Pnt3d p0, Pnt3d p1, Pnt3d p2) {
                float cp = (p1-p0).cross(p2-p0).dot(n);
                return (cp >= 0);
            };

            auto pushTri = [&](Pnt3d p1, Pnt3d p2, Pnt3d p3) {
                //cout << " pushTri " << p1 << "   " << p2 << "   " << p3 << endl;
                int a = nMesh.pushVert(p1,n);
                int b = nMesh.pushVert(p2,n);
                int c = nMesh.pushVert(p3,n);
                if (checkOrder(p1,p2,p3)) nMesh.pushTri(a,b,c);
                else nMesh.pushTri(a,c,b);
            };

            auto pushQuad = [&](Pnt3d p1, Pnt3d p2, Pnt3d p3, Pnt3d p4) {
                //cout << " pushQuad " << p1 << "   " << p2 << "   " << p3 << "   " << p4 << endl;
                int a = nMesh.pushVert(p1,n);
                int b = nMesh.pushVert(p2,n);
                int c = nMesh.pushVert(p3,n);
                int d = nMesh.pushVert(p4,n);
                if (checkOrder(p1,p2,p3)) nMesh.pushTri(a,b,c);
                else nMesh.pushTri(a,c,b);
                if (checkOrder(p2,p3,p4)) nMesh.pushTri(b,c,d);
                else nMesh.pushTri(b,d,c);
            };

            auto pushPen = [&](Pnt3d p1, Pnt3d p2, Pnt3d p3, Pnt3d p4, Pnt3d p5) {
                //cout << " pushPen " << p1 << "   " << p2 << "   " << p3 << "   " << p4 << "   " << p5 << endl;
                int a = nMesh.pushVert(p1,n);
                int b = nMesh.pushVert(p2,n);
                int c = nMesh.pushVert(p3,n);
                int d = nMesh.pushVert(p4,n);
                int e = nMesh.pushVert(p5,n);
                if (checkOrder(p1,p2,p3)) nMesh.pushTri(a,b,c);
                else nMesh.pushTri(a,c,b);
                if (checkOrder(p2,p3,p4)) nMesh.pushTri(b,c,d);
                else nMesh.pushTri(b,d,c);
                if (checkOrder(p2,p4,p5)) nMesh.pushTri(b,d,e);
                else nMesh.pushTri(b,e,d);
            };

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

                Vec3i pOrder(0,1,2); // get the order of the vertices
                for (int i=0; i<3; i++) { // max 3 sort steps
                    if (pSides[pOrder[0]] > pSides[pOrder[1]]) swap(pOrder[0], pOrder[1]);
                    else if (pSides[pOrder[0]] > pSides[pOrder[2]]) swap(pOrder[0], pOrder[2]);
                    else if (pSides[pOrder[1]] > pSides[pOrder[2]]) swap(pOrder[1], pOrder[2]);
                }
                //cout << " ordered vertices " << pOrder << "  " << pSides[pOrder[0]] << " " << pSides[pOrder[1]] << " " << pSides[pOrder[2]] << endl;

                // test first case: all vertices on the same cylinder face
                if (pSides[0] == pSides[1] && pSides[0] == pSides[2]) { // DEBUGGING
                    //cout << "  case 1" << endl;
                    pushTri(Pnt3d(tri.p[0]), Pnt3d(tri.p[1]), Pnt3d(tri.p[2]));
                    continue;
                }

                // test second case: all vertices on different cylinder faces
                if (pSides[0] != pSides[1] && pSides[0] != pSides[2] && pSides[1] != pSides[2]) {
                    //cout << "  case 2" << endl;
                    bool passed_middle = false;
                    for (uint i=0; i<sides.size(); i++) {
                        Vec2d s = sides[i];
                        if (i == 0) { // first triangle
                            int pi = pOrder[0]; // vertex index on that face
                            Pnt3d pv = Pnt3d(tri.p[pi]);
                            Vec3d pr1(s[1],0,0); // point on cylinder edge
                            Vec3d pr2(s[1],0,0); // point on cylinder edge
                            Vec3d vp1 = Vec3d(tri.v[pOrder[1]]); // vector to middle point
                            Vec3d vp2 = Vec3d(tri.v[pOrder[2]]); // vector to last point
                            pr1[2] = pv[2] + vp1[2]/vp1[0]*(s[1]-pv[0]);
                            pr2[2] = pv[2] + vp2[2]/vp2[0]*(s[1]-pv[0]);
                            pushTri(pv,pr1,pr2);
                            continue;
                        }

                        if (i == sides.size()-1) { // last triangle
                            int pi = pOrder[2]; // vertex index on that face
                            Pnt3d pv = Pnt3d(tri.p[pi]);
                            Vec3d pr1(s[0],0,0); // point on cylinder edge
                            Vec3d pr2(s[0],0,0); // point on cylinder edge
                            Vec3d vp1 = Vec3d(tri.v[pOrder[1]]); // vector to middle point
                            Vec3d vp2 = Vec3d(tri.v[pOrder[0]]); // vector to last point
                            pr1[2] = pv[2] + vp1[2]/vp1[0]*(s[0]-pv[0]);
                            pr2[2] = pv[2] + vp2[2]/vp2[0]*(s[0]-pv[0]);
                            pushTri(pv,pr1,pr2);
                            continue;
                        }

                        if (int(i) == pSides[pOrder[1]]) { // pentagon in the middle
                            Pnt3d pv = Pnt3d(tri.p[pOrder[1]]); // point in the middle
                            Vec3d pr11(s[0],0,0); // point on cylinder edge
                            Vec3d pr12(s[0],0,0); // point on cylinder edge
                            Vec3d pr21(s[1],0,0); // point on cylinder edge
                            Vec3d pr22(s[1],0,0); // point on cylinder edge
                            Vec3d vp1 = Vec3d(tri.v[pOrder[0]]); // vector from middle to last
                            Vec3d vp2 = Vec3d(tri.v[pOrder[1]]); // vector from first to last
                            Vec3d vp3 = Vec3d(tri.v[pOrder[2]]); // vector from first to middle
                            Pnt3d pv1 = Vec3d(tri.p[pOrder[0]]); // first vertex
                            Pnt3d pv2 = Vec3d(tri.p[pOrder[2]]); // last vertex
                            pr11[2] = pv1[2] + vp2[2]/vp2[0]*(s[0]-pv1[0]);
                            pr12[2] = pv1[2] + vp3[2]/vp3[0]*(s[0]-pv1[0]);
                            pr21[2] = pv1[2] + vp2[2]/vp2[0]*(s[1]-pv1[0]);
                            pr22[2] = pv[2] + vp1[2]/vp1[0]*(s[1]-pv[0]);
                            pushPen(pr11, pr12, pr21, pr22, pv);
                            passed_middle = true;
                            continue;
                        }

                        // middle quad
                        Vec3d pr11(s[0],0,0); // point on cylinder edge
                        Vec3d pr12(s[0],0,0); // point on cylinder edge
                        Vec3d pr21(s[1],0,0); // point on cylinder edge
                        Vec3d pr22(s[1],0,0); // point on cylinder edge
                        Vec3d vp1, vp2;
                        Pnt3d pv1, pv2;
                        if (!passed_middle) {
                            vp1 = Vec3d(tri.v[pOrder[1]]); // vector to middle point
                            vp2 = Vec3d(tri.v[pOrder[2]]); // vector to last point
                            pv1 = Pnt3d(tri.p[pOrder[2]]); // vertex on that face
                            pv2 = Pnt3d(tri.p[pOrder[1]]); // vertex on that face
                        } else {
                            vp1 = Vec3d(tri.v[pOrder[1]]); // vector from first point
                            vp2 = Vec3d(tri.v[pOrder[0]]); // vector from middle point
                            pv1 = Pnt3d(tri.p[pOrder[0]]); // vertex on that face
                            pv2 = Pnt3d(tri.p[pOrder[1]]); // vertex on that face
                        }
                        pr11[2] = pv1[2] + vp1[2]/vp1[0]*(s[0]-pv1[0]);
                        pr12[2] = pv2[2] + vp2[2]/vp2[0]*(s[0]-pv2[0]);
                        pr21[2] = pv1[2] + vp1[2]/vp1[0]*(s[1]-pv1[0]);
                        pr22[2] = pv2[2] + vp2[2]/vp2[0]*(s[1]-pv2[0]);
                        pushQuad(pr11, pr12, pr21, pr22);
                    }
                    continue;
                }

                // case 3
                if (pSides[pOrder[0]] == pSides[pOrder[1]]) {
                    //cout << "  case 3" << endl;
                    for (uint i=0; i<sides.size(); i++) {
                        Vec2d s = sides[i];
                        if (i == 0) { // first quad
                            Pnt3d pv1 = Pnt3d(tri.p[pOrder[0]]); // vertex on that face
                            Pnt3d pv2 = Pnt3d(tri.p[pOrder[1]]); // vertex on that face
                            Vec3d pr1(s[1],0,0); // point on cylinder edge
                            Vec3d pr2(s[1],0,0); // point on cylinder edge
                            Vec3d vp1 = Vec3d(tri.v[pOrder[1]]); // vector to last point
                            Vec3d vp2 = Vec3d(tri.v[pOrder[0]]); // vector to last point
                            pr1[2] = pv1[2] + vp1[2]/vp1[0]*(s[1]-pv1[0]);
                            pr2[2] = pv2[2] + vp2[2]/vp2[0]*(s[1]-pv2[0]);
                            pushQuad(pv1, pv2, pr1, pr2);
                            continue;
                        }
                        if (i == sides.size()-1) { // last triangle
                            Pnt3d pv = Pnt3d(tri.p[pOrder[2]]); // vertex on that face
                            Vec3d pr1(s[0],0,0); // point on cylinder edge
                            Vec3d pr2(s[0],0,0); // point on cylinder edge
                            Vec3d vp1 = Vec3d(tri.v[pOrder[1]]); // vector to middle point
                            Vec3d vp2 = Vec3d(tri.v[pOrder[0]]); // vector to last point
                            pr1[2] = pv[2] + vp1[2]/vp1[0]*(s[0]-pv[0]);
                            pr2[2] = pv[2] + vp2[2]/vp2[0]*(s[0]-pv[0]);
                            pushTri(pv,pr1,pr2);
                            continue;
                        }

                        Pnt3d pv1 = Pnt3d(tri.p[pOrder[0]]); // vertex on that face
                        Pnt3d pv2 = Pnt3d(tri.p[pOrder[1]]); // vertex on that face
                        Vec3d pr11(s[0],0,0); // point on cylinder edge
                        Vec3d pr12(s[0],0,0); // point on cylinder edge
                        Vec3d pr21(s[1],0,0); // point on cylinder edge
                        Vec3d pr22(s[1],0,0); // point on cylinder edge
                        Vec3d vp1 = Vec3d(tri.v[pOrder[1]]); // vector to last point
                        Vec3d vp2 = Vec3d(tri.v[pOrder[0]]); // vector to last point
                        pr11[2] = pv1[2] + vp1[2]/vp1[0]*(s[0]-pv1[0]);
                        pr12[2] = pv2[2] + vp2[2]/vp2[0]*(s[0]-pv2[0]);
                        pr21[2] = pv1[2] + vp1[2]/vp1[0]*(s[1]-pv1[0]);
                        pr22[2] = pv2[2] + vp2[2]/vp2[0]*(s[1]-pv2[0]);
                        pushQuad(pr11, pr12, pr21, pr22);
                    }
                    continue;
                }

                // case 4
                if (pSides[pOrder[1]] == pSides[pOrder[2]]) {
                    //cout << "  case 4" << endl;
                    for (uint i=0; i<sides.size(); i++) {
                        Vec2d s = sides[i];

                        Pnt3d pv = Pnt3d(tri.p[pOrder[0]]); // vertex on that face
                        Pnt3d pv1 = Pnt3d(tri.p[pOrder[1]]); // vertex on that face
                        Pnt3d pv2 = Pnt3d(tri.p[pOrder[2]]); // vertex on that face

                        if (i == 0) { // first triangle
                            Vec3d pr1(s[1],0,0); // point on cylinder edge
                            Vec3d pr2(s[1],0,0); // point on cylinder edge
                            Vec3d vp1 = Vec3d(tri.v[pOrder[1]]); // vector to middle point
                            Vec3d vp2 = Vec3d(tri.v[pOrder[2]]); // vector to last point
                            pr1[2] = pv[2] + vp1[2]/vp1[0]*(s[1]-pv[0]);
                            pr2[2] = pv[2] + vp2[2]/vp2[0]*(s[1]-pv[0]);
                            pushTri(pv,pr1,pr2);
                            continue;
                        }
                        if (i == sides.size()-1) { // last quad
                            Vec3d pr1(s[0],0,0); // point on cylinder edge
                            Vec3d pr2(s[0],0,0); // point on cylinder edge
                            Vec3d vp1 = Vec3d(tri.v[pOrder[2]]); // vector to last point
                            Vec3d vp2 = Vec3d(tri.v[pOrder[1]]); // vector to last point
                            pr1[2] = pv1[2] + vp1[2]/vp1[0]*(s[0]-pv1[0]);
                            pr2[2] = pv2[2] + vp2[2]/vp2[0]*(s[0]-pv2[0]);
                            pushQuad(pv1, pv2, pr1, pr2);
                            continue;
                        }

                        Vec3d pr11(s[0],0,0); // point on cylinder edge
                        Vec3d pr12(s[0],0,0); // point on cylinder edge
                        Vec3d pr21(s[1],0,0); // point on cylinder edge
                        Vec3d pr22(s[1],0,0); // point on cylinder edge
                        Vec3d vp1 = Vec3d(tri.v[pOrder[2]]); // vector to last point
                        Vec3d vp2 = Vec3d(tri.v[pOrder[1]]); // vector to last point
                        pr11[2] = pv1[2] + vp1[2]/vp1[0]*(s[0]-pv1[0]);
                        pr12[2] = pv2[2] + vp2[2]/vp2[0]*(s[0]-pv2[0]);
                        pr21[2] = pv1[2] + vp1[2]/vp1[0]*(s[1]-pv1[0]);
                        pr22[2] = pv2[2] + vp2[2]/vp2[0]*(s[1]-pv2[0]);
                        pushQuad(pr11, pr12, pr21, pr22);
                    }
                    continue;
                }

                cout << " unhandled triangle " << endl;
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

                    Vec2d side = getSide(p[0]);
                    Vec3d A = Vec3d(R*cos(side[0]), R*sin(side[0]), 0);
                    Vec3d B = Vec3d(R*cos(side[1]), R*sin(side[1]), 0);
                    Vec3d D = B-A;
                    D.normalize();

                    float t = (A[0]*D[1] - A[1]*D[0]) / (n[0]*D[1] - n[1]*D[0]);

                    //cout << "p: " << a/Pi*180 << " AB: " << side*(180/Pi) << " s: " << getSideN(a) << endl;
                    //if (a < side[0] || a > side[1]) cout << "   AAAH\n"; // TODO: check this out!

                    p[2] = h;
                    p[1] = n[1]*t;
                    p[0] = n[0]*t;

                    pos->setValue(p, i);
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
        cout << " BUILD B_Spline_Surface" << endl;

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

        int res = (Ncurv - 1)*0.5;
        float T = 1; //knots[knots.size()-1] - knots[0];
        for (int i=0; i<=res; i++) {
            float u = i*T/res;
            for (int j=0; j<=res; j++) {
                float v = j*T/res;
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

        /*cout << "B_Spline_Surface_with_knots du " << degu << " dv " << degv << "  pw " << cpoints.width << " ph " << cpoints.height << endl;
        cout << " knotsu ";
        for (auto ku : knotsu) cout << " " << ku;
        cout << endl;
        cout << " knotsv ";
        for (auto kv : knotsv) cout << " " << kv;
        cout << endl;
        cout << " points\n";
        for (int i = 0; i < cpoints.height; i++) {
            for (int j = 0; j < cpoints.width; j++) {
                cout << " p" << j << i << ": " << cpoints.get(j,i);
            }
            cout << endl;
        }*/

        if (knotsu.size() == 0 || knotsv.size() == 0) return 0;

        // BSpline mesh
        map<int, map<int, int> > ids;
        int res = (Ncurv - 1)*0.5;
        float Tu = knotsu[knotsu.size()-1] - knotsu[0];
        float Tv = knotsv[knotsv.size()-1] - knotsv[0];
        for (int i=0; i<=res; i++) {
            float u = knotsu[0]+i*Tu/res;
            for (int j=0; j<=res; j++) {
                float v = knotsv[0]+j*Tv/res;
                Vec3d p = isWeighted ? BSpline(u,v, degu, degv, cpoints, knotsu, knotsv, weights) : BSpline(u,v, degu, degv, cpoints, knotsu, knotsv);
                Vec3d n = isWeighted ? BSplineNorm(u,v, degu, degv, cpoints, knotsu, knotsv, weights) : BSplineNorm(u,v, degu, degv, cpoints, knotsu, knotsv);
                if (same_sense) n *= -1;
                ids[i][j] = nMesh.pushVert(p,n);

                if (i > 0 && j > 0) nMesh.pushQuad(ids[i][j], ids[i][j-1], ids[i-1][j-1], ids[i-1][j]);
            }
        }

        auto g = nMesh.asGeometry("BSpline");
        if (g) g->setMatrix(m);
        if (g && g->getMesh() && g->getMesh()->geo->getPositions() && g->getMesh()->geo->getPositions()->size() > 0) return g;
        return 0;
    }

    if (type == "Conical_Surface") {
        //cout << "Conical_Surface" << endl;
        return 0;
    }

    if (type == "Spherical_Surface") {
        //cout << "Spherical_Surface" << endl;
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

