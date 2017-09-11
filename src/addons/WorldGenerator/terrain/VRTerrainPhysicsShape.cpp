#include "VRTerrainPhysicsShape.h"
#include "VRTerrain.h"
#include "core/objects/geometry/VRPhysics.h"
#include "core/math/pose.h"

using namespace OSG;

VRTerrainPhysicsShape::VRTerrainPhysicsShape(VRTerrainPtr terrain) : terrain(terrain) {;}
VRTerrainPhysicsShape::~VRTerrainPhysicsShape() {;}

void VRTerrainPhysicsShape::getAabb(const btTransform& t, btVector3& aabbMin, btVector3& aabbMax) const {
    auto p = terrain->getWorldPose();
    auto bb = terrain->getBoundingBox();
    aabbMax = VRPhysics::toBtVector3( p->transform( bb.min() ) );
    aabbMin = VRPhysics::toBtVector3( p->transform( bb.max() ) );
}

void VRTerrainPhysicsShape::calculateLocalInertia(btScalar mass, btVector3& inertia) const {
    inertia.setValue(0,0,0); // terrain not dynamic!
}
