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
	TriangleIterator it;
	auto gpos = geometry->getPositions();

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

    vector<CGAL::Point> positions;
	vector<size_t> indices;
    vector<Vec3f> pos;
	vector<int> inds;
	size_t curIndex = 0;

	// Convert triangles to cgal indices and vertices
	for (it = TriangleIterator(geometry); !it.isAtEnd() ;++it) {
        vector<size_t> IDs(3);
        for (int i=0; i<3; i++) IDs[i] = isKnownPoint( it.getPosition(i) );

		for (int i=0; i<3; i++) {
			if (IDs[i] == numeric_limits<size_t>::max()) {
                Vec3f p = Vec3f(it.getPosition(i));
				pos.push_back(p);
				positions.push_back( CGAL::Point(p[0], p[1], p[2]) );
				IDs[i] = curIndex;
                //cout << "add point " << curIndex << "   " << osgPos << endl;
				size_t *curIndexPtr = new size_t;
				*curIndexPtr = curIndex;
				oct->add(OcPoint(p[0], p[1], p[2]), curIndexPtr);
				curIndex++;
			}
		}

        //cout << "add triangle " << IDs[0] << " " << IDs[1] << " " << IDs[2] << endl;
		if (IDs[0] == IDs[1] || IDs[0] == IDs[2] || IDs[1] == IDs[2]) continue; // ignore flat triangles

		for (int i=0; i<3; i++) indices.push_back(IDs[i]);
		for (int i=0; i<3; i++) inds.push_back(IDs[i]);
	}

	// Cleanup
	for (void* o : oct->getData()) delete (size_t*)o;
	delete oct;
	oct = new Octree(THRESHOLD);

	// Construct the polyhedron from raw data
    success = true;
	CGAL::Polyhedron *result = new CGAL::Polyhedron();
	PolyhedronBuilder<CGAL::HalfedgeDS> builder(positions, indices);
	result->delegate(builder);
	if (!result->is_closed()) {
        success = false;
        cout << "Error: The polyhedron is not a closed mesh!" << endl;
        create(GL_TRIANGLES, pos, pos, inds);
	}

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
            cout << "child: " << geo->getName() << " toPolyhedron\n";
            bool success;
			try {
			    polys[i] = toPolyhedron( geo->getMesh(), geo->getWorldMatrix(), success );
			} catch (exception e) {
			    success = false;
			    cout << getName() << ": toPolyhedron exception: " << e.what() << endl;
			}

            if (!success) {
			    cout << getName() << ": toPolyhedron went totaly wrong :(\n";
                //setCSGGeometry(polys[i]);
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
