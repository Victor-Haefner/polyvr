#ifndef VRTERRAINPHYSICSSHAPE_H_INCLUDED
#define VRTERRAINPHYSICSSHAPE_H_INCLUDED

#include "../VRWorldGeneratorFwd.h"
#include <OpenSG/OSGConfig.h>
#include <BulletCollision/CollisionShapes/btConcaveShape.h>

using namespace std;
OSG_BEGIN_NAMESPACE;

ATTRIBUTE_ALIGNED16(class)  VRTerrainPhysicsShape : public btCollisionShape {
    private:
        VRTerrainPtr terrain;

        btVector3 localAabbMin;
        btVector3 localAabbMax;

    public:
        VRTerrainPhysicsShape(VRTerrainPtr terrain);
        ~VRTerrainPhysicsShape();

        BT_DECLARE_ALIGNED_ALLOCATOR();

        virtual void getAabb(const btTransform& t,btVector3& aabbMin,btVector3& aabbMax) const;
        virtual void setLocalScaling(const btVector3& scaling) {}
        virtual const btVector3& getLocalScaling() const { return btVector3(0,0,0); }
        virtual void calculateLocalInertia(btScalar mass,btVector3& inertia) const;

        virtual const char*	getName() const { return "POLYVR-TERRAIN-SHAPE"; }
        virtual void setMargin(btScalar m) {}
        virtual btScalar getMargin() const { return 0; }
};

OSG_END_NAMESPACE;

#endif // VRTERRAINPHYSICSSHAPE_H_INCLUDED
