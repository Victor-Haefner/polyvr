#include "VRMeshSubdivision.h"

#include "core/math/partitioning/boundingbox.h"
#include "core/utils/toString.h"
#include "core/utils/isNan.h"
#include "core/utils/system/VRSystem.h"

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


bool VRMeshSubdivision::checkOrder(VRGeoData& g, size_t a, size_t b, size_t c, Vec3d n) {
    Pnt3d p0 = g.getPosition(a);
    Pnt3d p1 = g.getPosition(b);
    Pnt3d p2 = g.getPosition(c);

    if (isNan(p0)) printBacktrace();
    if (isNan(p1)) printBacktrace();
    if (isNan(p2)) printBacktrace();

    float cp = (p1-p0).cross(p2-p0).dot(n);
    return (cp >= 0);
}

void VRMeshSubdivision::pushTri(VRGeoData& g, size_t p1, size_t p2, size_t p3, Vec3d n) {
    if (checkOrder(g,p1,p2,p3,n)) g.pushTri(p1,p2,p3);
    else g.pushTri(p1,p3,p2);
}

void VRMeshSubdivision::pushQuad(VRGeoData& g, size_t p1, size_t p2, size_t p3, size_t p4, Vec3d n) {
    pushTri(g, p1,p2,p3, n);
    pushTri(g, p1,p3,p4, n);
}


void VRMeshSubdivision::removeDoubles(VRGeometryPtr geo) {
    VRGeoData newData;
    auto gg = geo->getMesh();

    auto toBin = [](Pnt3f p) {
        double eps = 1.0/1e-3;
        return Vec3i(p[0]*eps, p[1]*eps, p[2]*eps);
    };

    map<Vec3i, size_t> vIDmap;

    for (auto it = TriangleIterator(gg->geo); !it.isAtEnd() ;++it) {
        vector<Vec3f> p(3); // vertex positions
        for (int i=0; i<3; i++) {
            Pnt3f p = it.getPosition(i);
            Vec3i pi = toBin(p);
            if (!vIDmap.count(pi)) {
                vIDmap[pi] = newData.pushVert(Pnt3d(p), Vec3d(it.getNormal(i)));
            }
        }

        size_t a = vIDmap[toBin(it.getPosition(0))];
        size_t b = vIDmap[toBin(it.getPosition(1))];
        size_t c = vIDmap[toBin(it.getPosition(2))];
        newData.pushTri(a,b,c);
    }

    newData.apply(geo);
}

void VRMeshSubdivision::gridMergeTriangles(VRGeometryPtr geo, Vec3d g0, Vec3d res, int dim, int dim2) {
    double cellA = res[dim] * res[dim2];
    if (res[dim] < 0 || res[dim2] < 0) return;

    VRGeoData newData;
    auto gg = geo->getMesh();

    auto getGridPos = [&](Vec3f p) {
        Vec3i g;
        for(int i=0; i<3; i++) g[i] = floor((p[i]-g0[i])/res[i]);
        return g;
    };

    auto gridPos = [&](Vec3i gridID) {
        Pnt3d p = g0;
        for(int i=0; i<3; i++)
            if (res[i] > 0) p[i] += res[i]*gridID[i];
        return p;
    };

    auto pushCell = [&](Vec3i gridID, Vec3d n) {
        Vec3i g1,g2;
        g1[dim] = 1;
        g2[dim2] = 1;

        Pnt3d p1 = gridPos(gridID);
        Pnt3d p2 = gridPos(gridID + g1);
        Pnt3d p3 = gridPos(gridID + g1 + g2);
        Pnt3d p4 = gridPos(gridID + g2);

        size_t a = newData.pushVert(p1, n);
        size_t b = newData.pushVert(p2, n);
        size_t c = newData.pushVert(p3, n);
        size_t d = newData.pushVert(p4, n);
        pushQuad(newData, a,b,c,d, n);
    };

    auto comArea = [](vector<Vec3f> p) {
        Vec3f a = p[2]-p[0];
        Vec3f b = p[2]-p[1];
        return a.cross(b).length()*0.5;
    };

    map<size_t, size_t> vIDmap;
    auto pushTriangles = [&](vector<TriangleIterator> triangles) {
        for (auto& it : triangles) {
            for (int i=0; i<3; i++) {
                size_t idx = it.getPositionIndex(i);
                if (!vIDmap.count(idx)) {
                    Pnt3d p = Pnt3d(it.getPosition(i));
                    Vec3d n = Vec3d(it.getNormal(i));
                    vIDmap[idx] = newData.pushVert(p, n);
                }
            }

            size_t a = vIDmap[it.getPositionIndex(0)];
            size_t b = vIDmap[it.getPositionIndex(1)];
            size_t c = vIDmap[it.getPositionIndex(2)];
            newData.pushTri(a,b,c);
        }
    };

    map<Vec3i, vector<TriangleIterator>> gridTriangles;
    for (auto it = TriangleIterator(gg->geo); !it.isAtEnd() ;++it) {
        vector<Vec3f> p(3); // vertex positions
        for (int i=0; i<3; i++) p[i] = Vec3f(it.getPosition(i));

        Vec3f mid = (p[0]+p[1]+p[2])*1.0/3;
        Vec3i midID = getGridPos(mid);

        if (!gridTriangles.count(midID)) gridTriangles[midID] = vector<TriangleIterator>();
        gridTriangles[midID].push_back(it);
    }

    for (auto& gtri : gridTriangles) {
        Vec3i gridID = gtri.first;
        double A = 0;
        for (auto& it : gtri.second) {
            vector<Vec3f> p(3); // vertex positions
            for (int i=0; i<3; i++) p[i] = Vec3f(it.getPosition(i));
            A += comArea(p);
        }

        double q = A/cellA;
        if (q > 0.99) pushCell(gridID, Vec3d(gtri.second[0].getNormal(0)));
        else          pushTriangles(gtri.second);
    }

    newData.apply(geo);
}

void VRMeshSubdivision::segmentTriangle(VRGeoData& geo, Vec3i pSegments, vector<Pnt3f> points, Vec3d n, vector<Vec2d> segments, int dim, int dim2) {
    Vec3i pOrder(0,1,2); // get the order of the vertices
    for (int i=0; i<3; i++) { // max 3 sort steps
        if (pSegments[pOrder[0]] > pSegments[pOrder[1]]) swap(pOrder[0], pOrder[1]);
        else if (pSegments[pOrder[0]] > pSegments[pOrder[2]]) swap(pOrder[0], pOrder[2]);
        else if (pSegments[pOrder[1]] > pSegments[pOrder[2]]) swap(pOrder[1], pOrder[2]);
    }

    if (segments.size() == 0) {
        cout << "WARNING in VRMeshSubdivision::segmentTriangle, no segments!" << endl;
        return;
    }
    //cout << " ordered vertices " << pOrder << "  " << pSegments[pOrder[0]] << " " << pSegments[pOrder[1]] << " " << pSegments[pOrder[2]] << endl;

    vector<Vec3f> edges(3);
    edges[0] = points[pOrder[2]]-points[pOrder[1]];
    edges[1] = points[pOrder[2]]-points[pOrder[0]];
    edges[2] = points[pOrder[1]]-points[pOrder[0]];

    if (pSegments[0] == pSegments[1] && pSegments[0] == pSegments[2]) { // case 1
        //cout << "  case 1" << endl;
        auto a = geo.pushVert(Pnt3d(points[0]), n);
        auto b = geo.pushVert(Pnt3d(points[1]), n);
        auto c = geo.pushVert(Pnt3d(points[2]), n);
        pushTri(geo, a,b,c, n);
        return;
    }

    if (pSegments[0] != pSegments[1] && pSegments[0] != pSegments[2] && pSegments[1] != pSegments[2]) { // case 2
        //cout << "  case 2" << endl;
        Pnt3d pv1 = Pnt3d(points[pOrder[0]]); // vertex on that face
        Pnt3d pv2 = Pnt3d(points[pOrder[1]]); // vertex on that face
        Pnt3d pv3 = Pnt3d(points[pOrder[2]]); // vertex on that face

        Vec3d vp1 = Vec3d(edges[0]); // vector to last point
        Vec3d vp2 = Vec3d(edges[1]); // vector to last point
        Vec3d vp3 = Vec3d(edges[2]); // vector to last point

        int seg1 = pSegments[pOrder[0]];
        int seg2 = pSegments[pOrder[1]];
        int seg3 = pSegments[pOrder[2]];

        int i1 = geo.pushVert(pv1, n);
        int i2 = geo.pushVert(pv2, n);
        int i3 = geo.pushVert(pv3, n);

        vector<int> ep1;
        vector<int> ep2;

        Pnt3d pr1, pr2;
        bool passedMiddle = false;
        for (uint i=0; i<segments.size()-1; i++) {
            if (seg1+i == seg2) passedMiddle = true;

            Vec2d s = segments[i];

            pr1[dim] = s[1]; // point on cylinder edge
            pr2[dim] = s[1]; // point on cylinder edge

            pr1[dim2] = pv1[dim2] + vp2[dim2]/vp2[dim]*(s[1]-pv1[dim]);

            if (!passedMiddle)  pr2[dim2] = pv1[dim2] + vp3[dim2]/vp3[dim]*(s[1]-pv1[dim]);
            else                pr2[dim2] = pv2[dim2] + vp1[dim2]/vp1[dim]*(s[1]-pv2[dim]);

            int e1 = geo.pushVert(pr1, n);
            int e2 = geo.pushVert(pr2, n);
            ep1.push_back(e1);
            ep2.push_back(e2);
        }

        pushTri(geo, i1, ep1[0], ep2[0], n);
        for (uint i=0; i<segments.size()-2; i++) {
            pushQuad(geo, ep1[i], ep2[i], ep2[i+1], ep1[i+1], n);
            if (seg1+i == pSegments[pOrder[1]]-1) // middle
                pushTri(geo, i2, ep2[i], ep2[i+1], n);
        }
        pushTri(geo, i3, ep1.back(), ep2.back(), n);

        return;
    }

    if (pSegments[pOrder[0]] == pSegments[pOrder[1]]) { // case 3
        //cout << "  case 3" << endl;
        Pnt3d pv1 = Pnt3d(points[pOrder[0]]); // vertex on that face
        Pnt3d pv2 = Pnt3d(points[pOrder[1]]); // vertex on that face
        Pnt3d pv3 = Pnt3d(points[pOrder[2]]); // vertex on that face

        Vec3d vp1 = Vec3d(edges[1]); // vector to last point
        Vec3d vp2 = Vec3d(edges[0]); // vector to last point

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

            if (isNan(pr1) || isNan(pr2)) {
                cout << "NAN----------------------NAN" << endl;
                cout << s << ",   dim " << dim << ",   dim2 " << dim2 << endl;
                cout << pr1 << ",   " << pr2 << endl;
                cout << vp1 << ",   " << vp2 << endl;
                cout << pv1 << ",   " << pv2 << ",   " << pv3 << endl;
                cout << pOrder << endl;
                cout << pSegments[pOrder[0]] << ",   " << pSegments[pOrder[1]] << ",   " << pSegments[pOrder[2]] << endl;
                cout << toString(points) << ",   " << pSegments << endl;
            }

            int e1 = geo.pushVert(pr1, n);
            int e2 = geo.pushVert(pr2, n);
            ep1.push_back(e1);
            ep2.push_back(e2);
        }

        for (uint i=0; i<segments.size()-1; i++) {
            pushQuad(geo, ep1[i], ep2[i], ep2[i+1], ep1[i+1], n);
        }

        pushTri(geo, ep1.back(), ep2.back(), i3, n);
        return;
    }

    if (pSegments[pOrder[1]] == pSegments[pOrder[2]]) { // case 4
        //cout << "  case 4" << endl;
        Pnt3d pv1 = Pnt3d(points[pOrder[0]]); // vertex on that face
        Pnt3d pv2 = Pnt3d(points[pOrder[1]]); // vertex on that face
        Pnt3d pv3 = Pnt3d(points[pOrder[2]]); // vertex on that face

        Vec3d vp1 = Vec3d(edges[2]); // vector to last point
        Vec3d vp2 = Vec3d(edges[1]); // vector to last point

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

        pushTri(geo, i1, ep1[0], ep2[0], n);

        for (uint i=0; i<segments.size()-1; i++) {
            pushQuad(geo, ep1[i], ep2[i], ep2[i+1], ep1[i+1], n);
        }

        return;
    }

    cout << " unhandled triangle " << endl;
}

void VRMeshSubdivision::subdivideAxis(VRGeometryPtr geo, Vec3i gridN, Vec3d gMin, Vec3d res, int dim, int dim2) {
    if (gridN[dim] == 1) return;
    //cout << " ...... subdivideAxis gridN: " << gridN << ", gMin: " << gMin << ", res: " << res << ", dim: " << dim << ", dim2: " << dim2 << endl;

    VRGeoData newData;
    auto gg = geo->getMesh();

    //int K = 0;
    for (auto it = TriangleIterator(gg->geo); !it.isAtEnd() ;++it) {
        vector<Pnt3f> points(3); // vertex positions
        for (int i=0; i<3; i++) points[i] = it.getPosition(i);

        //K++;
        //if (K > 27) break;

        Vec3f n(0,1,0);
        Vec3f e1 = points[1]-points[0];
        Vec3f e2 = points[2]-points[0];
        Vec3f e3 = points[2]-points[1];
        double e1L2 = e1.squareLength();
        double e2L2 = e2.squareLength();
        double e3L2 = e3.squareLength();
        n = e1.cross(e2);
        if (n.length() < 1e-8) {
            cout << "Warning! skip small triangle, n: " << n << " dot: " << n.dot(Vec3f(0,1,0)) << ", e1: " << e1 << ", e2: " << e2 << ", e3: " << e3 << endl;
            continue;
        }
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
        float eps = 1e-6;

        // grid intersect triangle
        Vec3i pSegments;
        vector<Vec2d> segments;
        bool addSegments = false;
        for (int i=0; i<gridN[dim]; i++) {
            float g1 = gMin[dim] + i*res[dim];
            float g2 = g1 + res[dim];
            if (g1 <= aMin+eps && g2 >  aMin) { pSegments[vMin] = i; addSegments = true; }
            if (addSegments) segments.push_back(Vec2d(g1, g2));
            if (g1 <= aMid+eps && g2 >= aMid-eps) pSegments[vMid] = i;
            if (g1 <  aMax     && g2 >= aMax-eps) { pSegments[vMax] = i; break; }
            //cout << "     g12: " << Vec2d(g1, g2) << ", aMinMidMax: " << Vec3d(aMin, aMid, aMax) << ", vMinMidMax: " << Vec3d(vMin, vMid, vMax) << endl;
        }

        //if (abs(aMid-aMin) < eps) continue;
        //if (abs(aMid-aMax) < eps) continue;
        if (abs(aMid-aMin) < eps) pSegments[vMid] = pSegments[vMin];
        if (abs(aMid-aMax) < eps) pSegments[vMid] = pSegments[vMax];

        //cout << "   subdivide triangle " << pSegments << ", points: " << toString(points) << ", n: " << n << ", segments: " << toString(segments) << endl;
        segmentTriangle(newData, pSegments, points, Vec3d(n), segments, dim, dim2);
    }

    // apply and reset for next pass
    newData.apply(geo);
    gg = geo->getMesh();
}

void VRMeshSubdivision::subdivideGrid(VRGeometryPtr geo, Vec3d res) {
    auto gg = geo->getMesh();
    Boundingbox box;
    GeoVectorPropertyMTRecPtr positions = gg->geo->getPositions();
    for (size_t i=0; i<positions->size(); i++) {
        Vec3d p(positions->getValue<Pnt3f>(i));
        box.update(p);
    }
    Vec3d gMin = box.min();

    // compute grid params
    Vec3i gridN(1,1,1);
    Vec3d boxSize = box.size();
    for (int i=0; i<3; i++) {
        if (res[i] <= 0) continue;
        gridN[i] = max(1.0, floor(boxSize[i]/res[i]));
        res[i] = boxSize[i]/gridN[i];
    }

    subdivideAxis(geo, gridN, gMin, res, 0, 2);
    subdivideAxis(geo, gridN, gMin, res, 2, 0);
    gridMergeTriangles(geo, gMin, res, 0, 2);

    //subdivideAxis(geo, gridN, gMin, res, 1, 2);
    //gridMergeTriangles(geo, gMin, res, 1, 2);
    //gridMergeTriangles(geo, gMin, res, 0, 1);

    removeDoubles(geo);


    /*positions = gg->geo->getPositions();
    for (size_t i=0; i<positions->size(); i++) {
        Pnt3f p = positions->getValue<Pnt3f>(i);
        p[1] = i*0.01;
        positions->setValue(p, i);
    }*/
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
