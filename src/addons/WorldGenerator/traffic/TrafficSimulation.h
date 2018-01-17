#ifndef OldTrafficSimulation_H
#define OldTrafficSimulation_H

#include "core/objects/object/VRObject.h"
#include "core/objects/VRObjectFwd.h"
#include "core/utils/VRFunctionFwd.h"
#include "TrafficSimulator.h"

#include <set>
#include <map>
#include <OpenSG/OSGVector.h>
#include "addons/WorldGenerator/GIS/GISFwd.h"
#include "addons/WorldGenerator/VRWorldGeneratorFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class OldTrafficSimulation : public VRObject {
    private:
        VRUpdateCbPtr updateCb;
        TrafficSimulator sim;

    public:
        struct Vehicle {
            unsigned int id;
            unsigned int vehicleTypeId;
            unsigned int driverTypeId;

            Vec3d pos;
            Vec3d deltaPos; // The movement which is added to the vehicle every second.
            Vec3d orientation; // The orientation of the vehicle.
            Vec3d deltaOrientation; // The rotation which is added to the orientation every second.

            typedef uint8_t VehicleState; // Possible state-flags, use them with bit-wise & |
            static const VehicleState NONE            = 0;
            static const VehicleState RIGHT_INDICATOR = 1 << 0;
            static const VehicleState LEFT_INDICATOR  = 1 << 1;
            static const VehicleState ACCELERATING    = 1 << 2;
            static const VehicleState WAITING         = 1 << 3;
            static const VehicleState BLOCKED         = 1 << 4;
            static const VehicleState BRAKING         = 1 << 5;
            static const VehicleState COLLIDED        = 1 << 6;

            VehicleState state;
            VRTransformPtr model;
        };

    private:
        VRMaterialPtr a_red = 0;
        VRMaterialPtr a_orange = 0;
        VRMaterialPtr a_green = 0;

        double driverVehicleRadius = 0.5; // collision radius of user
        double viewDistance = 200;

        bool (*collisionHandler) (Vehicle& a, Vehicle& b); // handles collisions between vehicles

        VRTransformPtr player;
        bool playerCreated = false; // init flag

        map<unsigned int, VRTransformPtr> meshes; // vehicle types meshes
        map<unsigned int, Vehicle> vehicles;
        vector<VRGeometryPtr> lightBulbs; // traffic lights

    public:
        OldTrafficSimulation();
        ~OldTrafficSimulation();

        static OldTrafficSimulationPtr create();

        void setDrawingDistance(double distance = 200); // The maximal distance to draw vehicles
        void setTrafficDensity(double density = 10); // density will vary based on street type, number of vehicles on 100m norm-street

        // probability: vehicle spawning, maxSpeed in km/h, maxAcceleration in m/s^2, maxRotation in °/s
        void addVehicleType(const unsigned int id, const double probability, const double collisionRadius, const double maxSpeed, const double maxAcceleration, const double maxRoration, VRTransformPtr model);

        // behavior of AI driver, probability: driver spawning, lawlessness [0.0 to 1.0], cautiousness [0.0 to 1.0]
        void addDriverType(const unsigned int id, const double probability, const double lawlessness, const double cautiousness);
        void setSimulationSpeed(const double speed); // speed is a factor that is multiplied with normal time
        void setPlayerTransform(VRTransformPtr transform);

        void start();
        void pause();
        bool isRunning();

        void update();
        void tick();
        void updateScene();
        void updateViewAreas();

        void setCollisionHandler(bool (*handler) (Vehicle& a, Vehicle& b));
        void setVehiclePosition(const unsigned int id, const Vec3d& pos, const Vec3d& orientation);

        void constructRoadSystem(VRRoadNetworkPtr net);
};

OSG_END_NAMESPACE;

#endif
