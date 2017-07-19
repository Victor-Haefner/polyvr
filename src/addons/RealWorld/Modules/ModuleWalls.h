#ifndef MODULEWALLS_H
#define MODULEWALLS_H

#include "BaseModule.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class Wall;
class VRGeoData;

struct WallMaterial {
    VRMaterialPtr material;
    string k;
    string v;
    float width;
    float height;
};

class ModuleWalls: public BaseModule {
    public:
        virtual void loadBbox(MapGrid::Box bbox);
        virtual void unloadBbox(MapGrid::Box bbox);

        void physicalize(bool b);

        ModuleWalls(bool t, bool p);

    private:
        vector<WallMaterial*> wallList;

        void fillWallList();

        void addWall(string texture, string key, string value, float width, float height);
        void addWall(string texture, string key, string value);

        void addWallPart(Vec2d a1, Vec2d b1, Vec2d a2, Vec2d b2, Vec2d a3, Vec2d b3, VRGeoData& gdWall, float width, float height);
        Vec2d getNOrtho(Vec2d a, Vec2d b);

        Vec2d getIntersection(Vec2d a1, Vec2d b1, Vec2d a2, Vec2d b2);
        void createWallSide(Vec2d a, Vec2d b, Vec2d normal2D, VRGeoData& gdWall, float height);
        void createWallRoof(Vec2d a1, Vec2d a2, Vec2d b1, Vec2d b2, VRGeoData& gdWall, float height);
        //to do
        void addWall(Wall* wall, VRGeoData& gdWall, float width, float height);
};

OSG_END_NAMESPACE;

#endif // MODULEWALLS_H

