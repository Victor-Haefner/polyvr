#ifndef VRWORLDGENERATOR_H_INCLUDED
#define VRWORLDGENERATOR_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "addons/Semantics/VRSemanticsFwd.h"
#include "addons/WorldGenerator/VRWorldGeneratorFwd.h"
#include "addons/RealWorld/VRRealWorldFwd.h"
#include "core/math/VRMathFwd.h"
#include "core/objects/VRTransform.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRWorldGenerator : public VRTransform {
    private:
        VROntologyPtr ontology;
        VRObjectManagerPtr assets;
        VRNaturePtr nature;
        VRTerrainPtr terrain;
        map<string, VRMaterialPtr> materials;
        VRRoadNetworkPtr roads;

        static string assetMatVShdr;
        static string assetMatFShdr;
        static string assetTexMatVShdr;
        static string assetTexMatFShdr;

        void init();

    public:
        VRWorldGenerator();
        ~VRWorldGenerator();

        static VRWorldGeneratorPtr create();
        VRWorldGeneratorPtr ptr();

        void setOntology(VROntologyPtr ontology);
        VROntologyPtr getOntology();
        VRRoadNetworkPtr getRoadNetwork();
        VRObjectManagerPtr getAssetManager();
        VRNaturePtr getNature();
        VRTerrainPtr getTerrain();

        void addAsset( string name, VRTransformPtr geo );
        void addMaterial( string name, VRMaterialPtr mat );
        VRMaterialPtr getMaterial(string name);
};

OSG_END_NAMESPACE;

#endif // VRWORLDGENERATOR_H_INCLUDED
