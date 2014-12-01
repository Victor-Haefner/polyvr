/*
 * GTSGeometry.h
 *
 *  Created on: Feb 17, 2014
 *      Author: marcel
 */

#ifndef GTSGEOMETRY_H_
#define GTSGEOMETRY_H_

#include <string>
#include <gts.h>
#include "OpenSG/OSGConfig.h"
#include "objects/VRGeometry.h"

OSG_BEGIN_NAMESPACE

namespace CSGApp
{

class GTSGeometry : public VRGeometry
{
public:
	static GTSGeometry *createCube(const std::string name, const double size = 1);

	GTSGeometry(std::string name, GtsSurface *s);
	virtual ~GTSGeometry();
	void setCSGGeometry(GtsSurface *s);
	GtsSurface *getCSGGeometry();
	void translate(OSG::Vec3f v);
	void subtract(GTSGeometry &subtrahend);
	void unify(GTSGeometry &addend);
	void intersect(GTSGeometry &other);

private:
	GeometryTransitPtr toOsgGeometry();
	GtsSurface *_s;
};

} /* namespace CSGApp */

OSG_END_NAMESPACE

#endif /* GTSGEOMETRY_H_ */
