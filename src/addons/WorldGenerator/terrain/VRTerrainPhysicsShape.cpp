#include "VRTerrainPhysicsShape.h"
#include "VRTerrain.h"
#include "core/objects/geometry/VRPhysics.h"
#include "core/math/pose.h"
#include "core/utils/VRTimer.h"
#include "core/objects/material/VRTexture.h"

#include <LinearMath/btTransformUtil.h>

using namespace OSG;

VRTerrainPhysicsShape::VRTerrainPhysicsShape(VRTerrainPtr terrain, float resolution) : btConcaveShape(), terrain(terrain), resolution(resolution) {
	m_shapeType = CUSTOM_CONCAVE_SHAPE_TYPE;
    auto tex = terrain->getMap();
    texelSize = terrain->getTexelSize();
	texSize = Vec2i( tex->getSize()[0], tex->getSize()[1] );
	size = Vec2d( (texSize[0]-1) * texelSize[0], (texSize[1]-1) * texelSize[1] );
    boundingbox = terrain->getBoundingBox();
}

VRTerrainPhysicsShape::~VRTerrainPhysicsShape() {;}
void VRTerrainPhysicsShape::calculateLocalInertia(btScalar, btVector3& inertia) const { inertia.setValue(0,0,0); } //moving concave objects not supported
void VRTerrainPhysicsShape::setLocalScaling(const btVector3& scaling) { scale = scaling; }
const btVector3& VRTerrainPhysicsShape::getLocalScaling() const { return scale; }

void VRTerrainPhysicsShape::getAabb(const btTransform& t, btVector3& Min, btVector3& Max) const {
    // TODO: use t!
    Min = VRPhysics::toBtVector3( boundingbox.min() );
	Max = VRPhysics::toBtVector3( boundingbox.max() );
}

void VRTerrainPhysicsShape::processAllTriangles(btTriangleCallback* callback, const btVector3& aabbMin, const btVector3& aabbMax) const {
    Vec3d Min = VRPhysics::toVec3d(aabbMin);
    Vec3d Max = VRPhysics::toVec3d(aabbMax);
    boundingbox.clamp( Min );
    boundingbox.clamp( Max );

    Vec2d res;
    res[0] = resolution > 0 ? resolution : texelSize[0];
    res[1] = resolution > 0 ? resolution : texelSize[1];

    auto toSpace = [&](int X, int Y) {
        if (cache.count(X)) if (cache[X].count(Y)) return cache[X][Y];
        if (cache.size() > 50) cache.clear();
        double x = (double(X)-0.5)*res[0];
        double y = (double(Y)-0.5)*res[1];
        auto b = btVector3( x, terrain->getHeight(Vec2d(x,y), false), y );
        cache[X][Y] = b;
        return b;
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

	//auto D = timer.stop();
	//if (D > 3) cout << "   TTT " << D << endl;
}

map<int, map<int, btVector3>> VRTerrainPhysicsShape::cache;




