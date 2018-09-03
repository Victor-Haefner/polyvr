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
            map<int, bool> vehiclesightFar;

            Graph::position pos;
            float speed = 50;
            //float speed = 0.15;
            float targetVelocity = 0.0; //try km/h
            float currentVelocity = 0.0;
            float maxAcceleration = 0.0;
            float maxDecceleration = 0.0;
            Vec3d lastMove = Vec3d(0,0,0);
            Vec3d currentOffset = Vec3d(0,0,0);
            Vec3d currentdOffset = Vec3d(0,0,0);
            int lastMoveTS = 0;
            int indicatorTS = 0;
            int roadFrom = -1;
            int roadTo = -1;
            int behavior = 0; //0 = straight, 1 = left, 2 = right
            int currentState = 0; //1 = leaving lane, -1 = coming onto lane
            bool laneTransition = true;

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

        struct signal {
            float position; // edge coordinate from 0 to 1
            string type;
            string state;
        };

        struct road {
            int rID = -1;
            float density = 0;
            float length = 0;
            bool macro = true;
            map<int, int> vehicleIDs;
            int lastVehicleID = 0;
            VRRoadPtr r;
            vector<signal> signals;
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
        vector<int> forceSeedRoads;
        vector<Vehicle> users;
        list<Vehicle> vehiclePool;
        vector<VRObjectPtr> models;
        int maxUnits = 0;
        int numUnits = 0;
        bool isSimRunning = true;
        bool isShowingVehicleVision = false;
        bool isShowingGeometries = true;
        bool laneChange = false;
        float speedMultiplier = 1.0;
        float deltaT;
        float lastT = 0.0;
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
        void runWithoutGeometries();
        void setSpeedmultiplier(float speedMultiplier);
        void showGraph();
        void hideGraph();
        void showVehicVision();
        void hideVehicVision();
        void toggleLaneChanges();
        void forceIntention(int vID,int behavior);
        string getVehicleData(int ID);
        string getEdgeData(int ID);
        void runDiagnostics();
        void setSeedRoad(int debugOverRideSeedRoad);
        void setSeedRoadVec(vector<int> forceSeedRoads);
        bool isSeedRoad(int roadID);
};

OSG_END_NAMESPACE;

#endif // VRTRAFFICSIMULATION_H_INCLUDED
