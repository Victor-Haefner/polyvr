#ifndef TERRAIN_H
#define TERRAIN_H

#include "BaseWorldObject.h"
#include "../Vec2Helper.h"

using namespace OSG;
using namespace std;

namespace realworld {

    struct TerrainArea {
        vector<Vec2f> vertices;
        vector<int*> faces;
    };

    class Terrain: public BaseWorldObject{
        public:
            string id;
            vector<Vec2f> positions;
            vector<Vec3f>positions3D;

            Terrain(string id){
                this->id = id;
            }

            vector<Vec2f> getCorners(){
                return positions;
            }
    };
}

#endif // TERRAIN_H

