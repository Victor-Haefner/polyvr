#ifndef MAPCOORDINATOR_H
#define	MAPCOORDINATOR_H

#include "Altitude.h"
#include "Config.h"
#include "Elevation.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class Elevation;

class MapCoordinator {
    private:
        Vec2f zeroPos;
        float gridSize;
        float SCALE_REAL_TO_WORLD = 111000.0;
        Elevation* ele = 0;
        float startElevation;

    public:
        MapCoordinator(Vec2f zeroPos, float gridSize);

        Vec2f realToWorld(Vec2f realPosition);
        Vec2f worldToReal(Vec2f worldPosition);

        float getGridSize();
        float getElevation(float x, float y);
        float getElevation(Vec2f v);

        Vec2f getRealBboxPosition(Vec2f worldPosition);

        static Vec3f getNormal3D(Vec3f v1, Vec3f v2);
        static Vec3f getPositioveNormal3D(Vec3f v1, Vec3f v2);
};

OSG_END_NAMESPACE;

#endif	/* MAPCOORDINATOR_H */

