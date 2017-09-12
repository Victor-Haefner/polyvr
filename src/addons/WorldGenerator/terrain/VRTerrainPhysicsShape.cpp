#include "VRTerrainPhysicsShape.h"
#include "VRTerrain.h"
#include "core/objects/geometry/VRPhysics.h"
#include "core/math/pose.h"
#include "core/objects/material/VRTexture.h"

#include <bullet/LinearMath/btTransformUtil.h>

using namespace OSG;

VRTerrainPhysicsShape::VRTerrainPhysicsShape(VRTerrainPtr terrain) : terrain(terrain) {
    auto tex = terrain->getMap();
    auto dim = tex->getSize();

    double Hmax = -1e6;
    for (int i = 0; i < dim[0]; i++) {
        for (int j = 0; j < dim[1]; j++) {
            float h = tex->getPixel(Vec3i(i,j,0))[3];
            if (Hmax < h) Hmax = h;
        }
    }

	m_shapeType = TERRAIN_SHAPE_PROXYTYPE;
	m_heightStickWidth = dim[0];
	m_heightStickLength = dim[1];
	m_width = (btScalar) (dim[0] - 1);
	m_length = (btScalar) (dim[1] - 1);
    m_localAabbMin.setValue(0, -Hmax, 0);
    m_localAabbMax.setValue(m_width, Hmax, m_length);
    /*auto bb = terrain->getBoundingBox();
    m_localAabbMin = VRPhysics::toBtVector3( bb.min() );
    m_localAabbMax = VRPhysics::toBtVector3( bb.max() );
	m_localOrigin = btVector3(0,0,0);*/
	m_localOrigin = btScalar(0.5) * (m_localAabbMin + m_localAabbMax);

    auto texelSize = terrain->getTexelSize();
	m_localScaling.setValue(texelSize[0],1,texelSize[1]);
}

VRTerrainPhysicsShape::~VRTerrainPhysicsShape() {;}

void VRTerrainPhysicsShape::getAabb(const btTransform& t, btVector3& aabbMin, btVector3& aabbMax) const {
	btVector3 halfExtents = (m_localAabbMax-m_localAabbMin)* m_localScaling * btScalar(0.5);
	btVector3 localOrigin(0, 0, 0);
	btMatrix3x3 abs_b = t.getBasis().absolute();
	btVector3 center = t.getOrigin();
    btVector3 extent = halfExtents.dot3(abs_b[0], abs_b[1], abs_b[2]);
	extent += btVector3(getMargin(),getMargin(),getMargin());

	aabbMin = center - extent;
	aabbMax = center + extent;
	//cout << "VRTerrainPhysicsShape::getAabb " << VRPhysics::toVec3d(halfExtents) << " " << VRPhysics::toVec3d(halfExtents) << endl;
}

/// this returns the vertex in bullet-local coordinates
btVector3 VRTerrainPhysicsShape::getVertex(int x, int y) const {
	btScalar height = terrain->getMap()->getPixel(Vec3i(x,y,0))[3] + 0.03;
	btVector3 vertex;
    vertex.setValue( -m_width*0.5 + x, height - m_localOrigin[1], -m_length*0.5 + y );
	vertex *= m_localScaling;
	return vertex;
}

static inline int getQuantized ( btScalar x ) {
	return (int) x < 0.0 ? x - 0.5 : x + 0.5;
}

/// given input vector, return quantized version
/**
  This routine is basically determining the gridpoint indices for a given
  input vector, answering the question: "which gridpoint is closest to the
  provided point?".
  "with clamp" means that we restrict the point to be in the heightfield's
  axis-aligned bounding box.
 */
Vec3i VRTerrainPhysicsShape::quantizeWithClamp(const btVector3& point) const {
	btVector3 clampedPoint(point);
	clampedPoint.setMax(m_localAabbMin);
	clampedPoint.setMin(m_localAabbMax);

	Vec3i out;
	out[0] = getQuantized(clampedPoint.getX());
	out[1] = getQuantized(clampedPoint.getY());
	out[2] = getQuantized(clampedPoint.getZ());
	return out;
}

/// process all triangles within the provided axis-aligned bounding box
/**
  basic algorithm:
    - convert input aabb to local coordinates (scale down and shift for local origin)
    - convert input aabb to a range of heightfield grid points (quantize)
    - iterate over all triangles in that subset of the grid
 */
void VRTerrainPhysicsShape::processAllTriangles(btTriangleCallback* callback,const btVector3& aabbMin,const btVector3& aabbMax) const {
	// scale down the input aabb's so they are in local (non-scaled) coordinates
	btVector3 localAabbMin = aabbMin*btVector3(1.f/m_localScaling[0],1.f/m_localScaling[1],1.f/m_localScaling[2]);
	btVector3 localAabbMax = aabbMax*btVector3(1.f/m_localScaling[0],1.f/m_localScaling[1],1.f/m_localScaling[2]);

	// account for local origin
	localAabbMin += m_localOrigin;
	localAabbMax += m_localOrigin;

	//quantize the aabbMin and aabbMax, and adjust the start/end ranges
	Vec3i quantizedAabbMin = quantizeWithClamp(localAabbMin) - Vec3i(1,1,1);
	Vec3i quantizedAabbMax = quantizeWithClamp(localAabbMax) + Vec3i(1,1,1);

	int startX=0;
	int endX=m_heightStickWidth-1;
	int startJ=0;
	int endJ=m_heightStickLength-1;

    if (quantizedAabbMin[0]>startX) startX = quantizedAabbMin[0];
    if (quantizedAabbMax[0]<endX) endX = quantizedAabbMax[0];
    if (quantizedAabbMin[2]>startJ) startJ = quantizedAabbMin[2];
    if (quantizedAabbMax[2]<endJ) endJ = quantizedAabbMax[2];

	for(int j=startJ; j<endJ; j++) {
		for(int x=startX; x<endX; x++) {
			btVector3 vertices[3];
            //first triangle
            vertices[0] = getVertex(x,j);
            vertices[1] = getVertex(x,j+1);
            vertices[2] = getVertex(x+1,j);
            callback->processTriangle(vertices,x,j);
            //second triangle
            vertices[0] = getVertex(x+1,j);
            vertices[2] = getVertex(x+1,j+1);
            callback->processTriangle(vertices,x,j);
		}
	}
}

void VRTerrainPhysicsShape::calculateLocalInertia(btScalar, btVector3& inertia) const { inertia.setValue(0,0,0); } //moving concave objects not supported
void VRTerrainPhysicsShape::setLocalScaling(const btVector3& scaling) {}
const btVector3& VRTerrainPhysicsShape::getLocalScaling() const { return btVector3(1,1,1); }




