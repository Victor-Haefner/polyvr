#ifndef WALL_H
#define	WALL_H

#include "BaseWorldObject.h"
#include "../Vec2Helper.h"

OSG_BEGIN_NAMESPACE;
using namespace std;


struct WallStructure {
    vector<Vec2d> vertices;
    vector<int*> faces;
};

class Wall: public BaseWorldObject {
public:
    string id;
    vector<Vec2d> positions;
    float width;
    float height;

    Wall(string id);

    vector<Vec2d*> getSides();

    vector<Vec2d> getCorners();

    struct Vec2fWithAdjIdx {
        Vec2d pos;
        unsigned int index;
        Vec2fWithAdjIdx* adjLeft;
        Vec2fWithAdjIdx* adjRight;
    };

    static bool sortVerticesX(Vec2fWithAdjIdx* vai1, Vec2fWithAdjIdx* vai2);
};

OSG_END_NAMESPACE;


#endif	/* WALL_H */

