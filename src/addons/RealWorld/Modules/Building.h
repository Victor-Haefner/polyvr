#ifndef BUILDING_H
#define	BUILDING_H

#include "BaseWorldObject.h"
#include "../Vec2Helper.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

struct BuildingStructure {
    vector<Vec2f> vertices;
    vector<int*> faces;
};

class Building: public BaseWorldObject {
public:
    string id;
    vector<Vec2f> positions;

    Building(string id);

    vector<Vec2f*> getSides();
    vector<Vec2f> getCorners();

    bool isClockwise();
    void makeClockwise();
    bool isClockwiseCorrect();

    struct Vec2fWithAdjIdx {
        Vec2f pos;
        unsigned int index;
        Vec2fWithAdjIdx* adjLeft;
        Vec2fWithAdjIdx* adjRight;
    };

    static bool sortVerticesX(Vec2fWithAdjIdx* vai1, Vec2fWithAdjIdx* vai2);

    // checks if points are on same side of a line
    static bool pointsOnSameSide(Vec2f p1, Vec2f p2, Vec2f l1, Vec2f l2);

    // checks if point is inside of a triangle
    static bool pointInsideTriangle(Vec2f p, Vec2f a, Vec2f b, Vec2f c);

    static Vec2fWithAdjIdx* copyVai(Vec2fWithAdjIdx* vai);

    static void createTriangles(BuildingStructure* bs, vector<Vec2fWithAdjIdx*>* vertices, Vec2fWithAdjIdx* v);

    BuildingStructure* getStructure();
};

OSG_END_NAMESPACE;


#endif	/* BUILDING_H */

