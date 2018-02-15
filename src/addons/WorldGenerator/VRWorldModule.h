#ifndef VRWORLDMODULE_H_INCLUDED
#define VRWORLDMODULE_H_INCLUDED

#include <map>
#include <string>
#include <vector>
#include <OpenSG/OSGVector.h>
#include "VRWorldGeneratorFwd.h"
#include "addons/Semantics/VRSemanticsFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRWorldModule {
    protected:
        VRWorldGeneratorWeakPtr world;
        VRPlanetWeakPtr planet;
        VRTerrainWeakPtr terrain;
        VROntologyWeakPtr ontology;

    public:
        VRWorldModule();
        ~VRWorldModule();

        void setWorld(VRWorldGeneratorPtr w);
};

OSG_END_NAMESPACE;

#endif // VRWORLDMODULE_H_INCLUDED
