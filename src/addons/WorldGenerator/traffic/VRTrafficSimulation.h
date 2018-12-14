#ifndef VRTRAFFICSIMULATION_H_INCLUDED
#define VRTRAFFICSIMULATION_H_INCLUDED

#include <OpenSG/OSGVector.h>
#include "addons/Semantics/VRSemanticsFwd.h"
#include "addons/WorldGenerator/VRWorldGeneratorFwd.h"
#include "core/math/VRMathFwd.h"
#include "core/math/pose.h"
#include "core/math/graph.h"
#include "core/objects/object/VRObject.h"
#include "core/utils/VRDoublebuffer.h"
#include <boost/thread/recursive_mutex.hpp>

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRTrafficSimulation : public VRObject {
    public:
        enum VEHICLE {
            SCOOTER = 0,
            CAR = 1,
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
            enum INTENTION {
                STRAIGHT = 0,
                SWITCHLEFT = 1,
                SWITCHRIGHT = 2,
                REVERSE = 3
            };
            enum DecisionSTATE {
                DRIVE,
                WAIT
            };

            ///Data
            int vID = -1;
            VRTransformPtr t;
            VRTransformPtr offset;
            VRObjectPtr mesh;
            Graph::position pos;
            float length = 4.4;
            float width = 1.7;
            bool isUser = false;
            bool simVisible = false;
            bool vrwVisible = false;
            bool collisionDetected;
            PosePtr simPose;
            Pose simPose2;
            //doubleBuffer poseBuffer;
            int type;
            vector<int> signaling;

            vector<VRGeometryPtr> turnsignalsBL;
            vector<VRGeometryPtr> turnsignalsBR;
            vector<VRGeometryPtr> turnsignalsFL;
            vector<VRGeometryPtr> turnsignalsFR;
            vector<VRGeometryPtr> headlights;
            vector<VRGeometryPtr> backlights;

            ///Perception
            float distanceToNextSignal;
            float distanceToNextStop;
            float distanceToNextIntersec;
            Vec3d nextStop;
            string nextSignalState; //"000" - red|organge|green
            bool signalAhead;
            bool incTrafficRight = false;
            bool incTrafficLeft = false;
            bool incTrafficStraight = false;
            bool incTrafficFront = false;
            float frontVehicLastMove = 0;

            map<int, float> vehiclesight;
            map<int, float> vehiclesightFar;
            map<int, int> vehiclesightFarID;
            map<int, Vec3d> vehicleFPs;
            vector<int> nextLanesCoices;
            int lastFPTS = 0;

            ///Behavior
            DecisionSTATE state = DRIVE;
            Vec3d lastMove;

            float currentVelocity;
            float targetVelocity;
            float roadVelocity;
            float maxAcceleration;
            float maxDecceleration;
            float acceleration;
            float decceleration;

            float lastMoveTS = 0;
            float indicatorTS = 0;
            float lastLaneSwitchTS = 0;
            int roadFrom;
            int roadTo;
            int behavior = 0; //0 = straight, 1 = left, 2 = right
            int turnAhead = 0; //0 = straight, 1 = left, 2 = right
            int laneChangeState = 0; //1 = leaving lane, -1 = coming onto lane
            int nextTurnLane;
            bool laneTransition = true;
            bool turnAtIntersec = false;
            Vec3d currentOffset;
            Vec3d currentdOffset;
            Vec3d nextIntersection = Vec3d(0,-20,0);
            VRRoadIntersectionPtr lastIntersection;
            VRRoadIntersectionPtr lastFoundIntersection;

            Vehicle(Graph::position p, int type);
            Vehicle();
            ~Vehicle();

            void setupSG(VRObjectPtr g, map<string, VRMaterialPtr>& lightMaterials);
            void destroy();
            void hide();
            void setDefaults();
            void show();

            bool operator==(const Vehicle& v);

            void signalLights(int input, map<string, VRMaterialPtr>& lightMaterials);
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
        VRThreadCbPtr worker;

        boost::recursive_mutex mtx;
        boost::recursive_mutex mtx2;

        map<int, laneSegment> roads;
        map<int, Vehicle> vehicles;
        map<string, VRMaterialPtr> lightMaterials;
        map<int, map<int, int>> visionVecSaved;
        vector<int> seedRoads;
        vector<int> nearRoads;
        vector<int> forceSeedRoads;
        vector<Vehicle> users;
        list<int> vehiclePool;
        vector<VRObjectPtr> models;
        int maxUnits = 0;
        int numUnits = 0;
        size_t nID = -1;
        float deltaT;
        float lastT = 0.0;
        Vec3d globalOffset = Vec3d(0,0,0);

        float environmentFactor = 1; // 4 snow
        float roadFactor = 1; //1 in city, 2 highway

        float roadVelocity;

        string lastseedRoadsString = "";
        //int debugOverRideSeedRoad = -1;
        float killswitch1 = 5;
        float killswitch2 = 200;
        VRUpdateCbPtr turnSignalCb;

        VRUpdateCbPtr updateCb;
        VRGeometryPtr flowGeo;

        void updateTurnSignal();
        void updateGraph();
        void updateVehicVision();
        void updateVehicIntersecs();
        void updateIntersectionVis(bool in);

        ///Diagnostics
        bool hidden = false;
        int stopVehicleID = -1;
        int deleteVehicleID = -1;
        bool isSimRunning = true;
        bool isUpdRunning = true;
        bool isTimeForward = true;
        bool isShowingVehicleVision = false;
        bool isShowingGraph = false;
        bool isShowingIntersecs = false;
        bool isShowingGeometries = true;
        bool isShowingMarkers = false;
        int whichVehicleMarkers = -1;
        bool laneChange = true;
        float speedMultiplier = 1.0;
        int debugOverRideSeedRoad = -1;
        float visibilityRadius = 100;

    public:
        VRTrafficSimulation();
        ~VRTrafficSimulation();

        static VRTrafficSimulationPtr create();

        void initiateWorker();
        void trafficSimThread(VRThreadWeakPtr tw);

        void setRoadNetwork(VRRoadNetworkPtr roads);
        void updateSimulation();
        void updateDensityVisual(bool remesh = false);

        void addUser(VRTransformPtr t);
        VRTransformPtr getUser();

        void addVehicle(int roadID, float density, int type);
        void setTrafficDensity(float density, int type, int maxUnits = 0);

        int addVehicleModel(VRObjectPtr mesh);
        void setGlobalOffset(Vec3d globalOffset);

        void changeLane(int ID, int direction);

        ///Diagnostics:
        void toggleSim();
        void toggleSimUpd();
        void toggleVisibility();
        void toggleDirection();
        void setSpeedmultiplier(float speedMultiplier);
        void toggleGraph();
        void toggleIntersections();
        void toggleVehicVision();
        void toggleVehicMarkers(int i);
        void toggleLaneChanges();
        void forceIntention(int vID,int behavior);
        string getVehicleData(int ID);
        string getEdgeData(int ID);
        void runDiagnostics();
        void runVehicleDiagnostics();
        void stopVehicle(int ID);
        void setKillswitches(float k1, float k2);
        void deleteVehicle(int ID);
        void setSeedRoad(int debugOverRideSeedRoad);
        void setSeedRoadVec(vector<int> forceSeedRoads);
        void setVisibilityRadius(float visibilityRadius);
        bool isSeedRoad(int roadID);
};

OSG_END_NAMESPACE;

#endif // VRTRAFFICSIMULATION_H_INCLUDED
