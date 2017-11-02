#include "VRBRepSurface.h"
#include "core/math/triangulator.h"
#include "core/math/pose.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/OSGGeometry.h"
#include "core/objects/geometry/VRGeoData.h"
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

VRGeometryPtr VRBRepSurface::build(string type) {
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

    auto cylindricUnproject = [&](Vec3d& p) {
        mI.mult(Pnt3d(p),p);
        float h = p[2];
        p[0] /= R; p[1] /= R;
        float a = atan2(p[1], p[0]);
        return Vec2d(a,h);
    };

    auto rebaseAngle = [&](double& a, double& la) {
        if (la > -1000 && abs(a - la) > Pi) {
            if (a - la > Pi) a -= 2*Pi;
            else a += 2*Pi;
        }
    };

    if (type == "Plane") {
        //return 0;
        Triangulator t;
        if (bounds.size() == 0) cout << "Warning: No bounds!\n";
        for (auto b : bounds) {
            //if (b.points.size() == 0) cout << "Warning: No bound points for bound " << b.BRepType << endl;
            VRPolygon poly;
            for(auto p : b.points) {
                mI.mult(Pnt3d(p),p);
                poly.addPoint(Vec2d(p[0], p[1]));
                //cout << Vec2d(p[0], p[1]) << endl;
            }
            if (!poly.isCCW()) poly.reverseOrder();
            t.add(poly);
        }

        auto g = t.compute(); // TODO: check about g??
        if (!g->getMesh()->geo->getPositions()) cout << "NO MESH!\n";
        g->setMatrix(m);
        return g;
    }

    if (type == "Cylindrical_Surface") {

        static int i=0; i++;
        if (i != 22 && i != 23) return 0; // 22,23

        cout << "Cylindrical_Surface\n";
        // feed the triangulator with unprojected points
        Triangulator t;

        for (auto b : bounds) {
            VRPolygon poly;
            double la = -1001;
            cout << "Bound\n";
            cout << b.edgeEndsToString() << endl;

            // TODO: this fails for any closed bounds around the cylinder!
            // idea:
            //  dont use the bound points but go through the edges
            //  use edge angle values insteas of cartesian points
            //  if closed bound around cylinder detected, clip bound with lines at +/- PI, generating new bound on cylinder!

            cout << " poly\n";
            for (auto& e : b.edges) {
                if (e.etype == "Circle") {
                    float h = cylindricUnproject(e.EBeg)[1];
                    double a1, a2;
                    a1 = e.a1; a2 = e.a2;
                    Vec3d cd = e.center->dir();
                    Vec3d cu = e.center->up();
                    if (a2 < a1 && cd.dot(d) < 0) a2 += 2*Pi;


                    //a1 = cylindricUnproject(e.EBeg)[0];
                    //a2 = cylindricUnproject(e.EEnd)[0];
                    //if (a2 < a1) a2 += 2*Pi;

                    cout << " circle " << Vec3d(a1,a2,la) << endl;
                    rebaseAngle(a1, la);
                    rebaseAngle(a2, la);
                    cout << " circle " << cd.dot(d) << " " << cu.dot(u) << endl;

                    if (poly.size() == 0) poly.addPoint(Vec2d(a1,h));
                    poly.addPoint(Vec2d(a2,h));
                    cout << "cp1 " << Vec2d(a1,h) << endl;
                    cout << "cp2 " << Vec2d(a2,h) << endl;
                    la = a2;
                    continue;
                }

                if (e.etype == "Line") {
                    Vec2d p1 = cylindricUnproject(e.EBeg);
                    Vec2d p2 = cylindricUnproject(e.EEnd);

                    cout << " line " << Vec3d(p1[0],p2[0],la) << endl;
                    rebaseAngle(p1[0], la);
                    rebaseAngle(p2[0], la);
                    cout << " line " << Vec3d(p1[0],p2[0],la) << endl;

                    if (poly.size() == 0) poly.addPoint(p1);
                    poly.addPoint(p2);
                    cout << "lp1 " << p1 << endl;
                    cout << "lp2 " << p2 << endl;
                    la = p2[0];
                    continue;
                }

                if (e.etype == "B_Spline_Curve_With_Knots") {
                    for (auto& p : e.points) {
                        Vec2d pc = cylindricUnproject(p);
                        rebaseAngle(pc[0], la);

                        poly.addPoint(pc);
                        cout << "bsp " << pc << endl;
                        la = pc[0];
                    }
                    continue;
                }

                cout << "Unhandled edge of type " << e.etype << endl;
            }
            cout << endl;
            cout << poly.toString() << endl;

            /*for(auto p : b.points) {
                Vec2d pc = cylindricUnproject(p);
                rebaseAngle(pc[0], la);
                //cout << " h " << h << " a " << a << " p " << p << " +2pi " << (la > -1000 && abs(a - la)>Pi) << endl;
                la = pc[0];
                poly.addPoint(pc);
            }*/

            if (!poly.isCCW()) poly.reverseOrder();
            t.add(poly);
        }

        auto g = t.compute();
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
        if (g and 1) if (auto gg = g->getMesh()) {
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

            auto pushPen = [&](Pnt3d p1, Pnt3d p2, Pnt3d p3, Pnt3d p4, Pnt3d p5) {
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
                triangle t(it);
                if (t.A < 1e-6) continue; // ignore flat triangles

                Vec2d xs = getXsize(t.p); // triangle width
                vector<float> rays = angleFrame(xs[0], xs[1]);

                vector<Vec2d> sides;
                Vec3i pSides;
                //cout << " triangle size in x " << xs*180/Pi << " " << rays.size() << endl;
                //cout << " triangle: " << p[0] << "   " << p[1] << "   " << p[2] << endl;
                for (uint i=1; i<rays.size(); i++) {
                    auto s = Vec2d(rays[i-1], rays[i]);
                    //cout << "  side " << s*180/Pi << endl;
                    sides.push_back( s ); // get all cylinder faces
                    for (int j=0; j<3; j++) { // find out on what cylinder face each vertex lies
                        if (t.p[j][0] >= rays[i-1] && t.p[j][0] <= rays[i]) {
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
                    pushTri(Pnt3d(t.p[0]),Pnt3d(t.p[1]),Pnt3d(t.p[2]));
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
                            Pnt3d pv = Pnt3d(t.p[pi]);
                            Vec3d pr1(s[1],0,0); // point on cylinder edge
                            Vec3d pr2(s[1],0,0); // point on cylinder edge
                            Vec3d vp1 = Vec3d(t.v[pOrder[1]]); // vector to middle point
                            Vec3d vp2 = Vec3d(t.v[pOrder[2]]); // vector to last point
                            pr1[1] = pv[1] + vp1[1]/vp1[0]*(s[1]-pv[0]);
                            pr2[1] = pv[1] + vp2[1]/vp2[0]*(s[1]-pv[0]);
                            pushTri(pv,pr1,pr2);
                            continue;
                        }

                        if (i == sides.size()-1) { // last triangle
                            int pi = pOrder[2]; // vertex index on that face
                            Pnt3d pv = Pnt3d(t.p[pi]);
                            Vec3d pr1(s[0],0,0); // point on cylinder edge
                            Vec3d pr2(s[0],0,0); // point on cylinder edge
                            Vec3d vp1 = Vec3d(t.v[pOrder[1]]); // vector to middle point
                            Vec3d vp2 = Vec3d(t.v[pOrder[0]]); // vector to last point
                            pr1[1] = pv[1] + vp1[1]/vp1[0]*(s[0]-pv[0]);
                            pr2[1] = pv[1] + vp2[1]/vp2[0]*(s[0]-pv[0]);
                            pushTri(pv,pr1,pr2);
                            continue;
                        }

                        if (int(i) == pSides[pOrder[1]]) { // pentagon in the middle
                            Pnt3d pv = Pnt3d(t.p[pOrder[1]]); // point in the middle
                            Vec3d pr11(s[0],0,0); // point on cylinder edge
                            Vec3d pr12(s[0],0,0); // point on cylinder edge
                            Vec3d pr21(s[1],0,0); // point on cylinder edge
                            Vec3d pr22(s[1],0,0); // point on cylinder edge
                            Vec3d vp1 = Vec3d(t.v[pOrder[0]]); // vector from middle to last
                            Vec3d vp2 = Vec3d(t.v[pOrder[1]]); // vector from first to last
                            Vec3d vp3 = Vec3d(t.v[pOrder[2]]); // vector from first to middle
                            Pnt3d pv1 = Vec3d(t.p[pOrder[0]]); // first vertex
                            Pnt3d pv2 = Vec3d(t.p[pOrder[2]]); // last vertex
                            pr11[1] = pv1[1] + vp2[1]/vp2[0]*(s[0]-pv1[0]);
                            pr12[1] = pv1[1] + vp3[1]/vp3[0]*(s[0]-pv1[0]);
                            pr21[1] = pv1[1] + vp2[1]/vp2[0]*(s[1]-pv1[0]);
                            pr22[1] = pv[1] + vp1[1]/vp1[0]*(s[1]-pv[0]);
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
                            vp1 = Vec3d(t.v[pOrder[1]]); // vector to middle point
                            vp2 = Vec3d(t.v[pOrder[2]]); // vector to last point
                            pv1 = Pnt3d(t.p[pOrder[2]]); // vertex on that face
                            pv2 = Pnt3d(t.p[pOrder[1]]); // vertex on that face
                        } else {
                            vp1 = Vec3d(t.v[pOrder[1]]); // vector from first point
                            vp2 = Vec3d(t.v[pOrder[0]]); // vector from middle point
                            pv1 = Pnt3d(t.p[pOrder[0]]); // vertex on that face
                            pv2 = Pnt3d(t.p[pOrder[1]]); // vertex on that face
                        }
                        pr11[1] = pv1[1] + vp1[1]/vp1[0]*(s[0]-pv1[0]);
                        pr12[1] = pv2[1] + vp2[1]/vp2[0]*(s[0]-pv2[0]);
                        pr21[1] = pv1[1] + vp1[1]/vp1[0]*(s[1]-pv1[0]);
                        pr22[1] = pv2[1] + vp2[1]/vp2[0]*(s[1]-pv2[0]);
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
                            Pnt3d pv1 = Pnt3d(t.p[pOrder[0]]); // vertex on that face
                            Pnt3d pv2 = Pnt3d(t.p[pOrder[1]]); // vertex on that face
                            Vec3d pr1(s[1],0,0); // point on cylinder edge
                            Vec3d pr2(s[1],0,0); // point on cylinder edge
                            Vec3d vp1 = Vec3d(t.v[pOrder[1]]); // vector to last point
                            Vec3d vp2 = Vec3d(t.v[pOrder[0]]); // vector to last point
                            pr1[1] = pv1[1] + vp1[1]/vp1[0]*(s[1]-pv1[0]);
                            pr2[1] = pv2[1] + vp2[1]/vp2[0]*(s[1]-pv2[0]);
                            pushQuad(pv1, pv2, pr1, pr2);
                            continue;
                        }
                        if (i == sides.size()-1) { // last triangle
                            Pnt3d pv = Pnt3d(t.p[pOrder[2]]); // vertex on that face
                            Vec3d pr1(s[0],0,0); // point on cylinder edge
                            Vec3d pr2(s[0],0,0); // point on cylinder edge
                            Vec3d vp1 = Vec3d(t.v[pOrder[1]]); // vector to middle point
                            Vec3d vp2 = Vec3d(t.v[pOrder[0]]); // vector to last point
                            pr1[1] = pv[1] + vp1[1]/vp1[0]*(s[0]-pv[0]);
                            pr2[1] = pv[1] + vp2[1]/vp2[0]*(s[0]-pv[0]);
                            pushTri(pv,pr1,pr2);
                            continue;
                        }

                        Pnt3d pv1 = Pnt3d(t.p[pOrder[0]]); // vertex on that face
                        Pnt3d pv2 = Pnt3d(t.p[pOrder[1]]); // vertex on that face
                        Vec3d pr11(s[0],0,0); // point on cylinder edge
                        Vec3d pr12(s[0],0,0); // point on cylinder edge
                        Vec3d pr21(s[1],0,0); // point on cylinder edge
                        Vec3d pr22(s[1],0,0); // point on cylinder edge
                        Vec3d vp1 = Vec3d(t.v[pOrder[1]]); // vector to last point
                        Vec3d vp2 = Vec3d(t.v[pOrder[0]]); // vector to last point
                        pr11[1] = pv1[1] + vp1[1]/vp1[0]*(s[0]-pv1[0]);
                        pr12[1] = pv2[1] + vp2[1]/vp2[0]*(s[0]-pv2[0]);
                        pr21[1] = pv1[1] + vp1[1]/vp1[0]*(s[1]-pv1[0]);
                        pr22[1] = pv2[1] + vp2[1]/vp2[0]*(s[1]-pv2[0]);
                        pushQuad(pr11, pr12, pr21, pr22);
                    }
                    continue;
                }

                // case 4
                if (pSides[pOrder[1]] == pSides[pOrder[2]]) {
                    //cout << "  case 4" << endl;
                    for (uint i=0; i<sides.size(); i++) {
                        Vec2d s = sides[i];

                        Pnt3d pv = Pnt3d(t.p[pOrder[0]]); // vertex on that face
                        Pnt3d pv1 = Pnt3d(t.p[pOrder[1]]); // vertex on that face
                        Pnt3d pv2 = Pnt3d(t.p[pOrder[2]]); // vertex on that face

                        if (i == 0) { // first triangle
                            Vec3d pr1(s[1],0,0); // point on cylinder edge
                            Vec3d pr2(s[1],0,0); // point on cylinder edge
                            Vec3d vp1 = Vec3d(t.v[pOrder[1]]); // vector to middle point
                            Vec3d vp2 = Vec3d(t.v[pOrder[2]]); // vector to last point
                            pr1[1] = pv[1] + vp1[1]/vp1[0]*(s[1]-pv[0]);
                            pr2[1] = pv[1] + vp2[1]/vp2[0]*(s[1]-pv[0]);
                            pushTri(pv,pr1,pr2);
                            continue;
                        }
                        if (i == sides.size()-1) { // last quad
                            Vec3d pr1(s[0],0,0); // point on cylinder edge
                            Vec3d pr2(s[0],0,0); // point on cylinder edge
                            Vec3d vp1 = Vec3d(t.v[pOrder[2]]); // vector to last point
                            Vec3d vp2 = Vec3d(t.v[pOrder[1]]); // vector to last point
                            pr1[1] = pv1[1] + vp1[1]/vp1[0]*(s[0]-pv1[0]);
                            pr2[1] = pv2[1] + vp2[1]/vp2[0]*(s[0]-pv2[0]);
                            pushQuad(pv1, pv2, pr1, pr2);
                            continue;
                        }

                        Vec3d pr11(s[0],0,0); // point on cylinder edge
                        Vec3d pr12(s[0],0,0); // point on cylinder edge
                        Vec3d pr21(s[1],0,0); // point on cylinder edge
                        Vec3d pr22(s[1],0,0); // point on cylinder edge
                        Vec3d vp1 = Vec3d(t.v[pOrder[2]]); // vector to last point
                        Vec3d vp2 = Vec3d(t.v[pOrder[1]]); // vector to last point
                        pr11[1] = pv1[1] + vp1[1]/vp1[0]*(s[0]-pv1[0]);
                        pr12[1] = pv2[1] + vp2[1]/vp2[0]*(s[0]-pv2[0]);
                        pr21[1] = pv1[1] + vp1[1]/vp1[0]*(s[1]-pv1[0]);
                        pr22[1] = pv2[1] + vp2[1]/vp2[0]*(s[1]-pv2[0]);
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
        }

        if (g) g->setMatrix(m);
        if (g && g->getMesh() && g->getMesh()->geo->getPositions() && g->getMesh()->geo->getPositions()->size() > 0) return g;
        return 0;
    }

    if (type == "B_Spline_Surface") {
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
                ids[i][j] = nMesh.pushVert(p,n);

                if (i > 0 && j > 0) nMesh.pushQuad(ids[i][j], ids[i][j-1], ids[i-1][j-1], ids[i-1][j]);
            }
        }

        auto g = nMesh.asGeometry("BSpline");
        if (g) g->setMatrix(m);
        if (g && g->getMesh() && g->getMesh()->geo->getPositions() && g->getMesh()->geo->getPositions()->size() > 0) return g;
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

