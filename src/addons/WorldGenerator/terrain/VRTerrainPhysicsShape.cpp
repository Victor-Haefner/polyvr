#include "VRTerrainPhysicsShape.h"
#include "VRTerrain.h"
#include "core/objects/geometry/VRPhysics.h"
#include "core/math/pose.h"
#include "core/objects/material/VRTexture.h"

#include <bullet/LinearMath/btTransformUtil.h>

using namespace OSG;

VRTerrainPhysicsShape::VRTerrainPhysicsShape(VRTerrainPtr terrain, float resolution) : terrain(terrain), resolution(resolution) {
	m_shapeType = TERRAIN_SHAPE_PROXYTYPE;
    auto tex = terrain->getMap();
    texelSize = terrain->getTexelSize();
	texSize = Vec2i( tex->getSize()[0], tex->getSize()[1] );
	size = Vec2d( (texSize[0]-1) * texelSize[0], (texSize[1]-1) * texelSize[1] );
    boundingbox = terrain->getBoundingBox();
}

VRTerrainPhysicsShape::~VRTerrainPhysicsShape() {;}
void VRTerrainPhysicsShape::calculateLocalInertia(btScalar, btVector3& inertia) const { inertia.setValue(0,0,0); } //moving concave objects not supported
void VRTerrainPhysicsShape::setLocalScaling(const btVector3& scaling) {}
const btVector3& VRTerrainPhysicsShape::getLocalScaling() const { return btVector3(1,1,1); }

void VRTerrainPhysicsShape::getAabb(const btTransform& t, btVector3& Min, btVector3& Max) const {
    // TODO: use t!
    Min = VRPhysics::toBtVector3( boundingbox.min() );
	Max = VRPhysics::toBtVector3( boundingbox.max() );
}

/*Vec2d VRTerrain::toUVSpace(Vec2d p) {
    int W = tex->getSize()[0]-1;
    int H = tex->getSize()[1]-1;
    double u = (p[0]/size[0] + 0.5)*W;
    double v = (p[1]/size[1] + 0.5)*H;
    return Vec2d(u,v);
};

Vec2d VRTerrain::fromUVSpace(Vec2d uv) {
    int W = tex->getSize()[0]-1;
    int H = tex->getSize()[1]-1;
    double x = ((uv[0])/W-0.5)*size[0];
    double z = ((uv[1])/H-0.5)*size[1];
    return Vec2d(x,z);
};*/

void VRTerrainPhysicsShape::processAllTriangles(btTriangleCallback* callback, const btVector3& aabbMin, const btVector3& aabbMax) const {
    Vec3d Min = VRPhysics::toVec3d(aabbMin);
    Vec3d Max = VRPhysics::toVec3d(aabbMax);
    boundingbox.clamp( Min );
    boundingbox.clamp( Max );

    Vec2d res;
    res[0] = resolution > 0 ? resolution : texelSize[0];
    res[1] = resolution > 0 ? resolution : texelSize[1];

    auto toSpace = [&](double X, double Y) {
        double x = (X-0.5)*res[0];
        double y = (Y-0.5)*res[1];
        return btVector3( x, terrain->getHeight(Vec2d(x,y), false), y );
    };

	int x0 = round(Min[0]/res[0]);
	int y0 = round(Min[2]/res[1]);
	int x1 = round(Max[0]/res[0])+1;
	int y1 = round(Max[2]/res[1])+1;

    /*cout << " --- VRTerrainPhysicsShape::processAllTriangles "
        << Min << " " << Max << "  -> "
        << Vec2i(x0*res[0], y0*res[1])
        << " / " << Vec2i(x1*res[0], y1*res[1]) << endl;*/

	for (int y=y0; y<y1; y++) {
		for (int x=x0; x<x1; x++) {
			btVector3 vertices[3];
            vertices[0] = toSpace(x,y);
            vertices[1] = toSpace(x,y+1);
            vertices[2] = toSpace(x+1,y);
            callback->processTriangle(vertices,x,y);
            vertices[0] = toSpace(x+1,y);
            vertices[2] = toSpace(x+1,y+1);
            callback->processTriangle(vertices,x,y);
		}
	}
}





