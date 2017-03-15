#include "core/utils/VRDoublebuffer.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/OSGMaterial.h"
#include "core/objects/geometry/OSGGeometry.h"
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
	oct = new Octree(thresholdL);
	type = "CSGGeometry";
	dm->read(oldWorldTrans);
	polyhedron = new CGAL::Polyhedron();

	store("op", &operation);
}

CSGGeometry::~CSGGeometry() {}

CSGGeometryPtr CSGGeometry::ptr() { return static_pointer_cast<CSGGeometry>( shared_from_this() ); }
CSGGeometryPtr CSGGeometry::create(string name) { return shared_ptr<CSGGeometry>(new CSGGeometry(name) ); }

void CSGGeometry::setCSGGeometry(CGAL::Polyhedron *p) {
	if (!p->is_valid()) return;
	polyhedron = p;
	VRGeometry::setMesh( OSGGeometry::create((GeometryMTRecPtr)toOsgGeometry(p)) );
}

CGAL::Polyhedron* CSGGeometry::getCSGGeometry() {
    if (polyhedron == 0) {
        Matrix worldTransform = getWorldMatrix();
        bool success;
        polyhedron = toPolyhedron(getMesh()->geo, worldTransform, success);
    }

	return polyhedron;
}

void CSGGeometry::operate(CGAL::Polyhedron *p1, CGAL::Polyhedron *p2) {
	if (!p1->is_closed() || !p2->is_closed()) return;

    try {
        CGAL::Nef_Polyhedron np1(*p1), np2(*p2);
        if (operation == "unite") np1 += np2;
        else if(operation == "subtract") np1 -= np2;
        else if(operation == "intersect") np1 = np1.intersection(np2);
        else cout << "CSGGeometry: Warning: unexpected CSG operation!\n";
        np1.convert_to_polyhedron(*polyhedron);
    } catch (exception e) { cout << getName() << ": CSGGeometry::operate exception: " << e.what() << endl; }
}

void CSGGeometry::applyTransform(CGAL::Polyhedron* p, Matrix m) {
    if (p == 0) return;
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

	GeometryMTRecPtr mesh = Geometry::create();
	mesh->setPositions(positions);
	mesh->setNormals(normals);
	mesh->setIndices(indices);
	mesh->setTypes(types);
	mesh->setLengths(lengths);
	mesh->setMaterial(VRMaterial::getDefault()->getMaterial()->mat);
    createSharedIndex(mesh);
	calcVertexNormals(mesh, 0.523598775598 /*30 deg in rad*/);

	return GeometryTransitPtr(mesh);
}

size_t CSGGeometry::isKnownPoint(OSG::Pnt3f newPoint) {
	vector<void*> resultData = oct->radiusSearch(newPoint.subZero(), thresholdL);
	if (resultData.size() > 0) return *(size_t*)resultData.at(0);
	return numeric_limits<size_t>::max();
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

bool CSGGeometry::setEditMode(const bool editModeActive) {
	bool result = false;

	if (editMode != editModeActive) {
		editMode = editModeActive;
		if (editModeActive) enableEditMode();
		else {
			// We need to stop && report it back if an error occurred
			result = disableEditMode();

			// Promote news to our parents, but only if parents had edit mode disabled before
			VRObjectPtr obj = getParent();
			if (obj) {
                CSGGeometryPtr geo = static_pointer_cast<CSGGeometry>(obj);
                if (obj->getType() == "CSGGeometry" && !geo->getEditMode()) {
                    if (result) result = geo->setEditMode(true);
                    if (result) result = geo->setEditMode(false);
                }
			}

			// Promote news to _all_ children
			// Even if they had Edit Mode enabled till now we need their computed geometry to work on it
			for (auto c : children) {
				if (c->getType() != "CSGGeometry") continue;
                CSGGeometryPtr geo = static_pointer_cast<CSGGeometry>(c);
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

OSG_END_NAMESPACE
