#include "CSGGeometry.h"
#include "CGALTypedefs.h"
#include "PolyhedronBuilder.h"

#include "core/objects/geometry/OSGGeometry.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/material/VRMaterial.h"
#include "core/math/Octree.h"
#include "core/math/pose.h"
#include "core/utils/toString.h"

#include <OpenSG/OSGGeoFunctions.h>

using namespace OSG;


vector<string> CSGGeometry::getOperations() {
	vector<string> ops;
	ops.push_back("unite");
    ops.push_back("subtract");
    ops.push_back("intersect");
    return ops;
}

CSGGeometry::CSGGeometry(string name) : VRGeometry(name) {
	store("op", &operation);
}

CSGGeometry::~CSGGeometry() {}
CSGGeometryPtr CSGGeometry::ptr() { return static_pointer_cast<CSGGeometry>( shared_from_this() ); }

CSGGeometryPtr CSGGeometry::create(string name) {
    auto c = CSGGeometryPtr( new CSGGeometry(name) );
    c->init();
    return c;
}

void CSGGeometry::init() {
	oct = Octree::create(thresholdL);
	type = "CSGGeometry";
	setPose(oldWorldTrans);
	polyhedron = new CGALPolyhedron();
}

void CSGGeometry::setCSGGeometry(CGALPolyhedron* p) {
	if (!p->polyhedron->is_valid()) return;
	polyhedron = p;
	toOsgGeometry(p);
}

CGALPolyhedron* CSGGeometry::getCSGGeometry() {
    if (!polyhedron) {
        bool success;
        polyhedron = toPolyhedron(ptr(), getWorldPose(), success);
    }
	return polyhedron;
}

void CSGGeometry::applyTransform(CGALPolyhedron* P, PosePtr pose) {
    auto p = P->polyhedron;
    if (p == 0) return;
    Matrix4d m = pose->asMatrix();
    CGAL::Transformation t(m[0][0], m[1][0], m[2][0], m[3][0],
                           m[0][1], m[1][1], m[2][1], m[3][1],
                           m[0][2], m[1][2], m[2][2], m[3][2],
                                                      m[3][3]);
    transform(p->points_begin(), p->points_end(), p->points_begin(), t);
}

void CSGGeometry::toOsgGeometry(CGALPolyhedron *P) {
    auto p = P->polyhedron;
    VRGeoData data;

	//Iterate over all faces, add their vertices to 'positions' and write indices at
	//the same time. Results in no shared vertices && therefore no normal interpolation between
	//faces, but makes cubes look good. Well, well...

	Matrix4d localToWorld = getWorldMatrix();
	Vec3d translation = Vec3d(localToWorld[3]);
	Matrix4d worldToLocal;
	worldToLocal.invertFrom(localToWorld);

	// Convert indices && positions
	for (auto it = p->facets_begin(); it != p->facets_end(); it++) {
		auto circ = it->facet_begin();
		do {
			CGAL::Point cgalPos = circ->vertex()->point();
			// We need to transform each point from global coordinates into our local coordinate system
			// (CGAL uses global, OpenSG has geometry in node-local coords)
			Vec3d vecPos = Vec3d(CGAL::to_double(cgalPos.x()), CGAL::to_double(cgalPos.y()), CGAL::to_double(cgalPos.z()));
			Vec3d localVec = worldToLocal * (vecPos - translation);
			Pnt3d osgPos(localVec.x(), localVec.y(), localVec.z());
			data.pushVert(osgPos, Vec3d(0,1,0));
		} while (++circ != it->facet_begin());
		data.pushTri();
	}

	data.apply(ptr());
	setMaterial( VRMaterial::getDefault() );
    createSharedIndex(mesh->geo);
	calcVertexNormals(mesh->geo, 0.523598775598); // 30 degrees
}

size_t CSGGeometry::isKnownPoint(Pnt3f newPoint) {
	vector<void*> resultData = oct->radiusSearch(Vec3d(newPoint), thresholdL);
	if (resultData.size() > 0) return *(size_t*)resultData.at(0);
	return numeric_limits<size_t>::max();
}

void CSGGeometry::enableEditMode() { // Reset our result geometry
	CGALPolyhedron* p = new CGALPolyhedron();
	setCSGGeometry(p);
	for (auto c : children) if (c->hasTag("geometry")) c->setVisible(true);
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
			CSGGeometryPtr geo = dynamic_pointer_cast<CSGGeometry>( getParent() );
			if (geo) {
                if (!geo->getEditMode()) {
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
