#ifndef VRMILLINGWORKPIECE_H_INCLUDED
#define VRMILLINGWORKPIECE_H_INCLUDED

#include <string>
#include <OpenSG/OSGConfig.h>
#include "core/objects/geometry/VRGeometry.h"
#include "core/math/Octree.h"

OSG_BEGIN_NAMESPACE;

class VRMillingWorkPiece : public VRGeometry {
    private:
        Octree octree;
        Vec3i gridSize;
        float blockSize = 0.01;

        int lastToolChange = 0;
        pose toolPose;
        VRTransformWeakPtr tool;

        VRUpdatePtr uFkt;
        void update();

    public:
        VRMillingWorkPiece(string name);

        static VRMillingWorkPiecePtr create(string name);
        VRMillingWorkPiecePtr ptr();

        void reset(Vec3i gSize, float bSize = 0.01);
        void setCuttingTool(VRTransformPtr geo);
};

OSG_END_NAMESPACE;

#endif // VRMILLINGWORKPIECE_H_INCLUDED
