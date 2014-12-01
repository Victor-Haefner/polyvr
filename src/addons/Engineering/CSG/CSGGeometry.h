/*
 * CsgGeometry.h
 *
 *  Created on: Jan 12, 2014
 *      Author: marcel
 */

#ifndef CSGGEOMETRY_H_
#define CSGGEOMETRY_H_

#include <string>
#include "OpenSG/OSGConfig.h"
#include "core/objects/geometry/VRGeometry.h"
#include "CGALTypedefs.h"
#include "Octree/Octree.h"

OSG_BEGIN_NAMESPACE;

namespace CSGApp
{

class CSGGeometry : public VRGeometry
{
public:
	static vector<string> getOperations();

	CSGGeometry(std::string name);
	virtual ~CSGGeometry();
	// Reimplementation of VRTransform::update to do CSG geometry transformation
	//virtual void update();
	bool setEditMode(const bool b);
	bool getEditMode() const;
	void setOperation(const string op);
	string getOperation() const;
	//virtual void addChild(VRObject* child, bool osg = true);

protected:
	inline void applyTransform(CGAL::Polyhedron *p, const Matrix *m)
	{
		CGAL::Transformation t((*m)[0][0], (*m)[1][0], (*m)[2][0], (*m)[3][0],
							   (*m)[0][1], (*m)[1][1], (*m)[2][1], (*m)[3][1],
							   (*m)[0][2], (*m)[1][2], (*m)[2][2], (*m)[3][2],
																   (*m)[3][3]);
		transform(p->points_begin(), p->points_end(), p->points_begin(), t);
	}

	void setCSGGeometry(CGAL::Polyhedron *p);
	CGAL::Polyhedron *getCSGGeometry();
	GeometryTransitPtr toOsgGeometry(const CGAL::Polyhedron *p);
	size_t isKnownPoint(OSG::Pnt3f newPoint);
	CGAL::Polyhedron *toPolyhedron(const GeometryRecPtr geometry, const Matrix *worldTransform);
	CGAL::Polyhedron *subtract(CGAL::Polyhedron *minuend, CGAL::Polyhedron *subtrahend);
	CGAL::Polyhedron *unite(CGAL::Polyhedron *first, CGAL::Polyhedron *second);
	CGAL::Polyhedron *intersect(CGAL::Polyhedron *first, CGAL::Polyhedron *second);
	void enableEditMode();
	bool disableEditMode();
	void saveContent(xmlpp::Element* e);
	void loadContent(xmlpp::Element* e);

private:
	string _operation;
	bool _editMode;
	Matrix _oldWorldTrans;
	const float _THRESHOLD;
	Octree _octree;
};

} /* namespace CSGApp */

OSG_END_NAMESPACE;

#endif /* CSGGEOMETRY_H_ */
