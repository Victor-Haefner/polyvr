#ifndef VRMESHSUBDIVISION_H_INCLUDED
#define VRMESHSUBDIVISION_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>

#include "VRAlgorithmsFwd.h"
#include "core/objects/VRObjectFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRMeshSubdivision : public std::enable_shared_from_this<VRMeshSubdivision> {
	private:

        bool checkOrder(Pnt3d p0, Pnt3d p1, Pnt3d p2, Vec3d n);
        void pushTri(VRGeoData& g, Pnt3d p1, Pnt3d p2, Pnt3d p3, Vec3d n);
        void pushQuad(VRGeoData& g, Pnt3d p1, Pnt3d p2, Pnt3d p3, Pnt3d p4, Vec3d n);
        void pushPen(VRGeoData& g, Pnt3d p1, Pnt3d p2, Pnt3d p3, Pnt3d p4, Pnt3d p5, Vec3d n);

	public:
		VRMeshSubdivision();
		~VRMeshSubdivision();

		static VRMeshSubdivisionPtr create();
		VRMeshSubdivisionPtr ptr();

        void subdivideTriangles(VRGeometryPtr geo, Vec3d res);
        void subdivideGrid(VRGeometryPtr geo, Vec3d res);

        void segmentTriangle(VRGeoData& geo, Vec3i pSegments, vector<Pnt3f> points, Vec3d n, vector<Vec2d> segments);
};

OSG_END_NAMESPACE;

#endif //VRMESHSUBDIVISION_H_INCLUDED
