#ifndef VRBUILDING_H
#define	VRBUILDING_H

#include "../VRWorldGeneratorFwd.h"
#include "../VRWorldModule.h"
#include "core/math/polygon.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

/*struct BuildingStructure {
    vector<Vec2d> vertices;
    vector<int*> faces;
};*/

class VRBuilding : public VRWorldModule {
    private:
        int wallType = 0;
        int windowType = 0;
        int doorType = 0;
        float height = 0;
        float ground = 0;

        VRPolygon roof;
        vector<pair<float,VRPolygon>> foundations;
        vector<pair<float,VRPolygon>> stories;

    public:
        VRBuilding();
        ~VRBuilding();

        static VRBuildingPtr create();

        /*vector<Vec2d*> getSides();
        vector<Vec2d> getCorners();

        struct Vec2fWithAdjIdx {
            Vec2d pos;
            unsigned int index;
            Vec2fWithAdjIdx* adjLeft;
            Vec2fWithAdjIdx* adjRight;
        };

        static bool sortVerticesX(Vec2fWithAdjIdx* vai1, Vec2fWithAdjIdx* vai2);
        static bool pointsOnSameSide(Vec2d p1, Vec2d p2, Vec2d l1, Vec2d l2); // checks if points are on same side of a line
        static bool pointInsideTriangle(Vec2d p, Vec2d a, Vec2d b, Vec2d c); // checks if point is inside of a triangle
        static Vec2fWithAdjIdx* copyVai(Vec2fWithAdjIdx* vai);
        static void createTriangles(BuildingStructure* bs, vector<Vec2fWithAdjIdx*>* vertices, Vec2fWithAdjIdx* v);*/

        void addFoundation(VRPolygon polygon, float H);
        void addFloor(VRPolygon polygon, float H);
        void addRoof(VRPolygon polygon);

        void computeGeometry(VRGeometryPtr walls, VRGeometryPtr roofs);
};

OSG_END_NAMESPACE;


#endif	/* VRBUILDING_H */

