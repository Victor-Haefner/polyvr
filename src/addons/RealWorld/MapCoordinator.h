#ifndef MAPCOORDINATOR_H
#define	MAPCOORDINATOR_H

#include <OpenSG/OSGVector.h>

using namespace std;
OSG_BEGIN_NAMESPACE;

class Elevation;

class MapCoordinator {
    private:
        Vec2d zeroPos;
        Vec2d offset;
        float gridSize;
        float SCALE_REAL_TO_WORLD = 111000.0;
        Elevation* ele = 0;
        float startElevation = 0;

    public:
        MapCoordinator(Vec2d zeroPos, float gridSize);

        Vec2d realToWorld(Vec2d realPosition);
        Vec2d worldToReal(Vec2d worldPosition);

        float getGridSize();
        float getElevation(float x, float y);
        float getElevation(Vec2d v);
        Vec2d getRealBboxPosition(Vec2d worldPosition);
};

OSG_END_NAMESPACE;

#endif	/* MAPCOORDINATOR_H */

