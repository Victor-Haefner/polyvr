#ifndef VRWORLDGENERATOR_H_INCLUDED
#define VRWORLDGENERATOR_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "addons/Semantics/VRSemanticsFwd.h"
#include "addons/WorldGenerator/VRWorldGeneratorFwd.h"
#include "addons/RealWorld/VRRealWorldFwd.h"
#include "core/math/VRMathFwd.h"
#include "core/objects/object/VRObject.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRWorldGenerator : public VRObject {
    private:
        VROntologyPtr ontology;
        map<string, VRTransformPtr> assets;
        VRRoadNetworkPtr roads;

        void init();

    public:
        VRWorldGenerator();
        ~VRWorldGenerator();

        static VRWorldGeneratorPtr create();
        VRWorldGeneratorPtr ptr();

        void setOntology(VROntologyPtr ontology);
        VROntologyPtr getOntology();
        VRRoadNetworkPtr getRoadNetwork();

        void addAsset( string name, VRTransformPtr geo );
        VRTransformPtr getAsset(string name);
};

OSG_END_NAMESPACE;

#endif // VRWORLDGENERATOR_H_INCLUDED
