#ifndef MODULETERRAIN_H
#define MODULETERRAIN_H

#include "BaseModule.h"
#include <map>

OSG_BEGIN_NAMESPACE;
using namespace std;

class Terrain;
class GeometryData;

struct TerrainMaterial {
    VRMaterialPtr material;
    string k;
    string v;
    float height;
};

class ModuleTerrain : public BaseModule {
    public:
        virtual void loadBbox(AreaBoundingBox* bbox);
        virtual void unloadBbox(AreaBoundingBox* bbox);

        void physicalize(bool b);

        ModuleTerrain();

    private:
        vector<TerrainMaterial*> terrainList;
        map<string, VRMaterialPtr> materials;
        map<string, VRGeometryPtr> meshes;

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

