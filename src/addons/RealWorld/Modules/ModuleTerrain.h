#ifndef MODULETERRAIN_H
#define MODULETERRAIN_H

#include "BaseModule.h"
#include <map>

OSG_BEGIN_NAMESPACE;
using namespace std;

class Terrain;
class VRGeoData;

struct TerrainMaterial {
    VRMaterialPtr material;
    string k;
    string v;
    float height;
};

class ModuleTerrain : public BaseModule {
    public:
        virtual void loadBbox(MapGrid::Box bbox);
        virtual void unloadBbox(MapGrid::Box bbox);

        void physicalize(bool b);

        ModuleTerrain(bool t, bool p);

    private:
        vector<TerrainMaterial*> terrainList;
        map<string, VRMaterialPtr> materials;
        map<string, VRGeometryPtr> meshes;

        void fillTerrainList();

        void addTerrain(string texture, string key, string value, int height = 0);
        void addTerrain(Terrain* ter, VRGeoData* gdTerrain, int height);
        Vec3f tesselateTriangle(float p1X, float p1Y, float p2X, float p2Y, float p3X, float p3Y, float height, VRGeoData* gdTerrain, Vec3f usedCorners = Vec3f());
        VRGeometryPtr makeTerrainGeometry(Terrain* ter, TerrainMaterial* terrainMat);
};

OSG_END_NAMESPACE;

#endif // MODULETERRAIN_H

