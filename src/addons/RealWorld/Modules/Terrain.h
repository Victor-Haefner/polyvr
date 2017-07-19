#ifndef TERRAIN_H
#define TERRAIN_H

#include "BaseWorldObject.h"
#include "../Vec2Helper.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

struct TerrainArea {
    vector<Vec2d> vertices;
    vector<int*> faces;
};

class Terrain: public BaseWorldObject{
    public:
        string id;
        vector<Vec2d> positions;
        vector<Vec3d>positions3D;

        Terrain(string id);

        vector<Vec2d> getCorners();
};

OSG_END_NAMESPACE;

#endif // TERRAIN_H

