#include "VRMeshSubdivision.h"

#include "core/math/partitioning/boundingbox.h"
#include "core/utils/toString.h"

#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/OSGGeometry.h"
#include "core/objects/geometry/VRGeoData.h"

#include <OpenSG/OSGTriangleIterator.h>
#include <OpenSG/OSGGeoPnt3fProperty.h>

using namespace OSG;

VRMeshSubdivision::VRMeshSubdivision() {}
VRMeshSubdivision::~VRMeshSubdivision() {}

VRMeshSubdivisionPtr VRMeshSubdivision::create() { return VRMeshSubdivisionPtr( new VRMeshSubdivision() ); }
VRMeshSubdivisionPtr VRMeshSubdivision::ptr() { return static_pointer_cast<VRMeshSubdivision>(shared_from_this()); }


bool VRMeshSubdivision::checkOrder(Pnt3d p0, Pnt3d p1, Pnt3d p2, Vec3d n) {
    float cp = (p1-p0).cross(p2-p0).dot(n);
    return (cp >= 0);
}

void VRMeshSubdivision::pushTri(VRGeoData& g, Pnt3d p1, Pnt3d p2, Pnt3d p3, Vec3d n) {
    //cout << " pushTri " << p1 << "   " << p2 << "   " << p3 << endl;
    int a = g.pushVert(p1,n);
    int b = g.pushVert(p2,n);
    int c = g.pushVert(p3,n);
    if (checkOrder(p1,p2,p3,n)) g.pushTri(a,b,c);
    else g.pushTri(a,c,b);
}

void VRMeshSubdivision::pushQuad(VRGeoData& g, Pnt3d p1, Pnt3d p2, Pnt3d p3, Pnt3d p4, Vec3d n) {
    //cout << " pushQuad " << p1 << "   " << p2 << "   " << p3 << "   " << p4 << endl;
    int a = g.pushVert(p1,n);
    int b = g.pushVert(p2,n);
    int c = g.pushVert(p3,n);
    int d = g.pushVert(p4,n);
    if (checkOrder(p1,p2,p3,n)) g.pushTri(a,b,c);
    else g.pushTri(a,c,b);
    if (checkOrder(p2,p3,p4,n)) g.pushTri(b,c,d);
    else g.pushTri(b,d,c);
}

void VRMeshSubdivision::pushPen(VRGeoData& g, Pnt3d p1, Pnt3d p2, Pnt3d p3, Pnt3d p4, Pnt3d p5, Vec3d n) {
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
}

void VRMeshSubdivision::segmentTriangle(VRGeoData& geo, Vec3i pSegments, vector<Pnt3f> points, Vec3d n, vector<Vec2d> segments, int dim, int dim2) {
    Vec3i pOrder(0,1,2); // get the order of the vertices
    for (int i=0; i<3; i++) { // max 3 sort steps
        if (pSegments[pOrder[0]] > pSegments[pOrder[1]]) swap(pOrder[0], pOrder[1]);
        else if (pSegments[pOrder[0]] > pSegments[pOrder[2]]) swap(pOrder[0], pOrder[2]);
        else if (pSegments[pOrder[1]] > pSegments[pOrder[2]]) swap(pOrder[1], pOrder[2]);
    }
    //cout << " ordered vertices " << pOrder << "  " << pSegments[pOrder[0]] << " " << pSegments[pOrder[1]] << " " << pSegments[pOrder[2]] << endl;

    vector<Vec3f> edges(3);
    edges[0] = points[2]-points[1];
    edges[1] = points[2]-points[0];
    edges[2] = points[1]-points[0];

    // test first case: all vertices on the same cylinder face
    if (pSegments[0] == pSegments[1] && pSegments[0] == pSegments[2]) {
        //cout << "  case 1" << endl;
        pushTri(geo, Pnt3d(points[0]), Pnt3d(points[1]), Pnt3d(points[2]), n);
        return;
    }


    // TODO: push all vertices first, then make primitives! see case 3 and 4!
    // test second case: all vertices on different cylinder faces
    if (pSegments[0] != pSegments[1] && pSegments[0] != pSegments[2] && pSegments[1] != pSegments[2]) {
        //cout << "  case 2" << endl;
        bool passed_middle = false;
        for (uint i=0; i<segments.size(); i++) {
            Vec2d s = segments[i];

            Pnt3d pr11, pr12, pr21, pr22;
            pr11[dim] = s[0]; // point on cylinder edge
            pr12[dim] = s[0]; // point on cylinder edge
            pr21[dim] = s[1]; // point on cylinder edge
            pr22[dim] = s[1]; // point on cylinder edge
            Vec3d vp1 = Vec3d(edges[pOrder[0]]); // vector from middle to last
            Vec3d vp2 = Vec3d(edges[pOrder[1]]); // vector from first to last
            Vec3d vp3 = Vec3d(edges[pOrder[2]]); // vector from first to middle
            Pnt3d pv1 = Pnt3d(points[pOrder[0]]);
            Pnt3d pv2 = Pnt3d(points[pOrder[1]]);
            Pnt3d pv3 = Pnt3d(points[pOrder[2]]);

            if (i == 0) { // first triangle
                pr21[dim2] = pv1[dim2] + vp2[dim2]/vp2[dim]*(s[1]-pv1[dim]);
                pr22[dim2] = pv1[dim2] + vp3[dim2]/vp3[dim]*(s[1]-pv1[dim]);
                pushTri(geo, pv1,pr21,pr22, n);
                continue;
            }

            if (i == segments.size()-1) { // last triangle
                pr11[dim2] = pv3[dim2] + vp2[dim2]/vp2[dim]*(s[0]-pv3[dim]);
                pr12[dim2] = pv3[dim2] + vp1[dim2]/vp1[dim]*(s[0]-pv3[dim]);
                pushTri(geo, pv3,pr11,pr12, n);
                continue;
            }

            if (int(i) == pSegments[pOrder[1]]) { // pentagon in the middle
                pr11[dim2] = pv1[dim2] + vp2[dim2]/vp2[dim]*(s[0]-pv1[dim]);
                pr12[dim2] = pv1[dim2] + vp3[dim2]/vp3[dim]*(s[0]-pv1[dim]);
                pr21[dim2] = pv1[dim2] + vp2[dim2]/vp2[dim]*(s[1]-pv1[dim]);
                pr22[dim2] = pv2[dim2] + vp1[dim2]/vp1[dim]*(s[1]-pv2[dim]);
                pushPen(geo, pr11, pr12, pr21, pr22, pv2, n);
                passed_middle = true;
                continue;
            }

            // middle quad
            if (!passed_middle) {
                pr11[dim2] = pv3[dim2] + vp2[dim2]/vp2[dim]*(s[0]-pv3[dim]);
                pr12[dim2] = pv2[dim2] + vp3[dim2]/vp3[dim]*(s[0]-pv2[dim]);
                pr21[dim2] = pv3[dim2] + vp2[dim2]/vp2[dim]*(s[1]-pv3[dim]);
                pr22[dim2] = pv2[dim2] + vp3[dim2]/vp3[dim]*(s[1]-pv2[dim]);
                pushQuad(geo, pr11, pr12, pr21, pr22, n);
            } else {
                pr11[dim2] = pv1[dim2] + vp2[dim2]/vp2[dim]*(s[0]-pv1[dim]);
                pr12[dim2] = pv2[dim2] + vp1[dim2]/vp1[dim]*(s[0]-pv2[dim]);
                pr21[dim2] = pv1[dim2] + vp2[dim2]/vp2[dim]*(s[1]-pv1[dim]);
                pr22[dim2] = pv2[dim2] + vp1[dim2]/vp1[dim]*(s[1]-pv2[dim]);
                pushQuad(geo, pr11, pr12, pr21, pr22, n);
            }
        }
        return;
    }

    // case 3
    if (pSegments[pOrder[0]] == pSegments[pOrder[1]]) {
        //cout << "  case 3" << endl;
        Pnt3d pv1 = Pnt3d(points[pOrder[0]]); // vertex on that face
        Pnt3d pv2 = Pnt3d(points[pOrder[1]]); // vertex on that face
        Pnt3d pv3 = Pnt3d(points[pOrder[2]]); // vertex on that face

        Vec3d vp1 = Vec3d(edges[pOrder[1]]); // vector to last point
        Vec3d vp2 = Vec3d(edges[pOrder[0]]); // vector to last point

        int i1 = geo.pushVert(pv1, n);
        int i2 = geo.pushVert(pv2, n);
        int i3 = geo.pushVert(pv3, n);

        vector<int> ep1 = { i1 };
        vector<int> ep2 = { i2 };

        Pnt3d pr1, pr2;
        for (uint i=0; i<segments.size()-1; i++) {
            Vec2d s = segments[i];

            pr1[dim] = s[1]; // point on cylinder edge
            pr2[dim] = s[1]; // point on cylinder edge

            pr1[dim2] = pv1[dim2] + vp1[dim2]/vp1[dim]*(s[1]-pv1[dim]);
            pr2[dim2] = pv2[dim2] + vp2[dim2]/vp2[dim]*(s[1]-pv2[dim]);

            int e1 = geo.pushVert(pr1, n);
            int e2 = geo.pushVert(pr2, n);
            ep1.push_back(e1);
            ep2.push_back(e2);
        }

        for (uint i=0; i<segments.size()-1; i++) {
            geo.pushQuad(ep1[i], ep2[i], ep2[i+1], ep1[i+1]);
        }

        geo.pushTri(ep1.back(), ep2.back(), i3);
        return;
    }

    // case 4
    if (pSegments[pOrder[1]] == pSegments[pOrder[2]]) {
        //cout << "  case 3" << endl;
        Pnt3d pv1 = Pnt3d(points[pOrder[0]]); // vertex on that face
        Pnt3d pv2 = Pnt3d(points[pOrder[1]]); // vertex on that face
        Pnt3d pv3 = Pnt3d(points[pOrder[2]]); // vertex on that face

        Vec3d vp1 = Vec3d(edges[pOrder[2]]); // vector to last point
        Vec3d vp2 = Vec3d(edges[pOrder[1]]); // vector to last point

        int i1 = geo.pushVert(pv1, n);
        int i2 = geo.pushVert(pv2, n);
        int i3 = geo.pushVert(pv3, n);

        vector<int> ep1;
        vector<int> ep2;

        Pnt3d pr1, pr2;
        for (uint i=0; i<segments.size()-1; i++) {
            Vec2d s = segments[i];

            pr1[dim] = s[1]; // point on cylinder edge
            pr2[dim] = s[1]; // point on cylinder edge

            pr1[dim2] = pv1[dim2] + vp1[dim2]/vp1[dim]*(s[1]-pv1[dim]);
            pr2[dim2] = pv1[dim2] + vp2[dim2]/vp2[dim]*(s[1]-pv1[dim]);

            int e1 = geo.pushVert(pr1, n);
            int e2 = geo.pushVert(pr2, n);
            ep1.push_back(e1);
            ep2.push_back(e2);
        }

        ep1.push_back(i2);
        ep2.push_back(i3);

        geo.pushTri(i1, ep1[0], ep2[0]);

        for (uint i=0; i<segments.size()-1; i++) {
            geo.pushQuad(ep1[i], ep2[i], ep2[i+1], ep1[i+1]);
        }

        return;
    }

    cout << " unhandled triangle " << endl;
}

void VRMeshSubdivision::subdivideGrid(VRGeometryPtr geo, Vec3d res) {
    VRGeoData data(geo);
    VRGeoData newData;

    auto gg = geo->getMesh();

    // first iteration, get bounding box
    Boundingbox box;
    GeoVectorPropertyMTRecPtr positions = gg->geo->getPositions();
    for (size_t i=0; i<positions->size(); i++) {
        Vec3d p(positions->getValue<Pnt3f>(i));
        box.update(p);
    }

    // compute grid params
    Vec3i gridN(1,1,1);
    Vec3d boxSize = box.size();
    for (int i=0; i<3; i++) {
        if (res[i] <= 0) continue;
        gridN[i] = max(1.0, floor(boxSize[i]/res[i]));
        res[i] = boxSize[i]/gridN[i];
    }

    // second iteration, split all triangles along one dimension according to grid

    cout << " subdivide grid: " << gridN << ", box: " << boxSize << ", res: " << res << endl;

    auto subdivideAxis = [&](int dim, int dim2) {
        if (gridN[dim] == 1) return;
        float gMin = box.min()[dim];

        for (auto it = TriangleIterator(gg->geo); !it.isAtEnd() ;++it) {
            vector<Pnt3f> points(3); // vertex positions
            for (int i=0; i<3; i++) points[i] = it.getPosition(i);

            Vec3f n(0,1,0);
            Vec3f e1 = points[1]-points[0];
            Vec3f e2 = points[2]-points[0];
            Vec3f e3 = points[2]-points[1];
            double e1L2 = e1.squareLength();
            double e2L2 = e2.squareLength();
            double e3L2 = e3.squareLength();
            if (e1L2 > e3L2 && e2L2 > e3L2) n = e1.cross(e2);
            if (e1L2 > e2L2 && e3L2 > e2L2) n = e1.cross(e3);
            if (e2L2 > e1L2 && e3L2 > e1L2) n = e2.cross(e3);
            n.normalize();

            // analyse triangle
            float aMin = points[0][dim];
            float aMax = points[0][dim];
            float aMid = points[0][dim];
            int vMin = 0;
            int vMax = 0;
            int vMid = 0;

            for (int i=0; i<3; i++) {
                float x = points[i][dim];

                if (x < aMin) {
                    aMin = x;
                    vMin = i;
                }

                if (x > aMax) {
                    aMax = x;
                    vMax = i;
                }
            }

            vMid = 3-(vMin+vMax);
            aMid = points[vMid][dim];

            // grid intersect triangle
            Vec3i pSegments;
            vector<Vec2d> segments;
            for (int i=0; i<gridN[dim]; i++) {
                float g1 = gMin + i*res[dim];
                float g2 = g1 + res[dim];
                if (g2 >  aMin && g1 <  aMax) segments.push_back(Vec2d(g1, g2));
                if (g1 <= aMin && g2 >  aMin) pSegments[vMin] = i;
                if (g1 <= aMid && g2 >= aMid) pSegments[vMid] = i;
                if (g1 <  aMax && g2 >= aMax) pSegments[vMax] = i;
            }

            //cout << "   subdivide triangle " << pSegments << ", points: " << toString(points) << ", n: " << n << ", segments: " << toString(segments) << endl;
            segmentTriangle(newData, pSegments, points, Vec3d(n), segments, dim, dim2);
        }

        // apply and reset for next pass
        newData.apply(geo);
        gg = geo->getMesh();
        newData = VRGeoData();
    };

    subdivideAxis(0, 2);
    //subdivideAxis(1, 2);
    //subdivideAxis(2, 0);
}

void VRMeshSubdivision::subdivideTriangles(VRGeometryPtr geo, Vec3d res) {
    VRGeoData data(geo);
    VRGeoData newData;

    for (size_t i=0; i<data.size(); i++) newData.pushVert( data.getPosition(i) );

    // to subdivide a triangle in a mesh:
    //  create new points on edge to subdivide, linear interpolate
    //  append new vertices to geometry
    //  add new triangles indices in a new index vector
    //  add all triangle indices if no subdivision required

    auto gg = geo->getMesh();
    for (auto it = TriangleIterator(gg->geo); !it.isAtEnd() ;++it) {
        vector<Vec3f> p(3); // vertex positions
        for (int i=0; i<3; i++) p[i] = Vec3f(it.getPosition(i));

        vector< vector<int> > eIDs = { {it.getPositionIndex(0)}, {it.getPositionIndex(1)}, {it.getPositionIndex(2)} };
        int NpAdded = 0;

        auto pushTri = [&](int a, int b, int c) {
            auto p1 = newData.getPosition(a);
            auto p2 = newData.getPosition(b);
            auto p3 = newData.getPosition(c);
            auto v1 = p2-p1;
            auto v2 = p3-p1;
            float d = v1.cross(v2).dot(Vec3d(0,1,0));
            if (d >= 0) newData.pushTri(a,b,c);
            else        newData.pushTri(a,c,b);
        };

        auto inverted = [](vector<int>& v) {
            vector<int> i(v.size());
            for (int k=0; k<v.size(); k++) i[k] = v[v.size()-k-1];
            return i;
        };

        auto getNSubs = [&](Vec3f& A, Vec3f& B) {
            Vec3f edge = B-A;

            int eN = 1; // get subdivisions
            for (int j=0; j<3; j++) {
                if (res[j] <= 0) continue;
                int N = ceil(abs(edge[j])/res[j]);
                eN = max(N, eN);
            }

            return eN;
        };

        Vec3i tN;
        for (int i=0; i<3; i++) {
            Vec3f& A = p[i];
            Vec3f& B = p[(i+1)%3];
            int eN = getNSubs(A, B);
            tN[i] = eN;

            for (int k=1; k<eN; k++) {
                double x = double(k)/eN;
                int vID = newData.pushVert(Vec3d(A*(1.0-x)+B*x));
                eIDs[i].push_back(vID);
                NpAdded++;
            }
        }

        if (eIDs[0].size() == 1 && eIDs[1].size() == 1 && eIDs[2].size() == 1) { // no subdivisions
            pushTri(it.getPositionIndex(0), it.getPositionIndex(1), it.getPositionIndex(2));
            continue;
        }

        eIDs[0].push_back(it.getPositionIndex(1));
        eIDs[1].push_back(it.getPositionIndex(2));
        eIDs[2].push_back(it.getPositionIndex(0));

        Vec3i edgesByDivisions(0,1,2);
        if (eIDs[0].size() < eIDs[2].size() && eIDs[2].size() < eIDs[1].size()) edgesByDivisions = Vec3i(0,2,1);
        if (eIDs[1].size() < eIDs[0].size() && eIDs[0].size() < eIDs[2].size()) edgesByDivisions = Vec3i(1,0,2);
        if (eIDs[1].size() < eIDs[2].size() && eIDs[2].size() < eIDs[0].size()) edgesByDivisions = Vec3i(1,2,0);
        if (eIDs[2].size() < eIDs[0].size() && eIDs[0].size() < eIDs[1].size()) edgesByDivisions = Vec3i(2,0,1);
        if (eIDs[2].size() < eIDs[1].size() && eIDs[1].size() < eIDs[0].size()) edgesByDivisions = Vec3i(2,1,0);

        int I1 = edgesByDivisions[0]; // start with edge with least divisions
        int I2 = (I1+2)%3;
        int I3 = (I1+1)%3;
        vector<vector<int>> fan(eIDs[I1].size());
        fan[0] = inverted(eIDs[I2]);
        for (int i=1; i<eIDs[I1].size()-1; i++) { // create mid edges
            int i0 = eIDs[I1][i]; // start ID of fan edge
            int iPo = eIDs[I2][0]; // ID of opposing point to edge
            fan[i].push_back(eIDs[I1][i]); // start of fan edge
            Vec3f A = Vec3f(newData.getPosition(i0));
            Vec3f B = Vec3f(newData.getPosition(iPo));
            int eN = getNSubs(A, B);
            for (int k=1; k<eN; k++) {
                double x = double(k)/eN;
                int vID = newData.pushVert(Vec3d(A*(1.0-x)+B*x));
                fan[i].push_back(vID);
                NpAdded++;
            }
            fan[i].push_back(iPo); // end of fan edge, opposing point to edge
        }
        fan[fan.size()-1] = eIDs[I3];

        auto meshSlice = [&](vector<int>& e1, vector<int>& e2) { // e1 has more points
            int ei1 = 0;
            int ei2 = 0;
            double et1 = 0;
            double et2 = 0;
            double dt1 = 1.0/(e1.size()-1);
            double dt2 = 1.0/(e2.size()-1);
            for (;ei1<e1.size()-1 && ei2<e2.size()-1;) {
                if (et1 <= et2) pushTri(e1[ei1], e2[ei2], e1[ei1+1]);
                else            pushTri(e2[ei2], e2[ei2+1], e1[ei1]);

                // advance
                if (et1 <= et2) { ei1++; et1 += dt1; }
                else            { ei2++; et2 += dt2; }
            }
        };

        for (int i=1; i<fan.size(); i++) {
            auto& e1 = fan[i-1];
            auto& e2 = fan[i];
            if (e1.size() > e2.size()) meshSlice(e1, e2);
            else                       meshSlice(e2, e1);
        }
    }

    newData.apply(geo);
}
