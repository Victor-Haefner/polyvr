#ifndef VRSPATIALCOLLISIONMANAGER_H_INCLUDED
#define VRSPATIALCOLLISIONMANAGER_H_INCLUDED

#include "VRGeometry.h"
#include "addons/Bullet/VRPhysicsFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRSpatialCollisionManager : public VRGeometry {
    private:
        OctreePtr space;

    public:
        VRSpatialCollisionManager(float resolution);
        ~VRSpatialCollisionManager();

        static VRSpatialCollisionManagerPtr create(float resolution);

        void add(VRObjectPtr o);
        void localize(Boundingbox box);
};

OSG_END_NAMESPACE;

#endif // VRSPATIALCOLLISIONMANAGER_H_INCLUDED
