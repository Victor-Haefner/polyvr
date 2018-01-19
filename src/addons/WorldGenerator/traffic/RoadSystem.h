#ifndef ROADSYSTEM_H
#define ROADSYSTEM_H

#include <map>
#include <set>

#include "types.h"
#include "Node.h"
#include "Street.h"
#include "Vehicle.h"

using namespace std;

class RoadSystem {
    public:
        struct ViewArea { // circular area
            ID id;
            double radius;
            Vec2d position;

            ViewArea(const ID id, const Vec2d pos, const double radius);
        };

    private:

        double laneWidth;
        double defaultSpeed;
        Street::TYPE defaultType;
        double trafficDensity;

        map<ID, RSNode*> nodes;
        map<ID, Street*> streets;
        map<ID, VehicleType*> vehicleTypes; // default is 404, the range [500-800] is reserved for client-controlled vehicles
        map<ID, DriverType*> driverTypes; // 404 is the default type

        double vehicleTypesProbability; // The sum of all vehicleTypes probabilities.
        double driverTypesProbability; // The sum of all driverTypes probabilities.

        ID maxVehicleId; // The maximum id that is not in use for a generated vehicle.
        map<ID, Vehicle*> vehicles;
        set<NodeLogic*> nodeLogics;
        vector<ViewArea*> viewAreas;
        map<ViewArea*, ID> viewAreasVehicles; // The mappings of view areas to vehicles. If a vehicle moves, the viewarea will be moved, too.
        set< pair<ID, ID> > collisions; // A set of vehicles that are nearer than their combined radii.
        set<ID> freeVehicles; // A set of vehicles that are controlled by a client.
        vector< pair<ID, double> > sourceStreets; // A list of streets that can not be reached by normal vehicles. street id and its probability

        int offmapVehicleCount; // The number of vehicles which have left the map and needs to be re-added at a source.
        void findSourceStreets();
        void fillSourceStreets(); //Tries to add up to offmapVehicleCount vehicles back to the system.

    public:
        RoadSystem();
        ~RoadSystem();

        void tick();

        void setLaneWidth(const double width);
        void setDefaultSpeed(const double speed);
        double getLaneWidth() const;
        double getDefaultSpeed() const;

        void setDefaultType(const Street::TYPE type);
        Street::TYPE getDefaultType() const;

        void setTrafficDensity(const double density); // number of vehicles on a 100 meter long main road
        double getTrafficDensity() const;

        bool addNode(const ID id, const Vec2d& pos, const RSNode::FEATURE features = RSNode::NONE);
        bool removeNode(const ID id, const bool removeStreets = false);
        bool hasNode(const ID id) const;
        RSNode* getNode(const ID id) const;
        const map<ID, RSNode*>* getNodes() const;

        Street* createStreet(const ID id, const vector<ID>& nodeIds);
        bool addStreet(Street *street);
        bool removeStreet(const ID id, const bool removeVehicles = true);
        bool hasStreet(const ID id) const;
        Street* getStreet(const ID id) const;
        const map<ID, Street*>* getStreets() const;

        bool addVehicleType(const ID id, const double probability, const double radius, const double maxSpeed, const double maxAcc, const double maxRot);
        bool removeVehicleType(const ID id, const bool modifyVehicles = false);
        VehicleType* getVehicleType(const ID id) const;
        ID getRandomVehicleType() const;
        const map<ID, VehicleType*>* getVehicleTypes() const;

        bool addDriverType(const ID id, const double probability, const double lawlessness, const double cautiousness);
        bool removeDriverType(const ID id);
        DriverType* getDriverType(const ID id) const;
        ID getRandomDriverType() const;
        const map<ID, DriverType*>* getDriverTypes() const;

        bool addVehicle(const ID id, const Vec3d pos, const double radius);
        bool addVehicle(Vehicle *vehicle);
        bool removeVehicle(const ID id);
        Vehicle* getVehicle(const ID id) const;
        const map<ID, Vehicle*>* getVehicles() const;
        ID getUnusedVehicleId();

        bool addViewarea(const ID id, const Vec2d pos, const double radius);
        bool addViewarea(const ID id, const ID vehicle, const double radius);
        bool moveViewarea(const ID id, const Vec2d pos);
        const ViewArea* getViewarea(const ID id) const;
        bool removeViewarea(const ID id);

        void addCollision(ID a, ID b);
        void removeCollision(ID a, ID b);
        void clearCollisions();
        const set< pair<ID, ID> >* getCollisions() const;

        const set<NodeLogic*>* getNodeLogics() const;
        void addOffmapVehicle(const int amount = 1);
        int getOffmapVehicleCount() const;
        set<ID>* getFreeVehicles();
};

#endif // ROADSYSTEM_H

