#ifndef VRWORLDGENERATOR_H_INCLUDED
#define VRWORLDGENERATOR_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "addons/Semantics/VRSemanticsFwd.h"
#include "addons/WorldGenerator/GIS/GISFwd.h"
#include "addons/WorldGenerator/VRWorldGeneratorFwd.h"
#include "addons/RealWorld/VRRealWorldFwd.h"
#include "core/math/VRMathFwd.h"
#include "core/objects/VRTransform.h"
#include "addons/Bullet/VRPhysicsFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRWorldGenerator : public VRTransform {
    private:
        VRSpatialCollisionManagerPtr collisionShape;
        VROntologyPtr ontology;
        VRPlanetPtr planet;
        VRObjectManagerPtr assets;
        VRNaturePtr nature;
        VRTerrainPtr terrain;
        map<string, VRMaterialPtr> materials;
        VRRoadNetworkPtr roads;
        VRDistrictPtr district;
        OSMMapPtr osmMap;
        Vec2d coords;

        void processOSMMap(double subN = -1, double subE = -1, double subSize = -1);
        void init();

    public:
        VRWorldGenerator();
        ~VRWorldGenerator();

        static VRWorldGeneratorPtr create();
        VRWorldGeneratorPtr ptr();

        void setOntology(VROntologyPtr ontology);
        void setPlanet(VRPlanetPtr planet, Vec2d coords);
        void addAsset( string name, VRTransformPtr geo );
        void addMaterial( string name, VRMaterialPtr mat );
        void addOSMMap(string path, double subN = -1, double subE = -1, double subSize = -1);
        void reloadOSMMap(double subN = -1, double subE = -1, double subSize = -1);
        void clear();

        VROntologyPtr getOntology();
        VRPlanetPtr getPlanet();
        VRRoadNetworkPtr getRoadNetwork();
        VRObjectManagerPtr getAssetManager();
        VRNaturePtr getNature();
        VRTerrainPtr getTerrain();
        VRDistrictPtr getDistrict();
        VRMaterialPtr getMaterial(string name);

        void setupPhysics();
        void updatePhysics(Boundingbox box);

        string getStats();
};

OSG_END_NAMESPACE;

#endif // VRWORLDGENERATOR_H_INCLUDED
