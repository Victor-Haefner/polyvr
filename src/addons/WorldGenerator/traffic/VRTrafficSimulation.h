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
        enum VISION {
                INFRONT = 0,
                FRONTLEFT = 1,
                FRONTRIGHT = 2,
                BEHINDLEFT = 3,
                BEHINDRIGHT = 4,
                BEHIND = 5
        };

    private:
        struct Vehicle {
            enum VISION {
                INFRONT = 0,
                FRONTLEFT = 1,
                FRONTRIGHT = 2,
                BEHINDLEFT = 3,
                BEHINDRIGHT = 4,
                BEHIND = 5
            };
            enum INTENTION {
                STRAIGHT = 0,
                SWITCHLEFT = 1,
                SWITCHRIGHT = 2,
                REVERSE = 3
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

            map<int, float> vehiclesight;
            map<int, float> vehiclesightFar;
            map<int, int> vehiclesightFarID;
            map<int, Vec3d> vehicleFPs;
            int lastFPTS = 0;

            Graph::position pos;
            float length = 4.4;
            float width = 1.7;
            float speed;
            float currentVelocity;
            float targetVelocity;
            float maxAcceleration;
            float maxDecceleration;
            float Acceleration;
            float Decceleration;
            float DistanceToNextSignal;
            float DistanceToNextIntersec;
            Vec3d lastMove;
            Vec3d currentOffset;
            Vec3d currentdOffset;
            int lastMoveTS = 0;
            int indicatorTS = 0;
            int lastLaneSwitchTS = 0;
            int roadFrom;
            int roadTo;
            int behavior = 0; //0 = straight, 1 = left, 2 = right
            int currentState = 0; //1 = leaving lane, -1 = coming onto lane
            bool laneTransition = true;
            bool isUser = false;
            bool collisionDetected;
            VRRoadIntersectionPtr lastIntersection;

            Vehicle(Graph::position p);
            Vehicle();
            ~Vehicle();

            void destroy();
            void hide();
            void setDefaults();
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

        struct laneSegment { //road
            int rID = -1;
            float density = 0;
            float length = 0;
            bool macro = true;
            map<int, int> vehicleIDs;
            int lastVehicleID = 0;
            VRRoadPtr road;
            VREntityPtr lane;
            vector<signal> signals;
        };

        struct intersection {
            float density = 0;
            vector<int> roadIDs;
            map<int, int> vehicleIDs;
        };

        VRRoadNetworkPtr roadNetwork;
        map<int, laneSegment> roads;
        map<int, Vehicle> vehicles;
        vector<int> seedRoads;
        vector<int> nearRoads;
        vector<int> forceSeedRoads;
        vector<Vehicle> users;
        list<Vehicle> vehiclePool;
        vector<VRObjectPtr> models;
        size_t nID = -1;
        int maxUnits = 0;
        int numUnits = 0;
        int stopVehicleID = -1;
        bool isSimRunning = true;
        bool isShowingVehicleVision = false;
        bool isShowingGraph = false;
        bool isShowingGeometries = true;
        bool laneChange = false;
        float speedMultiplier = 1.0;
        float deltaT;
        float lastT = 0.0;
        string lastseedRoadsString = "";
        //int debugOverRideSeedRoad = -1;
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
        void updateGraph();

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
        void runWithGeometries();
        void setSpeedmultiplier(float speedMultiplier);
        void showGraph();
        void hideGraph();
        void showIntersections();
        void hideIntersections();
        void showVehicVision();
        void hideVehicVision();
        void toggleLaneChanges();
        void forceIntention(int vID,int behavior);
        string getVehicleData(int ID);
        string getEdgeData(int ID);
        void runDiagnostics();
        void stopVehicle(int ID);
        void setSeedRoad(int debugOverRideSeedRoad);
        void setSeedRoadVec(vector<int> forceSeedRoads);
        bool isSeedRoad(int roadID);
};

OSG_END_NAMESPACE;

#endif // VRTRAFFICSIMULATION_H_INCLUDED
