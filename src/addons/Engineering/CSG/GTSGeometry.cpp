/*
 * GTSGeometry.cpp
 *
 *  Created on: Feb 17, 2014
 *      Author: marcel
 */

#include <string>
#include <iostream>
#include "GTSGeometry.h"

using namespace std;

namespace OSG
{
namespace CSGApp
{

GTSGeometry *GTSGeometry::createCube(std::string name, double size)
{
	// Construct the GTS surface that represents our geometry
	GtsSurface *s = gts_surface_new(gts_surface_class(), gts_face_class(), gts_edge_class(), gts_vertex_class());

	// Save the data into a standard C data format so that adapting is easier...
	// GTS uses double as coordinate type
	// This results in a non-closed and self-intersecting surface. Using other generation method below.
//	vector<vector<double> > vertices;
//	vector<double> tmp;
//	tmp.push_back(-size/2.0); tmp.push_back(-size/2.0); tmp.push_back(size/2.0);
//	vertices.push_back(tmp);
//	tmp.clear();
//	tmp.push_back(size/2.0); tmp.push_back(-size/2.0); tmp.push_back(size/2.0);
//	vertices.push_back(tmp);
//	tmp.clear();
//	tmp.push_back(size/2.0); tmp.push_back(-size/2.0); tmp.push_back(-size/2.0);
//	vertices.push_back(tmp);
//	tmp.clear();
//	tmp.push_back(-size/2.0); tmp.push_back(-size/2.0); tmp.push_back(-size/2.0);
//	vertices.push_back(tmp);
//	tmp.clear();
//	tmp.push_back(-size/2.0); tmp.push_back(size/2.0); tmp.push_back(size/2.0);
//	vertices.push_back(tmp);
//	tmp.clear();
//	tmp.push_back(size/2.0); tmp.push_back(size/2.0); tmp.push_back(size/2.0);
//	vertices.push_back(tmp);
//	tmp.clear();
//	tmp.push_back(size/2.0); tmp.push_back(size/2.0); tmp.push_back(-size/2.0);
//	vertices.push_back(tmp);
//	tmp.clear();
//	tmp.push_back(-size/2.0); tmp.push_back(size/2.0); tmp.push_back(-size/2.0);
//	vertices.push_back(tmp);
//
//	// We need to define each triangle by hand. Are cubes that complex that no formula can elegantly generate one?!
//	vector<int> indices;
//	indices.push_back(3); // bottom
//	indices.push_back(2);
//	indices.push_back(1);
//	indices.push_back(3);
//	indices.push_back(1);
//	indices.push_back(0);
//
//	indices.push_back(4); // top
//	indices.push_back(5);
//	indices.push_back(6);
//	indices.push_back(4);
//	indices.push_back(6);
//	indices.push_back(7);
//
//	indices.push_back(0); // front
//	indices.push_back(1);
//	indices.push_back(5);
//	indices.push_back(0);
//	indices.push_back(5);
//	indices.push_back(4);
//
//	indices.push_back(2); // back
//	indices.push_back(3);
//	indices.push_back(7);
//	indices.push_back(2);
//	indices.push_back(7);
//	indices.push_back(6);
//
//	indices.push_back(3); // left
//	indices.push_back(0);
//	indices.push_back(4);
//	indices.push_back(3);
//	indices.push_back(4);
//	indices.push_back(7);
//
//	indices.push_back(1); // right
//	indices.push_back(2);
//	indices.push_back(6);
//	indices.push_back(1);
//	indices.push_back(6);
//	indices.push_back(5);
//
//	// Create faces from the vertex/index data
//	assert(indices.size() % 3 == 0);
//	vector<int>::const_iterator it = indices.begin();
//	while(it != indices.end())
//	{
//		GtsVertex *v1 = gts_vertex_new(gts_vertex_class(), vertices[*it][0], vertices[*it][1], vertices[*it][2]);
//		it++;
//		GtsVertex *v2 = gts_vertex_new(gts_vertex_class(), vertices[*it][0], vertices[*it][1], vertices[*it][2]);
//		it++;
//		GtsVertex *v3 = gts_vertex_new(gts_vertex_class(), vertices[*it][0], vertices[*it][1], vertices[*it][2]);
//		it++;
//		GtsEdge *e1 = gts_edge_new(gts_edge_class(), v1, v2); // Not exactly a comfy interface...
//		GtsEdge *e2 = gts_edge_new(gts_edge_class(), v2, v3);
//		GtsEdge *e3 = gts_edge_new(gts_edge_class(), v3, v1);
//		GtsFace *f = gts_face_new(gts_face_class(), e1, e2, e3);
//		gts_surface_add_face(s, f);
//	}

	// Cube generation code taken from GTS sample
	// http://gerris.dalembert.upmc.fr/darcs/gts-stable/test/boolean/cubes.c
	GtsVertex * v0 = gts_vertex_new (s->vertex_class, -size/2.0, -size/2.0,  size/2.0);
	GtsVertex * v1 = gts_vertex_new (s->vertex_class,  size/2.0, -size/2.0,  size/2.0);
	GtsVertex * v2 = gts_vertex_new (s->vertex_class,  size/2.0, -size/2.0, -size/2.0);
	GtsVertex * v3 = gts_vertex_new (s->vertex_class, -size/2.0, -size/2.0, -size/2.0);
	GtsVertex * v4 = gts_vertex_new (s->vertex_class, -size/2.0,  size/2.0,  size/2.0);
	GtsVertex * v5 = gts_vertex_new (s->vertex_class,  size/2.0,  size/2.0,  size/2.0);
	GtsVertex * v6 = gts_vertex_new (s->vertex_class,  size/2.0,  size/2.0, -size/2.0);
	GtsVertex * v7 = gts_vertex_new (s->vertex_class, -size/2.0,  size/2.0, -size/2.0);

	GtsEdge * e1 = gts_edge_new (s->edge_class, v0, v1);
	GtsEdge * e2 = gts_edge_new (s->edge_class, v1, v2);
	GtsEdge * e3 = gts_edge_new (s->edge_class, v2, v3);
	GtsEdge * e4 = gts_edge_new (s->edge_class, v3, v0);
	GtsEdge * e5 = gts_edge_new (s->edge_class, v0, v2);

	GtsEdge * e6 = gts_edge_new (s->edge_class, v4, v5);
	GtsEdge * e7 = gts_edge_new (s->edge_class, v5, v6);
	GtsEdge * e8 = gts_edge_new (s->edge_class, v6, v7);
	GtsEdge * e9 = gts_edge_new (s->edge_class, v7, v4);
	GtsEdge * e10 = gts_edge_new (s->edge_class, v4, v6);

	GtsEdge * e11 = gts_edge_new (s->edge_class, v3, v7);
	GtsEdge * e12 = gts_edge_new (s->edge_class, v2, v6);
	GtsEdge * e13 = gts_edge_new (s->edge_class, v1, v5);
	GtsEdge * e14 = gts_edge_new (s->edge_class, v0, v4);

	GtsEdge * e15 = gts_edge_new (s->edge_class, v1, v6);
	GtsEdge * e16 = gts_edge_new (s->edge_class, v2, v7);
	GtsEdge * e17 = gts_edge_new (s->edge_class, v3, v4);
	GtsEdge * e18 = gts_edge_new (s->edge_class, v0, v5);

	GtsFaceClass * klass = gts_face_class ();
	gts_surface_add_face (s, gts_face_new (klass, e1, e2, e5));
	gts_surface_add_face (s, gts_face_new (klass, e5, e3, e4));
	gts_surface_add_face (s, gts_face_new (klass, e6, e10, e7));
	gts_surface_add_face (s, gts_face_new (klass, e10, e9, e8));
	gts_surface_add_face (s, gts_face_new (klass, e2, e15, e12));
	gts_surface_add_face (s, gts_face_new (klass, e15, e13, e7));
	gts_surface_add_face (s, gts_face_new (klass, e3, e16, e11));
	gts_surface_add_face (s, gts_face_new (klass, e16, e12, e8));
	gts_surface_add_face (s, gts_face_new (klass, e17, e14, e4));
	gts_surface_add_face (s, gts_face_new (klass, e17, e11, e9));
	gts_surface_add_face (s, gts_face_new (klass, e18, e13, e1));
	gts_surface_add_face (s, gts_face_new (klass, e18, e14, e6));

	// Surface validity is checked in GTSGeometry, don't do that here
	return new GTSGeometry(name, s);
}

GTSGeometry::GTSGeometry(string name, GtsSurface *s)
{
	setCSGGeometry(s);
}

GTSGeometry::~GTSGeometry()
{
	free(_s);
}

void GTSGeometry::setCSGGeometry(GtsSurface *s)
{
	assert(s != 0);
	assert(gts_surface_is_closed(s));
	assert(gts_surface_is_manifold(s));
	assert(gts_surface_is_orientable(s));
//	assert(!gts_surface_is_self_intersecting(s));
	GtsSurface *intersecting = gts_surface_is_self_intersecting(s);
	if(intersecting != 0) {
//		_s = intersecting; // Only keep the intersecting faces
		_s = s;
		cout << "Warning: GTS surface " << name << " has self intersecting faces!\n";
	} else {
		_s = s;
	}

	VRGeometry::setMesh((GeometryRecPtr)toOsgGeometry());
}

GtsSurface *GTSGeometry::getCSGGeometry()
{
	return _s;
}

void GTSGeometry::translate(OSG::Vec3f v)
{
	VRGeometry::translate(v);
	GtsVector gtsV;
	gtsV[0] = v.x();
	gtsV[1] = v.y();
	gtsV[2] = v.z();
	GtsMatrix *m = gts_matrix_translate(NULL, gtsV);
	gts_surface_foreach_vertex(_s, (GtsFunc)gts_point_transform, m);
}

void GTSGeometry::subtract(GTSGeometry &subtrahend)
{
	GtsSurfaceInter *surfaceInter = gts_surface_inter_new(gts_surface_inter_class(),
															_s,									// Surfaces
															subtrahend.getCSGGeometry(),
															gts_bb_tree_surface(_s),			// face bounding boxes
															gts_bb_tree_surface(subtrahend.getCSGGeometry()),
															!gts_surface_is_closed(_s),			// bool is_open
															!gts_surface_is_closed(subtrahend.getCSGGeometry()));

	// The surface that will hold the result
	GtsSurface *result = gts_surface_new(gts_surface_class(), gts_face_class(), gts_edge_class(), gts_vertex_class());
	// For the boolean operator enum, see
	// http://gts.sourceforge.net/reference/gts-boolean-operations.html#GTSBOOLEANOPERATION
	gts_surface_inter_boolean(surfaceInter, result, GTS_1_OUT_2);
	free(_s);
	_s = result;
	VRGeometry::setMesh((GeometryRecPtr)toOsgGeometry());
}

void GTSGeometry::unify(GTSGeometry &addend)
{
	cout << "GTSG::translate: NYI!\n";
}

void GTSGeometry::intersect(GTSGeometry &other)
{
	cout << "GTSG::translate: NYI!\n";
}

// Must be declared as C function to be usable as a GtsFunc in GTSGeometry::toOsgGeometry()
gint foreachFaceFunc(gpointer item, gpointer data)
{
	GtsFace *f = (GtsFace*)item;
	// Retrieve the stuff passed over in data
	// This looks inefficient. Having these pointers globally available would be ugly as well.
	// Could alternatively iterate over vertices instead of faces. Does that mess up index counting?
	vector<void*> *funcData = (vector<void*>*)data;
	GeoPnt3fPropertyRecPtr positions = (GeoPnt3fProperty*)(*funcData)[0];
	GeoVec3fPropertyRecPtr normals = (GeoVec3fProperty*)(*funcData)[1];
	GeoUInt32PropertyRecPtr indices = (GeoUInt32Property*)(*funcData)[2];
	int *curIndex = (int*)(*funcData)[3];

	// Do the actual work
	GtsPoint gtsPos1 = f->triangle.e1->segment.v1->p;
	GtsPoint gtsPos2 = f->triangle.e1->segment.v2->p;
	// We have 2 of 3 vertices.
	// Now we want the third vertex. If edge2.vertex1 is the same as one of the vertices of edge1,
	// we want edge2.vertex2. If not, edge2.vertex1.
	GtsPoint gtsPos3;
	if(f->triangle.e2->segment.v1 == f->triangle.e1->segment.v1
			|| f->triangle.e2->segment.v1 == f->triangle.e1->segment.v2)
		gtsPos3 = f->triangle.e2->segment.v2->p;
	else
		gtsPos3 = f->triangle.e2->segment.v1->p;

	// Convert data to OSG format
	OSG::Pnt3f osgPos(gtsPos1.x, gtsPos1.y, gtsPos1.z);
	positions->addValue(osgPos);
	normals->addValue(Vec3f(0,1,0));
	indices->addValue(*curIndex);
//	cout << "Add: " << osgPos << " Ind: " << *curIndex << endl;
	(*curIndex)++;

	osgPos = OSG::Pnt3f(gtsPos2.x, gtsPos2.y, gtsPos2.z);
	positions->addValue(osgPos);
	normals->addValue(Vec3f(0,1,0));
	indices->addValue(*curIndex);
//	cout << "Add: " << osgPos << " Ind: " << *curIndex << endl;
	(*curIndex)++;

	osgPos = OSG::Pnt3f(gtsPos3.x, gtsPos3.y, gtsPos3.z);
	positions->addValue(osgPos);
	normals->addValue(Vec3f(0,1,0));
	indices->addValue(*curIndex);
//	cout << "Add: " << osgPos << " Ind: " << *curIndex << endl;
	(*curIndex)++;
	return 0; // "Returns: if 0 the calling sequence continues, otherwise it stops." (GTS docs)
}

GeometryTransitPtr GTSGeometry::toOsgGeometry()
{
	GeoPnt3fPropertyRecPtr positions = GeoPnt3fProperty::create();
	GeoVec3fPropertyRecPtr normals = GeoVec3fProperty::create();
	GeoUInt32PropertyRecPtr indices = GeoUInt32Property::create();
	int curIndex = 0;

	// Prepare the data package and process the faces
	vector<void*> funcData;
	funcData.push_back(positions);
	funcData.push_back(normals);
	funcData.push_back(indices);
	funcData.push_back(&curIndex);
	gts_surface_foreach_face(_s, &foreachFaceFunc, &funcData);

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
	mesh->setMaterial(getDefaultMaterial());
	calcVertexNormals(mesh);
	return GeometryTransitPtr(mesh);
}

} /* namespace CSGApp */
} /* namespace OSG */
