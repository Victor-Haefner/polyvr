#ifndef MODULETERRAIN_H
#define MODULETERRAIN_H

#include "BaseModule.h"
#include "Terrain.h"
#include "../World.h"
#include "../OSM/OSMMapDB.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

struct TerrainMaterial {
    VRMaterialPtr material;
    string k;
    string v;
    float height;
};

class ModuleTerrain : public BaseModule {
    public:
        virtual string getName();

        virtual void loadBbox(AreaBoundingBox* bbox);

    virtual void unloadBbox(AreaBoundingBox* bbox);

    void physicalize(bool b);

    ModuleTerrain(OSMMapDB* mapDB, MapCoordinator* mapCoordinator, World* world);

private:
    vector<TerrainMaterial*> terrainList;
    map<string, VRMaterialPtr> materials;
    //TerrainMaterials* matTerrain;
    OSMMapDB* mapDB;
    map<string, VRGeometryPtr> meshes;
    map<string, VRGeometryPtr>::iterator mesh_itr;

    void fillTerrainList();

    void addTerrain(string t, string k, string v);
    void addTerrain(string texture, string key, string value, int height);
    void addTerrain(Terrain* ter, GeometryData* gdTerrain, int height);

    Vec3f tesselateTriangle(float p1X, float p1Y, float p2X, float p2Y, float p3X, float p3Y, float height, GeometryData* gdTerrain);

    Vec3f tesselateTriangle(float p1X, float p1Y, float p2X, float p2Y, float p3X, float p3Y, float height, GeometryData* gdTerrain, Vec3f usedCorners);

    VRGeometryPtr makeTerrainGeometry(Terrain* ter, TerrainMaterial* terrainMat);

};

OSG_END_NAMESPACE;

#endif // MODULETERRAIN_H

