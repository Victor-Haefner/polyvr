#ifndef VRWORLDMODULE_H_INCLUDED
#define VRWORLDMODULE_H_INCLUDED

#include <map>
#include <string>
#include <vector>
#include <OpenSG/OSGVector.h>
#include "VRWorldGeneratorFwd.h"
#include "core/objects/VRObjectFwd.h"
#include "addons/Semantics/VRSemanticsFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRWorldModule {
    protected:
        VRWorldGeneratorWeakPtr world;
        VRPlanetWeakPtr planet;
        vector<VRTerrainPtr> terrains;
        VRTerrainWeakPtr terrain;
        VROntologyWeakPtr ontology;
        VRRoadNetworkWeakPtr roads;
        VRLodTreePtr lodTree;

    public:
        VRWorldModule();
        ~VRWorldModule();

        void setWorld(VRWorldGeneratorPtr w);
};

OSG_END_NAMESPACE;

#endif // VRWORLDMODULE_H_INCLUDED
