#ifndef WALL_H
#define	WALL_H

#include "BaseWorldObject.h"
#include "../Vec2Helper.h"

using namespace OSG;
using namespace std;

namespace realworld {

    struct WallStructure {
        vector<Vec2f> vertices;
        vector<int*> faces;
    };

    class Wall: public BaseWorldObject {
    public:
        string id;
        vector<Vec2f> positions;
        float width;
        float height;

        Wall(string id){
            this->id = id;
        }

        vector<Vec2f*> getSides() {
            vector<Vec2f*> result;

            for (unsigned int i=0; i<this->positions.size()-1; i++) {
                Vec2f* side = new Vec2f[2];
                side[0] = this->positions[i];
                side[1] = this->positions[(i+1) % this->positions.size()];
                result.push_back(side);
            }

            return result;
        }

        vector<Vec2f> getCorners(){
            return positions;
        }

        struct Vec2fWithAdjIdx {
            Vec2f pos;
            unsigned int index;
            Vec2fWithAdjIdx* adjLeft;
            Vec2fWithAdjIdx* adjRight;
        };

        static bool sortVerticesX(Vec2fWithAdjIdx* vai1, Vec2fWithAdjIdx* vai2) {
            return (vai1->pos.getValues()[0] > vai2->pos.getValues()[0]);
        }
    };
}


#endif	/* WALL_H */

