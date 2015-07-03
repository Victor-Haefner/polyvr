#include "CSGGeometry.h"
#include "CGALTypedefs.h"
#include "core/math/Octree.h"

#include "PolyhedronBuilder.h"

#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGGeometry.h>
#include <OpenSG/OSGTriangleIterator.h>

using namespace std;
using namespace OSG;

float triangleArea(Vec3f p1, Vec3f p2, Vec3f p3) {
    Vec3f v1 = p2-p1;
    Vec3f v2 = p3-p1;
    return v1.cross(v2).length()*0.5;
}

// Converts geometry to a polyhedron && applies the geometry node's world transform to the polyhedron.
// OpenSG geometry data isn't transformed itself but has an associated transform core. Both are unified for CGAL.
CGAL::Polyhedron* CSGGeometry::toPolyhedron(GeometryRecPtr geometry, Matrix worldTransform, bool& success) {
	vector<CGAL::Point> positions;
	vector<size_t> indices;
	size_t curIndex = 0;
	TriangleIterator it;
	auto gpos = geometry->getPositions();
    cout << getName() << " toPolyhedron\n";

	// fix flat triangles (all three points aligned)
	for (it = TriangleIterator(geometry); !it.isAtEnd() ;++it) {
        vector<Pnt3f> p(3);
        vector<Vec3f> v(3);
        Vec3i vi = Vec3i(it.getPositionIndex(0), it.getPositionIndex(1), it.getPositionIndex(2));
        for (int i=0; i<3; i++) p[i] = it.getPosition(i);
        v[0] = p[2]-p[1]; v[1] = p[2]-p[0]; v[2] = p[1]-p[0];
        float A = (v[2].cross(v[1])).length();
        if (A < 1e-16) { // small area, flat triangle?
            cout << "small area " << A << endl;
            for (int i=0; i<3; i++) cout << " pi " << p[i] << " vi " << vi[i] << " L " << v[i].squareLength() << endl;
            if (v[0].squareLength() < 1e-8) continue; // check if two points close, then ignore
            if (v[1].squareLength() < 1e-8) continue;
            if (v[2].squareLength() < 1e-8) continue;

            int im = 0;
            for (int i=1; i<3; i++) if (v[i].squareLength() > v[im].squareLength()) im = i;
            int j = (im+1)%3;
            cout << "set p[" << j << "] = " << p[j] << " with index i[" << im << "] = " << vi[im] << endl;
            gpos->setValue(p[j], vi[im]);
            for (int i=0; i<3; i++) p[i] = it.getPosition(i);
            cout << " result: " << endl;
            for (int i=0; i<3; i++) cout << "  pi " << p[i] << endl;
        }
	}

	// Convert triangles to cgal indices and vertices
	for (it = TriangleIterator(geometry); !it.isAtEnd() ;++it) {
        vector<size_t> IDs(3);
        for (int i=0; i<3; i++) IDs[i] = isKnownPoint( it.getPosition(i) );

		for (int i=0; i<3; i++) {
			if (IDs[i] == numeric_limits<size_t>::max()) {
                Pnt3f osgPos = it.getPosition(i);
				CGAL::Point cgalPos(osgPos.x(), osgPos.y(), osgPos.z());
				positions.push_back(cgalPos);
				IDs[i] = curIndex;
                //cout << "add point " << curIndex << "   " << osgPos << endl;
				size_t *curIndexPtr = new size_t;
				*curIndexPtr = curIndex;
				oct->add(OcPoint(osgPos.x(), osgPos.y(), osgPos.z()), curIndexPtr);
				curIndex++;
			}
		}

		float A = triangleArea(Vec3f(it.getPosition(0)), Vec3f(it.getPosition(1)), Vec3f(it.getPosition(2)));
		if (A < 1e-16) { cout << " tiny Area: " << A << endl; continue; }

        //cout << "add triangle " << IDs[0] << " " << IDs[1] << " " << IDs[2] << endl;
		if (IDs[0] == IDs[1] || IDs[0] == IDs[2] || IDs[1] == IDs[2]) continue; // ignore flat triangles

		for (int i=0; i<3; i++) indices.push_back(IDs[i]);
	}

	// Cleanup
	for (void* o : oct->getData()) delete (size_t*)o;
	delete oct;

	oct = new Octree(THRESHOLD);


    //cout << "\ntoPolyhedron " << getName() << " transformation : \n" << worldTransform << endl;
	//cout << "size: " << positions.size() << " " << indices.size() << endl;
	//for (size_t i = 0; i < positions.size(); i++) cout << positions[i] << endl;
	//for (size_t i = 0; i < indices.size(); i += 3) cout << indices[i] << indices[i+1] << indices[i+2] << endl;


	// Construct the polyhedron from raw data
	CGAL::Polyhedron *result = new CGAL::Polyhedron();
	PolyhedronBuilder<CGAL::HalfedgeDS> builder(positions, indices);
	result->delegate(builder);
	success = true;
	if (!result->is_closed()) { cout << "Error: The polyhedron is not a closed mesh!" << endl; success = false; }

	// Transform the polyhedron with the geometry's world transform matrix
	applyTransform(result, worldTransform);
	return result;
}

