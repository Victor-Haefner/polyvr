#ifndef TERRAIN_H
#define TERRAIN_H

#include "BaseWorldObject.h"
#include "../Vec2Helper.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

struct TerrainArea {
    vector<Vec2f> vertices;
    vector<int*> faces;
};

class Terrain: public BaseWorldObject{
    public:
        string id;
        vector<Vec2f> positions;
        vector<Vec3f>positions3D;

        Terrain(string id);

        vector<Vec2f> getCorners();
};

OSG_END_NAMESPACE;

#endif // TERRAIN_H

