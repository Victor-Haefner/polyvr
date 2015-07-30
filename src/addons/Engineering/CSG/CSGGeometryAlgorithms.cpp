#include "CSGGeometry.h"
#include "CGALTypedefs.h"
#include "core/math/Octree.h"
#include "core/objects/material/VRMaterial.h"

#include "PolyhedronBuilder.h"

#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGGeometry.h>
#include <OpenSG/OSGTriangleIterator.h>
#include <OpenSG/OSGGeoFunctions.h>

using namespace std;
using namespace OSG;

void CSGGeometry::setThreshold(float tL, float tA) { thresholdL = tL; thresholdA = tA; }
Vec2f CSGGeometry::getThreshold() { return Vec2f(thresholdL, thresholdA); }

float calcArea(Vec3f p1, Vec3f p2, Vec3f p3) {
    Vec3f v1 = p2-p1;
    Vec3f v2 = p3-p1;
    return v1.cross(v2).length()*0.5;
}

/*void removeFlatTriangles() {
    ;
}*/


 // Converts geometry to a polyhedron && applies the geometry node's world transform to the polyhedron.
// OpenSG geometry data isn't transformed itself but has an associated transform core. Both are unified for CGAL.
CGAL::Polyhedron* CSGGeometry::toPolyhedron(GeometryRecPtr geometry, Matrix worldTransform, bool& success) {
	TriangleIterator it;
	auto gpos = geometry->getPositions();

	//float THRESHOLD2 = thresholdL*thresholdL;

	// TODO: first merge all points!
	//       second fix flat triangles

	// fix flat triangles (all three points aligned)
	int NA = 0;
	do {
        NA = 0;
        for (it = TriangleIterator(geometry); !it.isAtEnd() ;++it) {
            vector<Pnt3f> p(3);
            vector<Vec3f> v(3);
            Vec3i vi = Vec3i(it.getPositionIndex(0), it.getPositionIndex(1), it.getPositionIndex(2));
            for (int i=0; i<3; i++) p[i] = it.getPosition(i);
            v[0] = p[2]-p[1]; v[1] = p[2]-p[0]; v[2] = p[1]-p[0];
            float A = (v[2].cross(v[1])).length();
            if (A < thresholdA) { // small area, flat triangle?
                //cout << "small area " << A << endl;
                //for (int i=0; i<3; i++) cout << " pi " << p[i] << " vi " << vi[i] << " L " << v[i].squareLength() << endl;
                if (v[0].squareLength() == 0) continue; // check if two points close, then ignore
                if (v[1].squareLength() == 0) continue;
                if (v[2].squareLength() == 0) continue;

                int imax = 0; int imax2 = 0; // point to move, point to move to
                for (int i=1; i<3; i++) if (v[i].squareLength() > v[imax].squareLength()) imax = i;
                if (imax2 == imax) imax2++;
                for (int i=1; i<3; i++) if (v[i].squareLength() > v[imax2].squareLength() && i != imax) imax2 = i;

                int j = imax2;//(im+1)%3;
                cout << "set p[" << j << "] = " << p[j] << " with index i[" << imax << "] = " << vi[imax] << endl;
                gpos->setValue(p[j], vi[imax]);
                NA++;
                for (int i=0; i<3; i++) p[i] = it.getPosition(i);
                //cout << " result: " << endl;
                //for (int i=0; i<3; i++) cout << "  pi " << p[i] << endl;
            }
        }
        cout << "fixed " << NA << " flat triangles\n";
	} while(NA);

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

		//if (calcArea(pos[IDs[0]], pos[IDs[1]], pos[IDs[2]]) < thresholdA) continue; // induces holes
		if (calcArea(pos[IDs[0]], pos[IDs[1]], pos[IDs[2]]) == 0) continue;

		bool verbose = false;
		vector<size_t> debIDs;
		debIDs.push_back(754);
		debIDs.push_back(755);
		for (int i : IDs) for (int j : debIDs) if (i == j) verbose = true;
		if (verbose) {
            cout << "IDs " << IDs[0] << " " << IDs[1] << " " << IDs[2] << endl;
            cout << " pos " << pos[IDs[0]] << "   " << pos[IDs[1]] << "   " << pos[IDs[2]] << endl;
            cout << " A " << calcArea(pos[IDs[0]], pos[IDs[1]], pos[IDs[2]]) << endl;
		}

		for (int i=0; i<3; i++) indices.push_back(IDs[i]);
		for (int i=0; i<3; i++) inds.push_back(IDs[i]);
	}

	// Cleanup
	for (void* o : oct->getData()) delete (size_t*)o;
	delete oct;
	oct = new Octree(thresholdL);

	// Construct the polyhedron from raw data
    success = true;
	CGAL::Polyhedron *result = new CGAL::Polyhedron();
	PolyhedronBuilder<CGAL::HalfedgeDS> builder(positions, indices);
	result->delegate(builder);
	if (!result->is_closed()) {
        success = false;
        cout << "Error: The polyhedron is not a closed mesh!" << endl;
        create(GL_TRIANGLES, pos, pos, inds);
        setWorldMatrix(worldTransform);
        createSharedIndex(mesh);
        calcVertexNormals(mesh, 0.523598775598 /*30 deg in rad*/);
        getMaterial()->setFrontBackModes(GL_FILL, GL_NONE);
	}

	// Transform the polyhedron with the geometry's world transform matrix
	applyTransform(result, worldTransform);
	return result;
}

void CSGGeometry::markEdges(vector<Vec2i> edges) {
    int N = getMesh()->getPositions()->size();
    GeoColor3fPropertyRecPtr cols = GeoColor3fProperty::create();
    cols->resize(N);
    for (int i=0; i<N; i++) cols->setValue(Color3f(1,1,1),i);
    for (auto e : edges) {
        cols->setValue(Color3f(1,0,0),e[0]);
        cols->setValue(Color3f(1,0,0),e[1]);
    }
    setColors(cols);
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
                //obj->setVisible(true); // We stay in edit mode, so both children need to be visible
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
    if (!polys[0]->is_closed() || !polys[1]->is_closed()) return false;

    operate(polys[0], polys[1]);
	for (auto p : polys) delete p; // clean up
    setCSGGeometry(polyhedron);
	return true;
}
