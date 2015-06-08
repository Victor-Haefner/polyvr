#include "core/utils/VRDoublebuffer.h"
#include "core/objects/material/VRMaterial.h"
#include "core/math/Octree.h"

#include "CSGGeometry.h"
#include "CGALTypedefs.h"
#include "PolyhedronBuilder.h"

#include <stdio.h>
#include <OpenSG/OSGTriangleIterator.h>
#include <OpenSG/OSGQuaternion.h>
#include <OpenSG/OSGGeoFunctions.h>
#include <OpenSG/OSGChunkMaterial.h>
#include <OpenSG/OSGMultiPassMaterial.h>
#include <libxml++/nodes/element.h>

//#include "../../../../csg_alt/GTSGeometry.h"
#include "core/scene/VRSceneManager.h"

OSG_BEGIN_NAMESPACE
using namespace std;

vector<string> CSGGeometry::getOperations() {
	vector<string> ops;
	ops.push_back("unite");
    ops.push_back("subtract");
    ops.push_back("intersect");
    return ops;
}

CSGGeometry::CSGGeometry(string name) : VRGeometry(name) {
	oct = new Octree(THRESHOLD);
	type = "CSGGeometry";
	dm->read(oldWorldTrans);
}

CSGGeometry::~CSGGeometry() {}

void CSGGeometry::setCSGGeometry(CGAL::Polyhedron *p) {
	if (!p->is_valid()) return;
	polyhedron = p;
	VRGeometry::setMesh((GeometryRecPtr)toOsgGeometry(p));
}

CGAL::Polyhedron* CSGGeometry::getCSGGeometry() {
    if (polyhedron == 0) {
        Matrix worldTransform = getWorldMatrix();
        polyhedron = toPolyhedron(getMesh(), worldTransform);
    }

	return polyhedron;
}

CGAL::Polyhedron* CSGGeometry::subtract(CGAL::Polyhedron *minuend, CGAL::Polyhedron *subtrahend) {
	if (!minuend->is_closed()) return 0;
	if (!subtrahend->is_closed()) return 0;

	CGAL::Nef_Polyhedron nef_minuend(*minuend);
	CGAL::Nef_Polyhedron nef_subtrahend(*subtrahend);
	nef_minuend -= nef_subtrahend;

	CGAL::Polyhedron *result = new CGAL::Polyhedron();
	nef_minuend.convert_to_polyhedron(*result);
	return result;
}

CGAL::Polyhedron* CSGGeometry::unite(CGAL::Polyhedron *first, CGAL::Polyhedron *second) {
	if (!first->is_closed()) return 0;
	if (!second->is_closed()) return 0;

	CGAL::Nef_Polyhedron nef_first(*first);
	CGAL::Nef_Polyhedron nef_second(*second);
	nef_first += nef_second;

	CGAL::Polyhedron* result = new CGAL::Polyhedron();
	nef_first.convert_to_polyhedron(*result);
	return result;
}

CGAL::Polyhedron* CSGGeometry::intersect(CGAL::Polyhedron *first, CGAL::Polyhedron *second) {
	if (!first->is_closed()) return 0;
	if (!second->is_closed()) return 0;

	CGAL::Nef_Polyhedron nef_first(*first);
	CGAL::Nef_Polyhedron nef_second(*second);
	CGAL::Nef_Polyhedron nef_result = nef_first.intersection(nef_second);

	CGAL::Polyhedron *result = new CGAL::Polyhedron();
	nef_result.convert_to_polyhedron(*result);
	return result;
}

void CSGGeometry::applyTransform(CGAL::Polyhedron* p, Matrix m) {
    CGAL::Transformation t(m[0][0], m[1][0], m[2][0], m[3][0],
                           m[0][1], m[1][1], m[2][1], m[3][1],
                           m[0][2], m[1][2], m[2][2], m[3][2],
                                                      m[3][3]);
    transform(p->points_begin(), p->points_end(), p->points_begin(), t);
}

GeometryTransitPtr CSGGeometry::toOsgGeometry(CGAL::Polyhedron *p) {
	GeoPnt3fPropertyRecPtr positions = GeoPnt3fProperty::create();
	GeoVec3fPropertyRecPtr normals = GeoVec3fProperty::create();
	GeoUInt32PropertyRecPtr indices = GeoUInt32Property::create();

	/*
	 * Iterate over all faces, add their vertices to 'positions' && write indices at
	 * the same time. Results in no shared vertices && therefore no normal interpolation between
	 * faces, but makes cubes look good. Well, well...
	 */

	Matrix localToWorld = getWorldMatrix();
	OSG::Vec3f translation;
	OSG::Quaternion rotation;
	OSG::Vec3f scaleFactor;
	OSG::Quaternion scaleOrientation;
	localToWorld.getTransform(translation, rotation, scaleFactor, scaleOrientation);
	Matrix worldToLocal;
	worldToLocal.invertFrom(localToWorld);

	// Convert indices && positions
	int curIndex = 0;
	for (CGAL::Polyhedron::Facet_const_iterator it = p->facets_begin(); it != p->facets_end(); it++) {
		CGAL::Polyhedron::Halfedge_around_facet_const_circulator circ = it->facet_begin();
		do {
			CGAL::Point cgalPos = circ->vertex()->point();
			// We need to transform each point from global coordinates into our local coordinate system
			// (CGAL uses global, OpenSG has geometry in node-local coords)
			OSG::Vec3f vecPos = OSG::Vec3f(CGAL::to_double(cgalPos.x()),
											  CGAL::to_double(cgalPos.y()),
											  CGAL::to_double(cgalPos.z()));
			OSG::Vec3f localVec = worldToLocal * (vecPos - translation);
			OSG::Pnt3f osgPos(localVec.x(), localVec.y(), localVec.z());

			positions->addValue(osgPos);
			normals->addValue(Vec3f(0,1,0));
			indices->addValue(curIndex);
			curIndex++;
		} while (++circ != it->facet_begin());
	}

	GeoUInt8PropertyRecPtr types = GeoUInt8Property::create();
	types->addValue(GL_TRIANGLES);
	GeoUInt32PropertyRecPtr lengths = GeoUInt32Property::create();
	lengths->addValue(indices->size());

	GeometryRecPtr mesh = Geometry::create();
	mesh->setPositions(positions);
	mesh->setNormals(normals);
	mesh->setIndices(indices);
	mesh->setTypes(types);
	mesh->setLengths(lengths);
	mesh->setMaterial(VRMaterial::getDefault()->getMaterial());
    createSharedIndex(mesh);
	calcVertexNormals(mesh, 0.523598775598 /*30 deg in rad*/);

	return GeometryTransitPtr(mesh);
}

size_t CSGGeometry::isKnownPoint(OSG::Pnt3f newPoint) {
	vector<void*> resultData = oct->radiusSearch(newPoint.x(), newPoint.y(), newPoint.z(), THRESHOLD);
	if (resultData.size() > 0) return *(size_t*)resultData.at(0);
	return numeric_limits<size_t>::max();
}

// Converts geometry to a polyhedron && applies the geometry node's world transform to the polyhedron.
// OpenSG geometry data isn't transformed itself but has an associated transform core. Both are unified for CGAL.
CGAL::Polyhedron* CSGGeometry::toPolyhedron(GeometryRecPtr geometry, Matrix worldTransform) {
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

void CSGGeometry::enableEditMode() {
	// Reset our result geometry
	CGAL::Polyhedron* p = new CGAL::Polyhedron();
	setCSGGeometry(p);
	delete p;

	for (auto c : children) {
		if (c->getType() == string("Geometry") || c->getType() == string("CSGGeometry")) c->setVisible(true);
	}
}

bool CSGGeometry::disableEditMode() {
	if (children.size() != 2) { cout << "CSGGeometry: Warning: editMode disabled with less than 2 children. Doing nothing.\n"; return false; }

	vector<CGAL::Polyhedron*> polys(2); // We need two child geometries to work with

	for (int i=0; i<2; i++) { // Prepare the polyhedra
		VRObject *obj = children[i];
        obj->setVisible(false);

		if (obj->getType() == string("Geometry")) {
			VRGeometry *geo = dynamic_cast<VRGeometry*>(obj);
			try {
			    polys[i] = toPolyhedron( geo->getMesh(), geo->getWorldMatrix() );
			} catch (exception e) {
			    cout << getName() << ": Could not convert mesh data to polyhedron: " << e.what();
				obj->setVisible(true); // We stay in edit mode, so both children need to be visible
				return false;
			}
		} else if(obj->getType() == "CSGGeometry") {
			CSGGeometry *geo = dynamic_cast<CSGGeometry*>(obj);
			polys[i] = geo->getCSGGeometry(); // TODO: where does this come from?? keep the old!
		}
	}

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

bool CSGGeometry::setEditMode(const bool editModeActive) {
	bool result = false;

	if (editMode != editModeActive) {
		editMode = editModeActive;
		if (editModeActive) enableEditMode();
		else {
			// We need to stop && report it back if an error occurred
			result = disableEditMode();

			// Promote news to our parents, but only if parents had edit mode disabled before
			VRObject *obj = getParent();
			if(obj->getType() == "CSGGeometry" && !((CSGGeometry*)obj)->getEditMode()) {
				CSGGeometry *geo = (CSGGeometry*)obj;
				if (result) result = geo->setEditMode(true);
				if (result) result = geo->setEditMode(false);
			}

			// Promote news to _all_ children
			// Even if they had Edit Mode enabled till now we need their computed geometry to work on it
			for (auto c : children) {
				if (c->getType() != "CSGGeometry") continue;
                CSGGeometry *geo = (CSGGeometry*)c;
                if (geo->getEditMode() && result) result = geo->setEditMode(false);
			}
		}
	}

	return result;
}

void CSGGeometry::setOperation(string op) {
	// Check if op is in our set of valid operations
	if (std::find(getOperations().begin(), getOperations().end(), op) == getOperations().end()) return;
	operation = op;

	// If edit mode was disabled before, enable it now to allow for corrections before computing
    // TODO This should maybe be propagated to the GUI
	if (!getEditMode()) setEditMode(true);
}

bool CSGGeometry::getEditMode() { return editMode; }
string CSGGeometry::getOperation() { return operation; }

void CSGGeometry::saveContent(xmlpp::Element* e) {
	VRGeometry::saveContent(e);
	e->set_attribute("op", operation);
}

void CSGGeometry::loadContent(xmlpp::Element* e) {
	VRGeometry::loadContent(e);
	if (e->get_attribute("op")) operation = e->get_attribute("op")->get_value();
}

OSG_END_NAMESPACE
