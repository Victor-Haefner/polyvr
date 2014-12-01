/*
 * CsgjsWrapper.cpp
 *
 *  Created on: Dec 17, 2013
 *      Author: marcel
 */

#include "CsgjsWrapper.h"

using namespace OSG;
using namespace CSGApp;

CsgjsWrapper::CsgjsWrapper(csgjs_model model) :
		_model(model)
{
}

CsgjsWrapper::~CsgjsWrapper()
{
}

CsgjsWrapper *CsgjsWrapper::createSphere(float radius, int slices, int stacks)
{
	return new CsgjsWrapper(csgjs_sphere(radius, slices, stacks));
}

CsgjsWrapper *CsgjsWrapper::createBox(int size)
{
	return new CsgjsWrapper(csgjs_cube(size));
}

void CsgjsWrapper::setModel(csgjs_model model)
{
	_model = model;
}

csgjs_model *CsgjsWrapper::model()
{
	return &_model;
}

GeometryTransitPtr CsgjsWrapper::toOsgGeometry()
{
	GeoPnt3fPropertyRecPtr positions = GeoPnt3fProperty::create();
	GeoVec3fPropertyRecPtr normals = GeoVec3fProperty::create();
	GeoUInt32PropertyRecPtr indices = GeoUInt32Property::create();

	// Convert vertices/positions, normals
	for(size_t i = 0; i < _model.vertices.size(); i++)
	{
		Pnt3f position(_model.vertices[i].pos.x, _model.vertices[i].pos.y, _model.vertices[i].pos.z);
		positions->addValue(position);
		Vec3f normal(_model.vertices[i].normal.x, _model.vertices[i].normal.y, _model.vertices[i].normal.z);
		normals->addValue(normal);
	}

	// Copy the indices over
	for(size_t i = 0; i < _model.indices.size(); i++)
	{
		indices->addValue((UInt32)_model.indices[i]);
	}

	GeoUInt8PropertyRecPtr types = GeoUInt8Property::create();
	types->addValue(GL_TRIANGLES);
	GeoUInt32PropertyRecPtr lengths = GeoUInt32Property::create();
	lengths->addValue(_model.indices.size());

	GeometryRecPtr mesh = Geometry::create();
	mesh->setPositions(positions);
	mesh->setNormals(normals);
	mesh->setIndices(indices);
	mesh->setTypes(types);
	mesh->setLengths(lengths);
	mesh->setMaterial(getDefaultMaterial());
	return GeometryTransitPtr(mesh);
}
