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
	public:
		VRMeshSubdivision();
		~VRMeshSubdivision();

		static VRMeshSubdivisionPtr create();
		VRMeshSubdivisionPtr ptr();

        void subdivideTriangles(VRGeometryPtr geo, Vec3d res);
        void subdivideGrid(VRGeometryPtr geo, Vec3d res);
};

OSG_END_NAMESPACE;

#endif //VRMESHSUBDIVISION_H_INCLUDED
