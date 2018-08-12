#ifndef VRSPATIALCOLLISIONMANAGER_H_INCLUDED
#define VRSPATIALCOLLISIONMANAGER_H_INCLUDED

#include "VRGeometry.h"
#include "addons/Bullet/VRPhysicsFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

ptrFctFwd( VRCollision, vector<VRCollision> );

class VRSpatialCollisionManager : public VRGeometry {
    private:
        OctreePtr space;
        VRCollisionCbPtr collisionCb;
        VRUpdateCbPtr updateCollisionCb;

        void checkCollisions();

    public:
        VRSpatialCollisionManager(float resolution);
        ~VRSpatialCollisionManager();

        static VRSpatialCollisionManagerPtr create(float resolution);

        void add(VRObjectPtr o, int objID);
        void addQuad(float width, float height, const Pose& p, int objID);
        void localize(Boundingbox box);

        void setCollisionCallback(VRCollisionCbPtr cb);
};

OSG_END_NAMESPACE;

#endif // VRSPATIALCOLLISIONMANAGER_H_INCLUDED
