#include "VRMeshSubdivision.h"

#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/OSGGeometry.h"
#include "core/objects/geometry/VRGeoData.h"

#include <OpenSG/OSGTriangleIterator.h>

using namespace OSG;

VRMeshSubdivision::VRMeshSubdivision() {}
VRMeshSubdivision::~VRMeshSubdivision() {}

VRMeshSubdivisionPtr VRMeshSubdivision::create() { return VRMeshSubdivisionPtr( new VRMeshSubdivision() ); }
VRMeshSubdivisionPtr VRMeshSubdivision::ptr() { return static_pointer_cast<VRMeshSubdivision>(shared_from_this()); }

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

    //GeoUInt32PropertyMTRecPtr lengths = GeoUInt32Property::create();
    //lengths->addValue(newIndex->size());

    //gg->geo->setIndices(newIndex);
    //gg->geo->setLengths(lengths);

    newData.apply(geo);
}
