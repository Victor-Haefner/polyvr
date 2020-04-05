#ifndef VRWORLDMODULE_H_INCLUDED
#define VRWORLDMODULE_H_INCLUDED

#include <map>
#include <string>
#include <vector>
#include "core/math/OSGMathFwd.h"
#include "VRWorldGeneratorFwd.h"
#include "core/objects/VRObjectFwd.h"
#include "addons/Semantics/VRSemanticsFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRWorldModule {
    protected:
        VRWorldGeneratorWeakPtr world;
        VRPlanetWeakPtr planet;
        VRTerrainWeakPtr terrain;
        vector<VRTerrainPtr> terrains;
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
