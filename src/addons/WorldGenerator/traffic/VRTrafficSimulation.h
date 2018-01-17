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
    public:
        enum VEHICLE {
            CAR = 0,
            SCOOTER = 1,
            BYCICLE = 2
        };

    private:
        struct vehicle {
            Graph::position pos;
            VRTransformPtr t;
            VRObjectPtr mesh;

            vehicle(Graph::position p);
            ~vehicle();
        };

        struct road {
            float density = 0;
            vector<vehicle> vehicles;
            VRRoadPtr r;
        };

        struct trafficLight {
            int state; // 0,1,2 = red, yellow, green
            int group;
            VRObjectPtr model;

            void updateModel();
        };

        struct intersection {
            float density = 0;
            vector<road> roads;
            vector<vehicle> vehicles;
        };

        VRRoadNetworkPtr roadNetwork;
        map<int, road> roads;
        vector<vehicle> vehicles;
        vector<VRObjectPtr> models;
        map<int, vector<trafficLight> > trafficLights;

        VRUpdateCbPtr updateCb;
        VRGeometryPtr flowGeo;

    public:
        VRTrafficSimulation();
        ~VRTrafficSimulation();

        static VRTrafficSimulationPtr create();

        void setRoadNetwork(VRRoadNetworkPtr roads);
        void updateSimulation();
        void updateDensityVisual();

        void addVehicle(int roadID, int type);
        void addVehicles(int roadID, float density, int type);
        void setTrafficDensity(float density, int type);

        void addVehicleModel(VRObjectPtr mesh);
};

OSG_END_NAMESPACE;

#endif // VRTRAFFICSIMULATION_H_INCLUDED
