#ifndef MODULEWALLS_H
#define MODULEWALLS_H

#include "../OSM/OSMMapDB.h"
#include "BaseModule.h"
#include "../World.h"
#include "core/objects/material/VRShader.h"
#include "triangulate.h"
#include "Wall.h"
#include <OpenSG/OSGSimpleMaterial.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

struct WallMaterial {
    SimpleMaterialRecPtr material;
    string k;
    string v;
    float width;
    float height;
};

class ModuleWalls: public BaseModule {
    public:
        virtual void loadBbox(AreaBoundingBox* bbox);
        virtual void unloadBbox(AreaBoundingBox* bbox);

        void physicalize(bool b);

        ModuleWalls();

    private:
        vector<WallMaterial*> wallList;

        void fillWallList();

        void addWall(string texture, string key, string value, float width, float height);
        void addWall(string texture, string key, string value);

        void addWallPart(Vec2f a1, Vec2f b1, Vec2f a2, Vec2f b2, Vec2f a3, Vec2f b3, GeometryData* gdWall, float width, float height);
        Vec2f getNOrtho(Vec2f a, Vec2f b);

        Vec2f getIntersection(Vec2f a1, Vec2f b1, Vec2f a2, Vec2f b2);
        void createWallSide(Vec2f a, Vec2f b, Vec2f normal2D, GeometryData* gdWall, float height);
        void createWallRoof(Vec2f a1, Vec2f a2, Vec2f b1, Vec2f b2, GeometryData* gdWall, float height);
        //to do
        void addWall(Wall* wall, GeometryData* gdWall, float width, float height);

        VRGeometryPtr makeWallGeometry(Wall* wall, WallMaterial* wallMat);
};

OSG_END_NAMESPACE;

#endif // MODULEWALLS_H

