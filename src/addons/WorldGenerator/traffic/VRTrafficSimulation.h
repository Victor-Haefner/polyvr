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
        struct Vehicle {
            enum VISION {
                INFRONT = 0,
                FROMLEFT = 1,
                FROMRIGHT = 2
                //BEHIND = 3
            };
            enum INTENTION {
                STRAIGHT = 0,
                SWITCHLEFT = 1,
                SWITCHRIGHT = 2
                //REVERSE = 3
            };

            int vID = -1;
            VRTransformPtr t;
            VRTransformPtr offset;
            VRObjectPtr mesh;

            vector<VRGeometryPtr> turnsignalsBL;
            vector<VRGeometryPtr> turnsignalsBR;
            vector<VRGeometryPtr> turnsignalsFL;
            vector<VRGeometryPtr> turnsignalsFR;
            vector<VRGeometryPtr> headlights;
            vector<VRGeometryPtr> backlights;

            map<int, bool> vehiclesight;

            Graph::position pos;
            float speed = 0.15;
            Vec3d lastMove = Vec3d(0,0,0);
            Vec3d currentOffset = Vec3d(0,0,0);
            int lastMoveTS = 0;
            int roadFrom = -1;
            int roadTo = -1;
            int behavior = 0; //0 = straight, 1 = left, 2 = right
            int currentState = 0; //1 = leaving lane, -1 = coming onto lane
            bool laneTransition = false;

            Vehicle(Graph::position p);
            Vehicle();
            ~Vehicle();

            void destroy();
            void hide();
            void show(Graph::position p);

            int getID();
            void setID(int vID);

            bool operator==(const Vehicle& v);
        };

        struct road {
            int rID = -1;
            float density = 0;
            float length = 0;
            bool macro = true;
            map<int, int> vehicleIDs;
            int lastVehicleID = 0;
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
            vector<int> roadIDs;
            map<int, int> vehicleIDs;
        };

        VRRoadNetworkPtr roadNetwork;
        map<int, road> roads;
        map<int, Vehicle> vehicles;
        vector<int> seedRoads;
        vector<int> nearRoads;
        vector<Vehicle> users;
        list<Vehicle> vehiclePool;
        vector<VRObjectPtr> models;
        map<int, vector<trafficLight> > trafficLights;
        int maxUnits = 0;
        int numUnits = 0;
        bool isSimRunning = true;
        float speedMultiplier = 1.0;
        string lastseedRoadsString = "";
        int debugOverRideSeedRoad = -1;

        VRMaterialPtr carLightWhiteOn;
        VRMaterialPtr carLightWhiteOff;
        VRMaterialPtr carLightRedOn;
        VRMaterialPtr carLightRedOff;
        VRMaterialPtr carLightOrangeOn;
        VRMaterialPtr carLightOrangeOff;
        VRMaterialPtr carLightOrangeBlink;
        VRUpdateCbPtr turnSignalCb;

        VRUpdateCbPtr updateCb;
        VRGeometryPtr flowGeo;

        void updateTurnSignal();

    public:
        VRTrafficSimulation();
        ~VRTrafficSimulation();

        static VRTrafficSimulationPtr create();

        void setRoadNetwork(VRRoadNetworkPtr roads);
        void updateSimulation();
        void updateDensityVisual(bool remesh = false);

        void addUser(VRTransformPtr t);

        void addVehicle(int roadID, float density, int type);
        void addVehicles(int roadID, float density, int type);
        void setTrafficDensity(float density, int type, int maxUnits = 0);

        int addVehicleModel(VRObjectPtr mesh);

        void changeLane(int ID, int direction);

        //diagnostics:
        void toggleSim();
        void setSpeedmultiplier(float speedMultiplier);
        void forceIntention(int vID,int behavior);
        string getVehicleData(int ID);
        void runDiagnostics();
        void setSeedRoad(int debugOverRideSeedRoad);
        bool isSeedRoad(int roadID);
};

OSG_END_NAMESPACE;

#endif // VRTRAFFICSIMULATION_H_INCLUDED
