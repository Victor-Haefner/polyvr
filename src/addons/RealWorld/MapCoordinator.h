#ifndef MAPCOORDINATOR_H
#define	MAPCOORDINATOR_H

#include <OpenSG/OSGVector.h>

using namespace std;
OSG_BEGIN_NAMESPACE;

class Elevation;

class MapCoordinator {
    private:
        Vec2f zeroPos;
        Vec2f offset;
        float gridSize;
        float SCALE_REAL_TO_WORLD = 111000.0;
        Elevation* ele = 0;
        float startElevation = 0;

    public:
        MapCoordinator(Vec2f zeroPos, float gridSize);

        Vec2f realToWorld(Vec2f realPosition);
        Vec2f worldToReal(Vec2f worldPosition);

        float getGridSize();
        float getElevation(float x, float y);
        float getElevation(Vec2f v);
        Vec2f getRealBboxPosition(Vec2f worldPosition);
};

OSG_END_NAMESPACE;

#endif	/* MAPCOORDINATOR_H */

