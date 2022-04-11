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
        bool checkOrder(VRGeoData& g, size_t p0, size_t p1, size_t p2, Vec3d n);
        void pushTri(VRGeoData& g, size_t p1, size_t p2, size_t p3, Vec3d n);
        void pushQuad(VRGeoData& g, size_t p1, size_t p2, size_t p3, size_t p4, Vec3d n);

        void removeDoubles(VRGeometryPtr g);
        void gridMergeTriangles(VRGeometryPtr g, Vec3d g0, Vec3d res, int dim, int dim2);

	public:
		VRMeshSubdivision();
		~VRMeshSubdivision();

		static VRMeshSubdivisionPtr create();
		VRMeshSubdivisionPtr ptr();

        void subdivideTriangles(VRGeometryPtr geo, Vec3d res);
        void subdivideGrid(VRGeometryPtr geo, Vec3d res);

        void segmentTriangle(VRGeoData& geo, Vec3i pSegments, vector<Pnt3f> points, Vec3d n, vector<Vec2d> segments, int dim = 0, int dim2 = 2);
};

OSG_END_NAMESPACE;

#endif //VRMESHSUBDIVISION_H_INCLUDED
