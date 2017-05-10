#ifndef VRROADNETWORK_H_INCLUDED
#define VRROADNETWORK_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "addons/Semantics/VRSemanticsFwd.h"
#include "addons/WorldGenerator/VRWorldGeneratorFwd.h"
#include "core/math/graph.h"
#include "core/objects/object/VRObject.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRRoadNetwork : public VRObject {
    private:
        GraphPtr graph;
        VRAsphaltPtr asphalt;
        VROntologyPtr ontology;

    public:
        VRRoadNetwork();
        ~VRRoadNetwork();

        static VRRoadNetworkPtr create();

        void setOntology(VROntologyPtr ontology);
        GraphPtr getGraph();
        void updateTexture();
};

OSG_END_NAMESPACE;

#endif // VRROADNETWORK_H_INCLUDED
