#include "VRBRepSurface.h"
#include "core/math/triangulator.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRGeoData.h"
#include <OpenSG/OSGTriangleIterator.h>

using namespace OSG;

VRBRepSurface::VRBRepSurface() {}

VRGeometryPtr VRBRepSurface::build(string type) {
    //cout << "VRSTEP::Surface build " << ID << " " << type << endl;

    Matrix m = trans.asMatrix();
    Matrix mI = m;
    mI.invert();

    if (type == "Plane") {
        Triangulator t;
        for (auto b : bounds) {
            polygon poly;
            for(auto p : b.points) {
                mI.mult(Pnt3f(p),p);
                poly.addPoint(Vec2f(p[0], p[1]));
            }
            if (!poly.isCCW()) poly.turn();
            t.add(poly);
        }

        auto g = t.compute();
        g->setMatrix(m);
        return g;
    }

    if (type == "Cylindrical_Surface") {

        // feed the triangulator with unprojected points
        Triangulator t;
        for (auto b : bounds) {
            polygon poly;
            //cout << "Bound" << endl;
            float la = -1001;
            for(auto p : b.points) {
                //cout << " p1 " << p << endl;
                mI.mult(Pnt3f(p),p);
                //cout << " p2 " << p << endl;
                float h = p[2];
                float a = atan2(p[1]/R, p[0]/R);
                if (la > -1000 && abs(a - la)>Pi) a += 2*Pi;
                la = a;
                //cout << h << "  " << a << endl;
                poly.addPoint(Vec2f(a, h));
            }
            if (!poly.isCCW()) poly.turn();
            t.add(poly);
        }
        auto g = t.compute();

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
            Vec2f res(pnts[0][0], pnts[0][0]);
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
            Vec3f n(0,0,1);

            auto checkOrder = [&](Pnt3f p0, Pnt3f p1, Pnt3f p2) {
                float cp = (p1-p0).cross(p2-p0).dot(n);
                return (cp >= 0);
            };

            auto pushTri = [&](Pnt3f p1, Pnt3f p2, Pnt3f p3) {
                int a = nMesh.pushVert(p1,n);
                int b = nMesh.pushVert(p2,n);
                int c = nMesh.pushVert(p3,n);
                if (checkOrder(p1,p2,p3)) nMesh.pushTri(a,b,c);
                else nMesh.pushTri(a,c,b);
            };

            auto pushQuad = [&](Pnt3f p1, Pnt3f p2, Pnt3f p3, Pnt3f p4) {
                int a = nMesh.pushVert(p1,n);
                int b = nMesh.pushVert(p2,n);
                int c = nMesh.pushVert(p3,n);
                int d = nMesh.pushVert(p4,n);
                if (checkOrder(p1,p2,p3)) nMesh.pushTri(a,b,c);
                else nMesh.pushTri(a,c,b);
                if (checkOrder(p2,p3,p4)) nMesh.pushTri(b,c,d);
                else nMesh.pushTri(b,d,c);
            };

            auto pushPen = [&](Pnt3f p1, Pnt3f p2, Pnt3f p3, Pnt3f p4, Pnt3f p5) {
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

            for (it = TriangleIterator(gg); !it.isAtEnd() ;++it) {
                vector<Pnt3f> p(3);
                vector<Vec3f> v(3);
                Vec3i vi = Vec3i(it.getPositionIndex(0), it.getPositionIndex(1), it.getPositionIndex(2));
                for (int i=0; i<3; i++) p[i] = it.getPosition(i);
                v[0] = p[2]-p[1]; v[1] = p[2]-p[0]; v[2] = p[1]-p[0];

                float A = v[0].cross(v[1]).squareLength();
                if (A < 1e-6) continue; // ignore flat triangles

                Vec2f xs = getXsize(p);
                float da = 2*Pi/Ncurv;
                vector<float> rays;
                vector<Vec2f> sides;
                Vec3i pSides;

                //for (int i = floor(xs[0]/da); i <= ceil(xs[1]/da); i++) rays.push_back(i*da); // get all cylinder edges (rays)
                rays = angleFrame(xs[0], xs[1]);

                //cout << " triangle size in x " << xs << " " << rays.size() << endl;
                //cout << " triangle: " << p[0] << "   " << p[1] << "   " << p[2] << endl;
                for (int i=1; i<rays.size(); i++) {
                    sides.push_back( Vec2f(rays[i-1], rays[i]) ); // get all cylinder faces
                    for (int j=0; j<3; j++) { // find out on what cylinder face each vertex lies
                        if (p[j][0] >= rays[i-1] && p[j][0] <= rays[i]) {
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
                if (pSides[0] == pSides[1] && pSides[0] == pSides[2]) {
                    //cout << "  case 1" << endl;
                    pushTri(p[0],p[1],p[2]);
                    continue;
                }

                // test second case: all vertices on different cylinder faces
                if (pSides[0] != pSides[1] && pSides[0] != pSides[2] && pSides[1] != pSides[2]) {
                    //cout << "  case 2" << endl;
                    bool passed_middle = false;
                    for (int i=0; i<sides.size(); i++) {
                        Vec2f s = sides[i];
                        if (i == 0) { // first triangle
                            int pi = pOrder[0]; // vertex index on that face
                            Pnt3f pv = p[pi];
                            Vec3f pr1(s[1],0,0); // point on cylinder edge
                            Vec3f pr2(s[1],0,0); // point on cylinder edge
                            Vec3f vp1 = v[pOrder[1]]; // vector to middle point
                            Vec3f vp2 = v[pOrder[2]]; // vector to last point
                            pr1[1] = pv[1] + vp1[1]/vp1[0]*(s[1]-pv[0]);
                            pr2[1] = pv[1] + vp2[1]/vp2[0]*(s[1]-pv[0]);
                            pushTri(pv,pr1,pr2);
                            continue;
                        }

                        if (i == sides.size()-1) { // last triangle
                            int pi = pOrder[2]; // vertex index on that face
                            Pnt3f pv = p[pi];
                            Vec3f pr1(s[0],0,0); // point on cylinder edge
                            Vec3f pr2(s[0],0,0); // point on cylinder edge
                            Vec3f vp1 = v[pOrder[1]]; // vector to middle point
                            Vec3f vp2 = v[pOrder[0]]; // vector to last point
                            pr1[1] = pv[1] + vp1[1]/vp1[0]*(s[0]-pv[0]);
                            pr2[1] = pv[1] + vp2[1]/vp2[0]*(s[0]-pv[0]);
                            pushTri(pv,pr1,pr2);
                            continue;
                        }

                        if (i == pSides[pOrder[1]]) { // pentagon in the middle
                            Pnt3f pv = p[pOrder[1]]; // point in the middle
                            Vec3f pr11(s[0],0,0); // point on cylinder edge
                            Vec3f pr12(s[0],0,0); // point on cylinder edge
                            Vec3f pr21(s[1],0,0); // point on cylinder edge
                            Vec3f pr22(s[1],0,0); // point on cylinder edge
                            Vec3f vp1 = v[pOrder[0]]; // vector from middle to last
                            Vec3f vp2 = v[pOrder[1]]; // vector from first to last
                            Vec3f vp3 = v[pOrder[2]]; // vector from first to middle
                            Pnt3f pv1 = p[pOrder[0]]; // first vertex
                            Pnt3f pv2 = p[pOrder[2]]; // last vertex
                            pr11[1] = pv1[1] + vp2[1]/vp2[0]*(s[0]-pv1[0]);
                            pr12[1] = pv1[1] + vp3[1]/vp3[0]*(s[0]-pv1[0]);
                            pr21[1] = pv1[1] + vp2[1]/vp2[0]*(s[1]-pv1[0]);
                            pr22[1] = pv[1] + vp1[1]/vp1[0]*(s[1]-pv[0]);
                            pushPen(pr11, pr12, pr21, pr22, pv);
                            passed_middle = true;
                            continue;
                        }

                        // middle quad
                        Vec3f pr11(s[0],0,0); // point on cylinder edge
                        Vec3f pr12(s[0],0,0); // point on cylinder edge
                        Vec3f pr21(s[1],0,0); // point on cylinder edge
                        Vec3f pr22(s[1],0,0); // point on cylinder edge
                        Vec3f vp1, vp2;
                        Pnt3f pv1, pv2;
                        if (!passed_middle) {
                            vp1 = v[pOrder[1]]; // vector to middle point
                            vp2 = v[pOrder[2]]; // vector to last point
                            pv1 = p[pOrder[2]]; // vertex on that face
                            pv2 = p[pOrder[1]]; // vertex on that face
                        } else {
                            vp1 = v[pOrder[1]]; // vector from first point
                            vp2 = v[pOrder[0]]; // vector from middle point
                            pv1 = p[pOrder[0]]; // vertex on that face
                            pv2 = p[pOrder[1]]; // vertex on that face
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
                    for (int i=0; i<sides.size(); i++) {
                        Vec2f s = sides[i];
                        if (i == 0) { // first quad
                            Pnt3f pv1 = p[pOrder[0]]; // vertex on that face
                            Pnt3f pv2 = p[pOrder[1]]; // vertex on that face
                            Vec3f pr1(s[1],0,0); // point on cylinder edge
                            Vec3f pr2(s[1],0,0); // point on cylinder edge
                            Vec3f vp1 = v[pOrder[1]]; // vector to last point
                            Vec3f vp2 = v[pOrder[0]]; // vector to last point
                            pr1[1] = pv1[1] + vp1[1]/vp1[0]*(s[1]-pv1[0]);
                            pr2[1] = pv2[1] + vp2[1]/vp2[0]*(s[1]-pv2[0]);
                            pushQuad(pv1, pv2, pr1, pr2);
                            continue;
                        }
                        if (i == sides.size()-1) { // last triangle
                            Pnt3f pv = p[pOrder[2]]; // vertex on that face
                            Vec3f pr1(s[0],0,0); // point on cylinder edge
                            Vec3f pr2(s[0],0,0); // point on cylinder edge
                            Vec3f vp1 = v[pOrder[1]]; // vector to middle point
                            Vec3f vp2 = v[pOrder[0]]; // vector to last point
                            pr1[1] = pv[1] + vp1[1]/vp1[0]*(s[0]-pv[0]);
                            pr2[1] = pv[1] + vp2[1]/vp2[0]*(s[0]-pv[0]);
                            pushTri(pv,pr1,pr2);
                            continue;
                        }

                        Pnt3f pv1 = p[pOrder[0]]; // vertex on that face
                        Pnt3f pv2 = p[pOrder[1]]; // vertex on that face
                        Vec3f pr11(s[0],0,0); // point on cylinder edge
                        Vec3f pr12(s[0],0,0); // point on cylinder edge
                        Vec3f pr21(s[1],0,0); // point on cylinder edge
                        Vec3f pr22(s[1],0,0); // point on cylinder edge
                        Vec3f vp1 = v[pOrder[1]]; // vector to last point
                        Vec3f vp2 = v[pOrder[0]]; // vector to last point
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
                    for (int i=0; i<sides.size(); i++) {
                        Vec2f s = sides[i];

                        Pnt3f pv = p[pOrder[0]]; // vertex on that face
                        Pnt3f pv1 = p[pOrder[1]]; // vertex on that face
                        Pnt3f pv2 = p[pOrder[2]]; // vertex on that face

                        if (i == 0) { // first triangle
                            Vec3f pr1(s[1],0,0); // point on cylinder edge
                            Vec3f pr2(s[1],0,0); // point on cylinder edge
                            Vec3f vp1 = v[pOrder[1]]; // vector to middle point
                            Vec3f vp2 = v[pOrder[2]]; // vector to last point
                            pr1[1] = pv[1] + vp1[1]/vp1[0]*(s[1]-pv[0]);
                            pr2[1] = pv[1] + vp2[1]/vp2[0]*(s[1]-pv[0]);
                            pushTri(pv,pr1,pr2);
                            continue;
                        }
                        if (i == sides.size()-1) { // last quad
                            Vec3f pr1(s[0],0,0); // point on cylinder edge
                            Vec3f pr2(s[0],0,0); // point on cylinder edge
                            Vec3f vp1 = v[pOrder[2]]; // vector to last point
                            Vec3f vp2 = v[pOrder[1]]; // vector to last point
                            pr1[1] = pv1[1] + vp1[1]/vp1[0]*(s[0]-pv1[0]);
                            pr2[1] = pv2[1] + vp2[1]/vp2[0]*(s[0]-pv2[0]);
                            pushQuad(pv1, pv2, pr1, pr2);
                            continue;
                        }

                        Vec3f pr11(s[0],0,0); // point on cylinder edge
                        Vec3f pr12(s[0],0,0); // point on cylinder edge
                        Vec3f pr21(s[1],0,0); // point on cylinder edge
                        Vec3f pr22(s[1],0,0); // point on cylinder edge
                        Vec3f vp1 = v[pOrder[2]]; // vector to last point
                        Vec3f vp2 = v[pOrder[1]]; // vector to last point
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
        }

        // project the points back into 3D space
        if (g) {
            auto gg = g->getMesh();
            if (gg) {
                GeoVectorPropertyRecPtr pos = gg->getPositions();
                GeoVectorPropertyRecPtr norms = gg->getNormals();
                if (pos) {
                    for (int i=0; i<pos->size(); i++) {
                        Pnt3f p = pos->getValue<Pnt3f>(i);
                        Vec3f n = norms->getValue<Vec3f>(i);
                        n = Vec3f(cos(p[0]), sin(p[0]), 0);
                        p[2] = p[1];
                        p[1] = n[1]*R;
                        p[0] = n[0]*R;
                        pos->setValue(p, i);
                        norms->setValue(n, i);
                    }
                }
            }
            g->setMatrix(m);
        }

        //auto m = VRMaterial::create("mat");
        //m->setWireFrame(1); // test
        //g->setMaterial(m);
        return g;
    }

    cout << "unhandled surface type " << type << endl;

    // wireframe
    auto geo = VRGeometry::create("face");
    GeoPnt3fPropertyRecPtr pos = GeoPnt3fProperty::create();
    GeoVec3fPropertyRecPtr norms = GeoVec3fProperty::create();
    GeoUInt32PropertyRecPtr inds = GeoUInt32Property::create();

    for (auto b : bounds) {
        for (int i=0; i<b.points.size(); i+=2) {
            Pnt3f p1 = b.points[i];
            Pnt3f p2 = b.points[i+1];
            pos->addValue(p1);
            pos->addValue(p2);
            norms->addValue(Vec3f(0,1,0));
            norms->addValue(Vec3f(0,1,0));
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

