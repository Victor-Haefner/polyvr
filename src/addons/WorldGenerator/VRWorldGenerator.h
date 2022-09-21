#ifndef VRWORLDGENERATOR_H_INCLUDED
#define VRWORLDGENERATOR_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGColor.h>
#include "addons/Semantics/VRSemanticsFwd.h"
#include "addons/WorldGenerator/GIS/GISFwd.h"
#include "addons/WorldGenerator/VRWorldGeneratorFwd.h"
#include "core/math/VRMathFwd.h"
#include "core/objects/VRTransform.h"
#include "addons/Bullet/VRPhysicsFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRWorldGenerator : public VRTransform {
    public:
        struct OsmEntity {
            vector<Vec3d> pnts;
            map<string, string> tags;
        };

        ptrRFctFwd( VRUserGen, OsmEntity, bool );

    private:
        /*
        struct lodLvl {
            double scFac = 1.0;
            int depth = 0;
            VRObjectPtr obj;
            vector<VRObjectPtr> children;
        };*/

        VRLodTreePtr lodTree;
        VRSpatialCollisionManagerPtr collisionShape;
        VROntologyPtr ontology;
        VRPlanetPtr planet;
        VRObjectManagerPtr assets;
        VRNaturePtr nature;
        VRLodPtr lod;
        vector<VRTerrainPtr> terrains;
        Vec2d terrainSize = Vec2d(100,100);
        map<string, VRMaterialPtr> materials;
        map<VREntityPtr, VRGeometryPtr> miscAreaByEnt;
        VRRoadNetworkPtr roads;
        VRTrafficSignsPtr trafficSigns;
        VRDistrictPtr district;
        OSMMapPtr osmMap;
        OSMMapPtr gmlMap;
        Vec2d coords;
        VRUserGenCbPtr userCbPtr;

        void processOSMMap(double subN = -1, double subE = -1, double subSize = -1);
        void processGMLfromOSM();

    public:
        VRWorldGenerator();
        ~VRWorldGenerator();

        static VRWorldGeneratorPtr create();
        VRWorldGeneratorPtr ptr();

        void initFull();
        void initMinimum();

        // see initFull function for how to call those
        VRRoadNetworkPtr addRoadNetwork();
        VRDistrictPtr addDistrict();
        VRObjectManagerPtr addAssetManager();
        VRNaturePtr addNatureManager();
        void addSpatialCollisions();
        void addLodTree();
        void addOntology();

        void setOntology(VROntologyPtr ontology);
        void setPlanet(VRPlanetPtr planet, Vec2d coords);
        void addAsset( string name, VRTransformPtr geo );
        void addMaterial( string name, VRMaterialPtr mat );
        void addOSMMap(string path, double subN = -1, double subE = -1, double subSize = -1);
        void addGML(string path, int EPSG_Code = 31467);
        void readOSMMap(string path);
        void reloadOSMMap(double subN = -1, double subE = -1, double subSize = -1);
        void clear();
        OSMMapPtr getOSMMap();
        OSMMapPtr getGMLMap();

        //getLODTerrain();
        void setupLOD(int layers);
        VRTerrainPtr addTerrain(VRTexturePtr sat, VRTexturePtr heights, double lodf, double loddist, int lod, bool isLit, Color4f mixColor, float mixAmount);
        void setupLODTerrain(string pathMap, string pathPaint = "", float scale = 1.0, bool cache = true, bool isLit = true, Color4f mixColor = Color4f(1,1,1,1), float mixAmount = 0);
        void setLODTerrainParameters(float heightScale);
        void setTerrainSize( Vec2d in );
        Vec2d getTerrainSize();
        VRLodTreePtr getLodTree();
        VROntologyPtr getOntology();
        VRPlanetPtr getPlanet();
        Vec2d getPlanetCoords();
        VRRoadNetworkPtr getRoadNetwork();
        VRTrafficSignsPtr getTrafficSigns();
        VRObjectManagerPtr getAssetManager();
        VRNaturePtr getNature();
        VRTerrainPtr getTerrain(int i);
        vector<VRTerrainPtr> getTerrains();
        VRDistrictPtr getDistrict();
        VRMaterialPtr getMaterial(string name);
        VRGeometryPtr getMiscArea(VREntityPtr mEnt);

        void setupPhysics();
        void updatePhysics(Boundingbox box);
        void setupTerrain(string path);
        VRSpatialCollisionManagerPtr getPhysicsSystem();

        void setUserCallback(VRUserGenCbPtr cb);

        string getStats();
};

OSG_END_NAMESPACE;

#endif // VRWORLDGENERATOR_H_INCLUDED
