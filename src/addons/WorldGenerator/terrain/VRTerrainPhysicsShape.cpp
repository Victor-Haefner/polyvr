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

    initialize(dim[0], dim[1], 1, -Hmax, Hmax);
    auto texelSize = terrain->getTexelSize();
    setLocalScaling(btVector3(texelSize[0],1,texelSize[1]));
}

VRTerrainPhysicsShape::~VRTerrainPhysicsShape() {;}

void VRTerrainPhysicsShape::initialize (int heightStickWidth, int heightStickLength, btScalar heightScale, btScalar minHeight, btScalar maxHeight) {
	btAssert(heightStickWidth > 1 && "bad width");
	btAssert(heightStickLength > 1 && "bad length");
	btAssert(heightfieldData && "null heightfield data");

	// initialize member variables
	m_shapeType = TERRAIN_SHAPE_PROXYTYPE;
	m_heightStickWidth = heightStickWidth;
	m_heightStickLength = heightStickLength;
	m_minHeight = minHeight;
	m_maxHeight = maxHeight;
	m_width = (btScalar) (heightStickWidth - 1);
	m_length = (btScalar) (heightStickLength - 1);
	m_heightScale = heightScale;
	m_localScaling.setValue(btScalar(1.), btScalar(1.), btScalar(1.));
    //m_localAabbMin.setValue(0, m_minHeight, 0);
    //m_localAabbMax.setValue(m_width, m_maxHeight, m_length);
    auto bb = terrain->getBoundingBox();
    m_localAabbMin = VRPhysics::toBtVector3( bb.min() );
    m_localAabbMax = VRPhysics::toBtVector3( bb.max() );
	m_localOrigin = btVector3(0,0,0);
}

void VRTerrainPhysicsShape::getAabb(const btTransform& t,btVector3& aabbMin,btVector3& aabbMax) const {
	btVector3 halfExtents = (m_localAabbMax-m_localAabbMin)* m_localScaling * btScalar(0.5);
	btVector3 localOrigin(0, 0, 0);
	localOrigin[1] = (m_minHeight + m_maxHeight) * btScalar(0.5);
	localOrigin *= m_localScaling;

	btMatrix3x3 abs_b = t.getBasis().absolute();
	btVector3 center = t.getOrigin();
    btVector3 extent = halfExtents.dot3(abs_b[0], abs_b[1], abs_b[2]);
	extent += btVector3(getMargin(),getMargin(),getMargin());

	aabbMin = center - extent;
	aabbMax = center + extent;
}

btScalar VRTerrainPhysicsShape::getRawHeightFieldValue(int x,int y) const {
    return terrain->getMap()->getPixel(Vec3i(x,y,0))[3] + 0.03;
}

/// this returns the vertex in bullet-local coordinates
void VRTerrainPhysicsShape::getVertex(int x,int y,btVector3& vertex) const {
	btAssert(x>=0);
	btAssert(y>=0);
	btAssert(x<m_heightStickWidth);
	btAssert(y<m_heightStickLength);
	btScalar height = getRawHeightFieldValue(x,y);
    vertex.setValue( (-m_width/btScalar(2.0)) + x, height - m_localOrigin.getY(), (-m_length/btScalar(2.0)) + y );
	vertex *= m_localScaling;
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
void VRTerrainPhysicsShape::quantizeWithClamp(int* out, const btVector3& point,int /*isMax*/) const {
	btVector3 clampedPoint(point);
	clampedPoint.setMax(m_localAabbMin);
	clampedPoint.setMin(m_localAabbMax);

	out[0] = getQuantized(clampedPoint.getX());
	out[1] = getQuantized(clampedPoint.getY());
	out[2] = getQuantized(clampedPoint.getZ());
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
	btVector3	localAabbMin = aabbMin*btVector3(1.f/m_localScaling[0],1.f/m_localScaling[1],1.f/m_localScaling[2]);
	btVector3	localAabbMax = aabbMax*btVector3(1.f/m_localScaling[0],1.f/m_localScaling[1],1.f/m_localScaling[2]);

	// account for local origin
	localAabbMin += m_localOrigin;
	localAabbMax += m_localOrigin;

	//quantize the aabbMin and aabbMax, and adjust the start/end ranges
	int	quantizedAabbMin[3];
	int	quantizedAabbMax[3];
	quantizeWithClamp(quantizedAabbMin, localAabbMin,0);
	quantizeWithClamp(quantizedAabbMax, localAabbMax,1);

	// expand the min/max quantized values
	// this is to catch the case where the input aabb falls between grid points!
	for (int i = 0; i < 3; ++i) {
		quantizedAabbMin[i]--;
		quantizedAabbMax[i]++;
	}

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
            getVertex(x,j,vertices[0]);
            getVertex(x,j+1,vertices[1]);
            getVertex(x+1,j,vertices[2]);
            callback->processTriangle(vertices,x,j);
            //second triangle
            getVertex(x+1,j,vertices[0]);
            //getVertex(x,j+1,vertices[1]);
            getVertex(x+1,j+1,vertices[2]);
            callback->processTriangle(vertices,x,j);
		}
	}
}

void VRTerrainPhysicsShape::calculateLocalInertia(btScalar ,btVector3& inertia) const {
    inertia.setValue(btScalar(0.),btScalar(0.),btScalar(0.)); //moving concave objects not supported
}

void VRTerrainPhysicsShape::setLocalScaling(const btVector3& scaling) { m_localScaling = scaling; }
const btVector3& VRTerrainPhysicsShape::getLocalScaling() const { return m_localScaling; }
