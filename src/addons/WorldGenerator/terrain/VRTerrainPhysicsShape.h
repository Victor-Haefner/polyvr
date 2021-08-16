#ifndef VRTERRAINPHYSICSSHAPE_H_INCLUDED
#define VRTERRAINPHYSICSSHAPE_H_INCLUDED

#include "../VRWorldGeneratorFwd.h"
#include "core/math/partitioning/boundingbox.h"
#include <vector>
#include <memory>
#include <OpenSG/OSGConfig.h>
#include "core/math/OSGMathFwd.h"
#include <BulletCollision/CollisionShapes/btConcaveShape.h>

using namespace std;
OSG_BEGIN_NAMESPACE;

ATTRIBUTE_ALIGNED16(class) VRTerrainPhysicsShape : public btConcaveShape {
    protected:
        VRTerrainPtr terrain;
        float resolution = 0;
        Vec2d size;
        Vec2i texSize;
        Vec2f texelSize;
        Boundingbox boundingbox;
        btVector3 scale = btVector3(1,1,1);
        static map<int, map<int, btVector3>> cache;

    public:
        BT_DECLARE_ALIGNED_ALLOCATOR();

        VRTerrainPhysicsShape(VRTerrainPtr terrain, float resolution = 0);
        ~VRTerrainPhysicsShape();

        virtual void getAabb(const btTransform& t,btVector3& aabbMin,btVector3& aabbMax) const override;
        virtual void processAllTriangles(btTriangleCallback* callback,const btVector3& aabbMin,const btVector3& aabbMax) const override;
        virtual void calculateLocalInertia(btScalar mass,btVector3& inertia) const override;
        virtual void setLocalScaling(const btVector3& scaling) override;
        virtual const btVector3& getLocalScaling() const override;
        virtual const char*	getName() const override { return "HEIGHTFIELD"; }
};

OSG_END_NAMESPACE;

#endif // VRTERRAINPHYSICSSHAPE_H_INCLUDED
