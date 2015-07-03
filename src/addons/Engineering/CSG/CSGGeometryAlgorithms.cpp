#include "CSGGeometry.h"
#include "CGALTypedefs.h"
#include "core/math/Octree.h"

#include "PolyhedronBuilder.h"

#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGGeometry.h>
#include <OpenSG/OSGTriangleIterator.h>

using namespace std;
using namespace OSG;

 // Converts geometry to a polyhedron && applies the geometry node's world transform to the polyhedron.
// OpenSG geometry data isn't transformed itself but has an associated transform core. Both are unified for CGAL.
CGAL::Polyhedron* CSGGeometry::toPolyhedron(GeometryRecPtr geometry, Matrix worldTransform, bool& success) {
	vector<CGAL::Point> positions;
	vector<size_t> indices;
	size_t curIndex = 0;
	TriangleIterator it;
	auto gpos = geometry->getPositions();
    cout << " toPolyhedron\n";

	// fix flat triangles (all three points aligned)
	for (it = TriangleIterator(geometry); !it.isAtEnd() ;++it) {
        vector<Pnt3f> p(3);
        vector<Vec3f> v(3);
        Vec3i vi = Vec3i(it.getPositionIndex(0), it.getPositionIndex(1), it.getPositionIndex(2));
        for (int i=0; i<3; i++) p[i] = it.getPosition(i);
        v[0] = p[2]-p[1]; v[1] = p[2]-p[0]; v[2] = p[1]-p[0];
        float A = (v[2].cross(v[1])).length();
        if (A < 1e-16) { // small area, flat triangle?
            for (int i=0; i<3; i++) if (v[i].squareLength() < 1e-8) continue; // check if two points close, then ignore

            int im = 0;
            for (int i=1; i<3; i++) if (v[i].squareLength() > v[im].squareLength()) im = i;
            gpos->setValue(p[(im+1)%3], vi[im]);
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
	if (!result->is_closed()) throw std::runtime_error("The polyhedron is not a closed mesh!");

	// Transform the polyhedron with the geometry's world transform matrix
	applyTransform(result, worldTransform);
	return result;
}


bool CSGGeometry::disableEditMode() {
	if (children.size() != 2) { cout << "CSGGeometry: Warning: editMode disabled with less than 2 children. Doing nothing.\n"; return false; }

	vector<CGAL::Polyhedron*> polys(2,0); // We need two child geometries to work with

	for (int i=0; i<2; i++) { // Prepare the polyhedra
		VRObject *obj = children[i];
        obj->setVisible(false);

		if (obj->getType() == string("Geometry")) {
			VRGeometry *geo = dynamic_cast<VRGeometry*>(obj);
			try {
			    bool success;
			    polys[i] = toPolyhedron( geo->getMesh(), geo->getWorldMatrix(), success );
			} catch (exception e) {
			    cout << getName() << ": Could not convert mesh data to polyhedron: " << e.what() << endl;
				obj->setVisible(true); // We stay in edit mode, so both children need to be visible
				return false;
			}
			continue;
		}

		if(obj->getType() == "CSGGeometry") {
			CSGGeometry *geo = dynamic_cast<CSGGeometry*>(obj);
			polys[i] = geo->getCSGGeometry(); // TODO: where does this come from?? keep the old!
			continue;
		}

		cout << "Warning! polyhedron " << i << " not acquired because ";
		cout << obj->getName() << " has wrong type " << obj->getType();
		cout << ", it should be 'Geometry' or 'CSGGeometry'!" << endl;
	}

	if (polys[0] == 0) cout << "Warning! first polyhedron is 0! " << children[0]->getName() << endl;
	if (polys[1] == 0) cout << "Warning! second polyhedron is 0! " << children[1]->getName() << endl;
	if (polys[0] == 0 || polys[1] == 0) return false;

    if (polyhedron) delete polyhedron;
    polyhedron = 0;
	if (operation == "unite") polyhedron = unite(polys[0], polys[1]);
	else if(operation == "subtract") polyhedron = subtract(polys[0], polys[1]);
	else if(operation == "intersect") polyhedron = intersect(polys[0], polys[1]);
	else cout << "CSGGeometry: Warning: unexpected CSG operation!\n";

	// Clean up
	for (auto p : polys) delete p;

	if (polyhedron == 0) return false;
    setCSGGeometry(polyhedron);
	return true;
}
