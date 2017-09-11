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
    float roadTerrainOffset = 0.03; // also defined in vrroadbase.cpp

    double Hmax = -1e6;
    physicsHeightBuffer = shared_ptr<vector<float>>( new vector<float>(dim[0]*dim[1]) );
    for (int i = 0; i < dim[0]; i++) {
        for (int j = 0; j < dim[1]; j++) {
            int k = j*dim[0]+i;
            float h = tex->getPixel(Vec3i(i,j,0))[3];
            (*physicsHeightBuffer)[k] = h + roadTerrainOffset;
            if (Hmax < h) Hmax = h;
        }
    }

    initialize(dim[0], dim[1], &(*physicsHeightBuffer)[0], 1, -Hmax, Hmax, 1, PHY_FLOAT, false);
    auto texelSize = terrain->getTexelSize();
    setLocalScaling(btVector3(texelSize[0],1,texelSize[1]));
}

VRTerrainPhysicsShape::~VRTerrainPhysicsShape() {;}

void VRTerrainPhysicsShape::initialize (
int heightStickWidth, int heightStickLength, const void* heightfieldData,
btScalar heightScale, btScalar minHeight, btScalar maxHeight, int upAxis,
PHY_ScalarType hdt, bool flipQuadEdges
) {
	// validation
	btAssert(heightStickWidth > 1 && "bad width");
	btAssert(heightStickLength > 1 && "bad length");
	btAssert(heightfieldData && "null heightfield data");
	// btAssert(heightScale) -- do we care?  Trust caller here
	btAssert(minHeight <= maxHeight && "bad min/max height");
	btAssert(upAxis >= 0 && upAxis < 3 &&
	    "bad upAxis--should be in range [0,2]");
	btAssert(hdt != PHY_UCHAR || hdt != PHY_FLOAT || hdt != PHY_SHORT &&
	    "Bad height data type enum");

	// initialize member variables
	m_shapeType = TERRAIN_SHAPE_PROXYTYPE;
	m_heightStickWidth = heightStickWidth;
	m_heightStickLength = heightStickLength;
	m_minHeight = minHeight;
	m_maxHeight = maxHeight;
	m_width = (btScalar) (heightStickWidth - 1);
	m_length = (btScalar) (heightStickLength - 1);
	m_heightScale = heightScale;
	m_heightfieldDataUnknown = heightfieldData;
	m_heightDataType = hdt;
	m_flipQuadEdges = flipQuadEdges;
	m_useDiamondSubdivision = false;
	m_useZigzagSubdivision = false;
	m_upAxis = upAxis;
	m_localScaling.setValue(btScalar(1.), btScalar(1.), btScalar(1.));

	// determine min/max axis-aligned bounding box (aabb) values
	switch (m_upAxis)
	{
	case 0:
		{
			m_localAabbMin.setValue(m_minHeight, 0, 0);
			m_localAabbMax.setValue(m_maxHeight, m_width, m_length);
			break;
		}
	case 1:
		{
			m_localAabbMin.setValue(0, m_minHeight, 0);
			m_localAabbMax.setValue(m_width, m_maxHeight, m_length);
			break;
		};
	case 2:
		{
			m_localAabbMin.setValue(0, 0, m_minHeight);
			m_localAabbMax.setValue(m_width, m_length, m_maxHeight);
			break;
		}
	default:
		{
			//need to get valid m_upAxis
			btAssert(0 && "Bad m_upAxis");
		}
	}

	// remember origin (defined as exact middle of aabb)
	m_localOrigin = btScalar(0.5) * (m_localAabbMin + m_localAabbMax);
}

void VRTerrainPhysicsShape::getAabb(const btTransform& t,btVector3& aabbMin,btVector3& aabbMax) const {
	btVector3 halfExtents = (m_localAabbMax-m_localAabbMin)* m_localScaling * btScalar(0.5);

	btVector3 localOrigin(0, 0, 0);
	localOrigin[m_upAxis] = (m_minHeight + m_maxHeight) * btScalar(0.5);
	localOrigin *= m_localScaling;

	btMatrix3x3 abs_b = t.getBasis().absolute();
	btVector3 center = t.getOrigin();
    btVector3 extent = halfExtents.dot3(abs_b[0], abs_b[1], abs_b[2]);
	extent += btVector3(getMargin(),getMargin(),getMargin());

	aabbMin = center - extent;
	aabbMax = center + extent;
}

btScalar VRTerrainPhysicsShape::getRawHeightFieldValue(int x,int y) const {
	btScalar val = 0.f;
	switch (m_heightDataType)
	{
	case PHY_FLOAT:
		{
			val = m_heightfieldDataFloat[(y*m_heightStickWidth)+x];
			break;
		}

	case PHY_UCHAR:
		{
			unsigned char heightFieldValue = m_heightfieldDataUnsignedChar[(y*m_heightStickWidth)+x];
			val = heightFieldValue * m_heightScale;
			break;
		}

	case PHY_SHORT:
		{
			short hfValue = m_heightfieldDataShort[(y * m_heightStickWidth) + x];
			val = hfValue * m_heightScale;
			break;
		}

	default:
		{
			btAssert(!"Bad m_heightDataType");
		}
	}

	return val;
}




/// this returns the vertex in bullet-local coordinates
void	VRTerrainPhysicsShape::getVertex(int x,int y,btVector3& vertex) const
{
	btAssert(x>=0);
	btAssert(y>=0);
	btAssert(x<m_heightStickWidth);
	btAssert(y<m_heightStickLength);

	btScalar	height = getRawHeightFieldValue(x,y);

	switch (m_upAxis)
	{
	case 0:
		{
		vertex.setValue(
			height - m_localOrigin.getX(),
			(-m_width/btScalar(2.0)) + x,
			(-m_length/btScalar(2.0) ) + y
			);
			break;
		}
	case 1:
		{
			vertex.setValue(
			(-m_width/btScalar(2.0)) + x,
			height - m_localOrigin.getY(),
			(-m_length/btScalar(2.0)) + y
			);
			break;
		};
	case 2:
		{
			vertex.setValue(
			(-m_width/btScalar(2.0)) + x,
			(-m_length/btScalar(2.0)) + y,
			height - m_localOrigin.getZ()
			);
			break;
		}
	default:
		{
			//need to get valid m_upAxis
			btAssert(0);
		}
	}

	vertex*=m_localScaling;
}



static inline int
getQuantized
(
btScalar x
)
{
	if (x < 0.0) {
		return (int) (x - 0.5);
	}
	return (int) (x + 0.5);
}



/// given input vector, return quantized version
/**
  This routine is basically determining the gridpoint indices for a given
  input vector, answering the question: "which gridpoint is closest to the
  provided point?".
  "with clamp" means that we restrict the point to be in the heightfield's
  axis-aligned bounding box.
 */
void VRTerrainPhysicsShape::quantizeWithClamp(int* out, const btVector3& point,int /*isMax*/) const
{
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
void	VRTerrainPhysicsShape::processAllTriangles(btTriangleCallback* callback,const btVector3& aabbMin,const btVector3& aabbMax) const
{
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

	switch (m_upAxis)
	{
	case 0:
		{
			if (quantizedAabbMin[1]>startX)
				startX = quantizedAabbMin[1];
			if (quantizedAabbMax[1]<endX)
				endX = quantizedAabbMax[1];
			if (quantizedAabbMin[2]>startJ)
				startJ = quantizedAabbMin[2];
			if (quantizedAabbMax[2]<endJ)
				endJ = quantizedAabbMax[2];
			break;
		}
	case 1:
		{
			if (quantizedAabbMin[0]>startX)
				startX = quantizedAabbMin[0];
			if (quantizedAabbMax[0]<endX)
				endX = quantizedAabbMax[0];
			if (quantizedAabbMin[2]>startJ)
				startJ = quantizedAabbMin[2];
			if (quantizedAabbMax[2]<endJ)
				endJ = quantizedAabbMax[2];
			break;
		};
	case 2:
		{
			if (quantizedAabbMin[0]>startX)
				startX = quantizedAabbMin[0];
			if (quantizedAabbMax[0]<endX)
				endX = quantizedAabbMax[0];
			if (quantizedAabbMin[1]>startJ)
				startJ = quantizedAabbMin[1];
			if (quantizedAabbMax[1]<endJ)
				endJ = quantizedAabbMax[1];
			break;
		}
	default:
		{
			//need to get valid m_upAxis
			btAssert(0);
		}
	}




	for(int j=startJ; j<endJ; j++)
	{
		for(int x=startX; x<endX; x++)
		{
			btVector3 vertices[3];
			if (m_flipQuadEdges || (m_useDiamondSubdivision && !((j+x) & 1))|| (m_useZigzagSubdivision  && !(j & 1)))
			{
        //first triangle
        getVertex(x,j,vertices[0]);
        getVertex(x+1,j,vertices[1]);
        getVertex(x+1,j+1,vertices[2]);
        callback->processTriangle(vertices,x,j);
        //second triangle
      //  getVertex(x,j,vertices[0]);//already got this vertex before, thanks to Danny Chapman
        getVertex(x+1,j+1,vertices[1]);
        getVertex(x,j+1,vertices[2]);
        callback->processTriangle(vertices,x,j);
			} else
			{
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



}

void	VRTerrainPhysicsShape::calculateLocalInertia(btScalar ,btVector3& inertia) const
{
	//moving concave objects not supported

	inertia.setValue(btScalar(0.),btScalar(0.),btScalar(0.));
}

void	VRTerrainPhysicsShape::setLocalScaling(const btVector3& scaling)
{
	m_localScaling = scaling;
}
const btVector3& VRTerrainPhysicsShape::getLocalScaling() const
{
	return m_localScaling;
}
