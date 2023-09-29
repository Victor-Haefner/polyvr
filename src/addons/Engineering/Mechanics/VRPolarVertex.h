#ifndef VRPOLARVERTEX_H_INCLUDED
#define VRPOLARVERTEX_H_INCLUDED


#include <OpenSG/OSGVector.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

struct PolarCoords {
    Vec3d axis;
    Vec3d dir0;
    Vec3d dir1;

    PolarCoords(Vec3d a, Vec3d p0);
};

struct PolarVertex {
    Vec3d vertex;
	Vec2d polarCoords;
	Vec2d profileCoords;
	int plane = -1;
	double radius = -1;

	PolarVertex();
	PolarVertex(Vec3d v);

	void setPlaneIndex(int i);
	Vec3d getOrthogonal(Vec3d v);

    double computeRadius(PolarCoords& coords);
    Vec2d computePolar(PolarCoords& coords);
	Vec2d computeProfile(PolarCoords& coords);
	void computeAndSetAttributes(PolarCoords& coords);
};

OSG_END_NAMESPACE;

#endif // VRPOLARVERTEX_H_INCLUDED
