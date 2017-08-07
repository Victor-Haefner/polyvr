#ifndef VRTRAFFICSIMULATION_H_INCLUDED
#define VRTRAFFICSIMULATION_H_INCLUDED

#include <OpenSG/OSGVector.h>
#include "addons/Semantics/VRSemanticsFwd.h"
#include "addons/WorldGenerator/VRWorldGeneratorFwd.h"
#include "core/math/VRMathFwd.h"
#include "core/math/graph.h"
#include "core/objects/object/VRObject.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRTrafficSimulation : public VRObject {
    private:
        struct node {
            float density = 0;
        };

        struct vehicle {
            Graph::position pos;
        };

        struct trafficLight {
            int state; // 0,1,2 = red, yellow, green
            int group;
            VRObjectPtr model;

            void updateModel();
        };

        VRRoadNetworkPtr roads;
        map<int, node> nodes;
        vector<vehicle> vehicles;
        vector<VRGeometryPtr> models;
        map<int, vector<trafficLight> > trafficLights;

    public:
        VRTrafficSimulation();
        ~VRTrafficSimulation();

        static VRTrafficSimulationPtr create();

        void setRoadNetwork(VRRoadNetworkPtr roads);

        void updateModel();

        void doTimeStep();
};

OSG_END_NAMESPACE;

#endif // VRTRAFFICSIMULATION_H_INCLUDED
