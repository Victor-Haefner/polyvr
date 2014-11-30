/*
 * CsgGeometry.cpp
 *
 *  Created on: Jan 12, 2014
 *      Author: marcel
 */

#include "core/utils/VRDoublebuffer.h"
#include "CSGGeometry.h"
#include "core/objects/material/VRMaterial.h"
#include "PolyhedronBuilder.h"
#include <stdio.h>
#include <OpenSG/OSGTriangleIterator.h>
#include <OpenSG/OSGQuaternion.h>
#include <OpenSG/OSGGeoFunctions.h>
#include <OpenSG/OSGChunkMaterial.h>
#include <libxml++/nodes/element.h>

#define MEASURE_TIME // mesh conversion runtime measurements (toPolyhedron/toOsgGeometry)
#ifdef MEASURE_TIME
	#include <time.h>
#endif

//#include "../../../../csg_alt/GTSGeometry.h"
#include "core/scene/VRSceneManager.h"

OSG_BEGIN_NAMESPACE
using namespace std;
using namespace CSGApp;

vector<string> CSGGeometry::getOperations() {
	vector<string> ops;
	ops.push_back("unite");
    ops.push_back("subtract");
    ops.push_back("intersect");
    return ops;
}

CSGGeometry::CSGGeometry(string name) :
		VRGeometry(name),
		_operation(getOperations()[0]), // "unite"
		_editMode(true),
		_THRESHOLD(1e-4),
		_octree(_THRESHOLD)
{
	VRObject::type = "CSGGeometry";
	dm->read(_oldWorldTrans); // Initialization
	//VRScene* scene = VRSceneManager::getCurrent();
	/*GTSGeometry *boxA = GTSGeometry::createCube("boxA", 1.0);
	scene->add(boxA);
	GTSGeometry *boxB = GTSGeometry::createCube("boxB", 1.0);
	boxB->translate(Vec3f(.2, -.2, .2));
	boxA->subtract(*boxB);*/
}

CSGGeometry::~CSGGeometry()
{
}

void CSGGeometry::setCSGGeometry(CGAL::Polyhedron *p)
{
	assert(p->is_valid());
	VRGeometry::setMesh((GeometryRecPtr)toOsgGeometry(p));
}

CGAL::Polyhedron *CSGGeometry::getCSGGeometry()
{
	Matrix worldTransform = getWorldMatrix();
	CGAL::Polyhedron *p = toPolyhedron(getMesh(), &worldTransform);
	return p;
}

/*void CSGGeometry::update()
{
	cout << "CSGGeometry::update()\n";
	bool localChange = VRTransform::change; // We need to cache this value as VRTransform::update() will reset it
	VRTransform::update(); // Calculates the newWorldTrans we read below

	if(localChange) {
		Matrix newWorldTrans;
		dm->read(newWorldTrans);

		// Calculate transformation matrix from old to new state (change = invert(oldMatrix) * newMatrix)
		Matrix diffTrans;
		diffTrans.identity();
		if(!diffTrans.invertFrom(_oldWorldTrans)) {
			cout << getName() << ": Transformation matrix inversion wasn't successful!\n";
		}

		diffTrans.mult(newWorldTrans);

		// Useful for debugging, not needed at runtime
		//OSG::Vec3f translation;
		//OSG::Quaternion rotation;
		//OSG::Vec3f scaleFactor;
		//OSG::Quaternion scaleOrientation;
		//diffTrans.getTransform(translation, rotation, scaleFactor, scaleOrientation);
		//cout << "Transl: " << translation << endl;

		// Apply the transformation to our CSG geometry
		applyTransform(&_p, &diffTrans);
		_oldWorldTrans = newWorldTrans;
	}
}*/

CGAL::Polyhedron *CSGGeometry::subtract(CGAL::Polyhedron *minuend, CGAL::Polyhedron *subtrahend)
{
	assert(minuend->is_closed());
	assert(subtrahend->is_closed());

	CGAL::Nef_Polyhedron nef_minuend(*minuend);
	CGAL::Nef_Polyhedron nef_subtrahend(*subtrahend);
	nef_minuend -= nef_subtrahend;

	CGAL::Polyhedron *result = new CGAL::Polyhedron();
	nef_minuend.convert_to_polyhedron(*result);
	return result;
}

CGAL::Polyhedron *CSGGeometry::unite(CGAL::Polyhedron *first, CGAL::Polyhedron *second)
{
	assert(first->is_closed());
	assert(second->is_closed());

	CGAL::Nef_Polyhedron nef_first(*first);
	CGAL::Nef_Polyhedron nef_second(*second);
	nef_first += nef_second;

	CGAL::Polyhedron *result = new CGAL::Polyhedron();
	nef_first.convert_to_polyhedron(*result);
	return result;
}

CGAL::Polyhedron *CSGGeometry::intersect(CGAL::Polyhedron *first, CGAL::Polyhedron *second)
{
	assert(first->is_closed());
	assert(second->is_closed());

	CGAL::Nef_Polyhedron nef_first(*first);
	CGAL::Nef_Polyhedron nef_second(*second);
	CGAL::Nef_Polyhedron nef_result = nef_first.intersection(nef_second);

	CGAL::Polyhedron *result = new CGAL::Polyhedron();
	nef_result.convert_to_polyhedron(*result);
	return result;
}

GeometryTransitPtr CSGGeometry::toOsgGeometry(const CGAL::Polyhedron *p)
{
#ifdef MEASURE_TIME
	timespec startTime;
	clock_gettime(CLOCK_MONOTONIC, &startTime);
#endif

	GeoPnt3fPropertyRecPtr positions = GeoPnt3fProperty::create();
	GeoVec3fPropertyRecPtr normals = GeoVec3fProperty::create();
	GeoUInt32PropertyRecPtr indices = GeoUInt32Property::create();

	/*
	 * Iterate over all faces, add their vertices to 'positions' and write indices at
	 * the same time. Results in no shared vertices and therefore no normal interpolation between
	 * faces, but makes cubes look good. Well, well...
	 */

	Matrix localToWorld = getWorldMatrix();
	OSG::Vec3f translation;
	OSG::Quaternion rotation;
	OSG::Vec3f scaleFactor;
	OSG::Quaternion scaleOrientation;
	localToWorld.getTransform(translation, rotation, scaleFactor, scaleOrientation);
	Matrix worldToLocal;
	if(!worldToLocal.invertFrom(localToWorld)) {
		cout << "CSGGeometry::toOsgGeometry: Warning: matrix inversion failed!";
	}

	// Convert indices and positions
	int curIndex = 0;
	for(CGAL::Polyhedron::Facet_const_iterator it = p->facets_begin(); it != p->facets_end(); it++)
	{
		CGAL::Polyhedron::Halfedge_around_facet_const_circulator circ = it->facet_begin();
		do
		{
			CGAL::Point cgalPos = circ->vertex()->point();
			//cout << "toOsg: " << cgalPos << endl;
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
//			printf("index: %i\n", circ->vertex()->id());
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
	calcVertexNormals(mesh, 0.523598775598 /*30 deg in rad*/);

#ifdef MEASURE_TIME
	timespec endTime;
	clock_gettime(CLOCK_MONOTONIC, &endTime);
	cout << "toOsgGeometry: " << ((uint64_t)(endTime.tv_sec - startTime.tv_sec) * 1000000000) + endTime.tv_nsec - startTime.tv_nsec << " nsec\n";
#endif
	return GeometryTransitPtr(mesh);
}

size_t CSGGeometry::isKnownPoint(OSG::Pnt3f newPoint)
{
	// Search the octree
	vector<void*> resultData = _octree.radiusSearch(newPoint.x(), newPoint.y(), newPoint.z(), _THRESHOLD);
	if(resultData.size() > 0) {
		// We found a very similar vertex. Return the first find for now.
		return *(size_t*)resultData.at(0);
	}

	return numeric_limits<size_t>::max();
}

// Converts geometry to a polyhedron and applies the geometry node's world transform to the polyhedron.
// OpenSG geometry data isn't transformed itself but has an associated transform core. Both are unified for CGAL.
CGAL::Polyhedron* CSGGeometry::toPolyhedron(const GeometryRecPtr geometry, const Matrix *worldTransform)
{
#ifdef MEASURE_TIME
	timespec startTime;
	clock_gettime(CLOCK_MONOTONIC, &startTime);
#endif

	vector<CGAL::Point> positions;
	vector<size_t> indices;
	size_t curIndex = 0;

	// Convert triangles to indices and vertices, leaving out redundant vertices
	// (f.e. from different normals at an edge)
	TriangleIterator it(geometry);
	while(!it.isAtEnd()) {
		for(size_t i = 0; i < 3; i++) {
			// Copy 3 vertices of the triangle
			Pnt3f osgPos = it.getPosition(i);
			//cout << "osgPos: " << osgPos << endl;

			size_t knownIndex = isKnownPoint(osgPos);
			if(knownIndex < numeric_limits<size_t>::max()) {
				indices.push_back(knownIndex);
			}
			else {
				CGAL::Point cgalPos(osgPos.x(), osgPos.y(), osgPos.z());
				positions.push_back(cgalPos);
				indices.push_back(curIndex);
				size_t *curIndexPtr = new size_t;
				*curIndexPtr = curIndex;
				//cout << "index: " << *curIndexPtr << " pos: " << osgPos << endl << flush;
				_octree.add(Point(osgPos.x(), osgPos.y(), osgPos.z()), curIndexPtr);
				curIndex++;
			}
		}

		++it; // post-increment isn't implemented >.>
	}

	// Cleanup
	vector<void*> octreeData = _octree.getData();
	for(size_t i = 0; i < octreeData.size(); i++)
		delete (size_t*)octreeData[i];

	_octree = Octree(_THRESHOLD);


    cout << "\ntoPolyhedron " << getName() << " transformation : \n" << worldTransform << endl;
	cout << "size: " << positions.size() << " " << indices.size() << endl;
	for(size_t i = 0; i < positions.size(); i++) cout << positions[i] << endl;
	for(size_t i = 0; i < indices.size(); i += 3) cout << indices[i] << indices[i+1] << indices[i+2] << endl;


	// Construct the polyhedron from raw data
	CGAL::Polyhedron *result = new CGAL::Polyhedron();
	PolyhedronBuilder<CGAL::HalfedgeDS> builder(positions, indices);
	result->delegate(builder);
	if(!result->is_closed()) {
		throw std::runtime_error("The polyhedron is not a closed mesh!");
	}

	// Transform the polyhedron with the geometry's world transform matrix
	applyTransform(result, worldTransform);

#ifdef MEASURE_TIME
	timespec endTime;
	clock_gettime(CLOCK_MONOTONIC, &endTime);
	cout << "toPolyhedron: " << ((uint64_t)(endTime.tv_sec - startTime.tv_sec) * 1000000000) + endTime.tv_nsec - startTime.tv_nsec << " nsec\n";
#endif
	return result;
}

void CSGGeometry::enableEditMode()
{
	// Reset our result geometry
	CGAL::Polyhedron *p = new CGAL::Polyhedron();
	setCSGGeometry(p);
	delete p;

	for(size_t i = 0; i < children.size(); i++) {
		VRObject *obj = children[i];
		if(obj->getType() == string("Geometry") || obj->getType() == string("CSGGeometry")) {
			obj->setVisible(true);
		}
	}
}

bool CSGGeometry::disableEditMode()
{
	//cout << "child count: " << children.size() << endl;

	if(children.size() > 2) {
		cout << "CSGGeometry: Warning: editMode disabled with more than 2 children. The others will be ignored.\n";
	}
	else if(children.size() < 2) {
		cout << "CSGGeometry: Warning: editMode disabled with less than 2 children. Doing nothing.\n";
		return false;
	}

	// We need two child geometries to work with
	vector<CGAL::Polyhedron*> polys(2);

	// Prepare the polyhedra
	for(size_t i = 0; i < polys.size(); i++) {
		VRObject *obj = children[i];

		if(obj->getType() == string("Geometry")) {
			//cout << "Found Geometry child\n";
			obj->setVisible(false); // Hide the child
			VRGeometry *geo = dynamic_cast<VRGeometry*>(obj);

			try {
				// If the model is not a closed mesh without any doubled tris, this might/will fail.
				// Vertex deduplication is done there, but nothing more.

				Matrix worldTransform = geo->getWorldMatrix();
				CGAL::Polyhedron *result = toPolyhedron(geo->getMesh(), &worldTransform);
				polys[i] = result;
			}
			catch (exception e) {
				cout << getName() << ": Could not convert mesh data to polyhedron: " << e.what();
				obj->setVisible(true); // We stay in edit mode, so both children need to be visible
				return false;
			}
		}
		else if(obj->getType() == string("CSGGeometry")) {
			//cout << "Found CSGGeometry child\n";
			obj->setVisible(false);
			CSGGeometry *geo = dynamic_cast<CSGGeometry*>(obj);
			polys[i] = geo->getCSGGeometry();
		}
	}

	CGAL::Polyhedron *result;

	if(_operation == string("unite")) {
		#ifdef MEASURE_TIME
			timespec startTime;
			clock_gettime(CLOCK_MONOTONIC, &startTime);
		#endif
		result = unite(polys[0], polys[1]);
		#ifdef MEASURE_TIME
			timespec endTime;
			clock_gettime(CLOCK_MONOTONIC, &endTime);
			cout << "unite: " << ((uint64_t)(endTime.tv_sec - startTime.tv_sec) * 1000000000) + endTime.tv_nsec - startTime.tv_nsec << " nsec\n";
		#endif
	}
	else if(_operation == string("subtract")) {
		#ifdef MEASURE_TIME
			timespec startTime;
			clock_gettime(CLOCK_MONOTONIC, &startTime);
		#endif
		result = subtract(polys[0], polys[1]);
		#ifdef MEASURE_TIME
			timespec endTime;
			clock_gettime(CLOCK_MONOTONIC, &endTime);
			cout << "subtract: " << ((uint64_t)(endTime.tv_sec - startTime.tv_sec) * 1000000000) + endTime.tv_nsec - startTime.tv_nsec << " nsec\n";
		#endif
	}
	else if(_operation == string("intersect")) {
		#ifdef MEASURE_TIME
			timespec startTime;
			clock_gettime(CLOCK_MONOTONIC, &startTime);
		#endif
		result = intersect(polys[0], polys[1]);
		#ifdef MEASURE_TIME
			timespec endTime;
			clock_gettime(CLOCK_MONOTONIC, &endTime);
			cout << "subtract: " << ((uint64_t)(endTime.tv_sec - startTime.tv_sec) * 1000000000) + endTime.tv_nsec - startTime.tv_nsec << " nsec\n";
		#endif
	}
	else {
		cout << "CSGGeometry: Warning: unexpected CSG operation!\n";
	}

	// Clean up
	for(size_t i = 0; i < polys.size(); i++) {
		delete polys[i];
	}

	setCSGGeometry(result);
	delete result; // Is stored as OSG mesh now, Polyhedron not needed anymore
	return true;
}

/*void CSGGeometry::addChild(VRObject* child, bool osg)
{
	VRObject::addChild(child, osg);

	if(child->getType() == "Geometry") {
		cout << "Bruteforcing CSG geometry\n";
		VRGeometry *geo = (VRGeometry*)child;
		CGAL::Polyhedron *p = toPolyhedron(geo->getMesh(), geo->getWorldMatrix(false));
		CGAL::Nef_Polyhedron np(*p);
		cout << "valid: " << p->is_valid() << " closed: " << p->is_closed();
		cout << " np simple: " << np.is_simple() << " HE count: " << p->size_of_halfedges() << endl;

		//ofstream file("torus.off");
		//file << (*p);
		//cout << "Wrote polyhedron to torus.off\n";

		//CGAL::Polyhedron::Halfedge_iterator it;
		//for(it = p->halfedges_begin(); it != p->halfedges_end(); it++) {
		//	cout << "border: " << it->is_border() << endl;
		//}

		setCSGGeometry(*p);
	}
}*/

bool CSGGeometry::setEditMode(const bool editModeActive)
{
	bool result = false;

	if(_editMode != editModeActive) {
		if(editModeActive) {
			enableEditMode();
		}
		else {
			// We need to stop and report it back if an error occurred
			result = disableEditMode();

			// Promote news to our parents, but only if parents had edit mode disabled before
			VRObject *obj = getParent();
			if(obj->getType() == "CSGGeometry" && !((CSGGeometry*)obj)->getEditMode()) {
				CSGGeometry *geo = (CSGGeometry*)obj;
				if(result)
					result = geo->setEditMode(true);
				if(result)
					result = geo->setEditMode(false);
			}

			// Promote news to _all_ children
			// Even if they had Edit Mode enabled till now we need their computed geometry to work on it
			for(size_t i = 0; i < children.size(); i++) {
				if(children[i]->getType() == "CSGGeometry") {
					CSGGeometry *geo = (CSGGeometry*)children[i];
					if(geo->getEditMode() && result)
						result = geo->setEditMode(false);
				}
			}
		}

		_editMode = editModeActive;
	}

	return result;
}

void CSGGeometry::setOperation(const string op)
{
	// Check if op is in our set of valid operations
	bool validOp = false;
	vector<string> ops = getOperations();
	for(vector<string>::const_iterator it = ops.begin(); it != ops.end(); it++) {
		if(op.compare(*it) == 0)
			validOp = true;
	}

	assert(validOp);
	_operation = op;

	if(!getEditMode()) {
		// If edit mode was disabled before, enable it now to allow for corrections before computing
		// TODO This should maybe be propagated to the GUI
		setEditMode(true);
	}
}

bool CSGGeometry::getEditMode() const { return _editMode; }
string CSGGeometry::getOperation() const { return _operation; }

void CSGGeometry::saveContent(xmlpp::Element* e)
{
	VRGeometry::saveContent(e);
	e->set_attribute("op", _operation);
}

void CSGGeometry::loadContent(xmlpp::Element* e)
{
	VRGeometry::loadContent(e);
	if(e->get_attribute("op")) {
		_operation = e->get_attribute("op")->get_value();
	}
	assert(_operation.length() != 0);
}

OSG_END_NAMESPACE
