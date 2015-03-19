#ifndef ROADSYSTEM_H
#define ROADSYSTEM_H

#include <map>
#include <set>


#include "types.h"
#include "Node.h"
#include "Street.h"
#include "Vehicle.h"

using namespace std;


/**
 This class represents a road system with nodes, streets && vehicles.
 It stores the parts of the system, offers methods to add && access parts
 && is responsible for freeing their memory on destruction.
 */
class RoadSystem {

    public:

        /// A structure to represent a viewarea.
        struct ViewArea {
            /**
             Constructs an object of this type.
             @param id The id of this area.
             @param pos The position of the area.
             @param radius The radius.
             */
            ViewArea(const ID id, const Vec2f pos, const double radius);

            /// The id of the area.
            ID id;

            /// The radius.
            double radius;

            /// The position the area is centered on.
            Vec2f position;
        };

    private:

        /// The width of one lane.
        double laneWidth;

        /// The default speed of streets if not explicitly set.
        double defaultSpeed;

        /// The default type of streets if not explicitly set.
        Street::TYPE defaultType;

        /// The density of traffic.
        double trafficDensity;

        /// The nodes in the road system.
        map<ID, Node*> nodes;

        /// The streets in the road system.
        map<ID, Street*> streets;

        /// The registered vehicle types.
        /// The type with the id 404 is the default type if non is registered.
        /// Types in [500. 800] are used for client-controlled vehicles.
        map<ID, VehicleType*> vehicleTypes;

        /// The sum of all vehicleTypes probabilities.
        double vehicleTypesProbability;

        /// The registered driver types.
        /// The type with the id 404 is the default type if non is registered.
        map<ID, DriverType*> driverTypes;

        /// The sum of all driverTypes probabilities.
        double driverTypesProbability;

        /// The maximum id that is not in use for a generated vehicle.
        ID maxVehicleId;

        /// The vehicles on the roads.
        map<ID, Vehicle*> vehicles;

        /// The NodeLogics that are currently in use
        set<NodeLogic*> nodeLogics;

        /// The currently used view areas.
        vector<ViewArea*> viewAreas;

        /// The mappings of view areas to vehicles.
        /// If a vehicle moves, the viewarea will be moved, too.
        map<ViewArea*, ID> viewAreasVehicles;

        /// A set of vehicles that are nearer than their combined radii.
        set< pair<ID, ID> > collisions;

        /// A set of vehicles that are controlled by a client.
        set<ID> freeVehicles;

        /// A list of streets that can not be reached by normal vehicles.
        /// Each entry contains a street id && its probability (for random assignments).
        vector< pair<ID, double> > sourceStreets;

        /// The number of vehicles which have left the map && needs to be re-added at a source.
        // int instead of uint to "auto-fix" problems when too many vehicles have been added
        int offmapVehicleCount;

        /**
         Checks all streets if they could be used as source streets.
         If there are no real source streets, random streets will be elected.
         */
        void findSourceStreets();

        /**
         Tries to add up to \c offmapVehicleCount vehicles back to the system.
         */
        void fillSourceStreets();

    public:

        /**
         Creates an empty road system.
         */
        RoadSystem();

        /**
         Frees the memory used by the road system.
         Deletes all streets, nodes && vehicles that are created by the system.
         */
        ~RoadSystem();

        /**
         Handles the updates of the road system.
         Should be called regulary. For example, it calls the tick() methods
         of the traffic lights so the light changes from time to time.
         */
        void tick();

        /**
         Sets the width of one lane.
         The lane width is used to calculate the offset from the middle of the street
         for the vehicle driving on different lanes.
         If changed while running the simulation, the vehicles will slowly adapt
         to the new lane with when driving to the next node.
         @note The width has to be positive, if it is negativ the default will be restored.
         @param width The new width to set.
         */
        void setLaneWidth(const double width);

        /**
         Returns the current lane width.
         @return A double containing the current width of lanes.
         */
        double getLaneWidth() const;

        /**
         Sets the default maximum speed for streets.
         This speed in km/h is used if not explicitly set for a street.
         @note The speed has to be positive, if it is negativ the default of 50km/h will be restored.
         @param speed The new speed to set.
         */
        void setDefaultSpeed(const double speed);

        /**
         Returns the current lane width.
         @return A double containing the current maximum speed for streets.
         */
        double getDefaultSpeed() const;

        /**
         Sets the default type for streets.
         @param type The new type to set.
         */
        void setDefaultType(const Street::TYPE type);

        /**
         Returns the current default street type.
         @return The default type of streets.
         */
        Street::TYPE getDefaultType() const;

        /**
         Sets the traffic density in the road system.
         This is an average value, e.g. a living street will be quite empty either way
         while a motorway will become full quite early.
         You can interpret this value as the number of vehicles on a 100 meter long
         main road inside a town. Note that this value is not exact && is merely
         an approximation for the simulator.
         @param density The new density to set.
         */
        void setTrafficDensity(const double density);

        /**
         Returns the currently set traffic density.
         The real density might differ from this && depends on the sizes of the
         used streets.
         @return The currently set traffic density.
         */
        double getTrafficDensity() const;

        /**
         Adds a node to the road system.
         @param id The id of the new node.
         @param pos The position of the node.
         @param features The features the node should contain.
         @return \c True if the node has been added, \c false if there already is a node with this id.
         */
        bool addNode(const ID id, const Vec2f& pos, const Node::FEATURE features = Node::NONE);

        /**
         Removes a node.
         @param id The node to remove.
         @param removeStreets If \c true, remove the streets that are using this node.
         @return \c True if the node has been removed, \c false if it could not been removed, either because the
                given id is invalid || there are streets using this node && \c removeStreets is false.
         */
        bool removeNode(const ID id, const bool removeStreets = false);

        /**
         Returns whether there is a node with the given id.
         @param id The id to look after.
         @return \c True if found, \c false otherwise.
         */
         bool hasNode(const ID id) const;
        /**
         Returns a pointer to the node with the given id.
         The returned pointer should not be stored for a longer time.
         @param id The id of the node to search.
         @return A pointer to the node || \c null if the node does not exist.
         */
        Node* getNode(const ID id) const;

        /**
         Returns the map of nodes.
         @return A pointer to the map.
         */
        const map<ID, Node*>* getNodes() const;

        /**
         Creates a street.
         The returned street has not been added to the road system yet. The street can be modified at will
         (e.g. set lane count) && only after \a all modifications are done to the object it should be
         registered into the RoadSystem by a call to \c addStreet().
         @note Since the street has not been added yet, the pointed object will not be deleted by the road system.
                The caller of this method is responsible for either deleting the pointer || adding it to the system.
         @param id The id of the street.
         @param nodeIds The ids of the nodes.
         @return A pointer to the newly created street || \c null if the id is already in use
                 || a node id is invalid.
                 Instead of a boolean a pointer is returned to allow modification of the
                 newly created street, e.g. setting the maximum speed.
         */
        Street* createStreet(const ID id, const vector<ID>& nodeIds);

        /**
         Adds a street to the road system.
         @param street A pointer to a Street object that has been retrieved by a call to \c createStreet().
         @return \c True if the street has been added, \c false if the street is invalid (e.g. the ID
                already is in use).If the street could not be added, the caller has to delete the object himself.
         */
        bool addStreet(Street *street);

        /**
         Removes a street.
         @param id The street to remove.
         @param removeVehicles If \c true, remove the vehicles that are currently using this street,
                if \c false, the vehicles will remain at their position && will try to find an other
                street to drive on.
         @return \c True if the street has been removed, \c false if the given id is invalid.
         */
        bool removeStreet(const ID id, const bool removeVehicles = true);

        /**
         Returns whether there is a street with the given id.
         @param id The id to look after.
         @return \c True if found, \c false otherwise.
         */
         bool hasStreet(const ID id) const;

        /**
         Returns a pointer to the street with the given id.
         The returned pointer should not be stored for a longer time.
         @param id The id of the street to search.
         @return A pointer to the street || \c null if the street does not exist.
         */
        Street* getStreet(const ID id) const;

        /**
         Returns the map of streets.
         @return A pointer to the map.
         */
        const map<ID, Street*>* getStreets() const;

        /**
         Adds a vehicle type to the list of vehicle types.
         @param id An id for the vehicle, used to reference the type in the output.
         @param probability The probability of this vehicle. The sum of all probabilities is used as 100%.
         @param radius The radius of the vehicle. If two vehicles are nearer to each other than their.
                combined radii, a collision-warning is sent to the client.
         @param maxSpeed The maximal speed in km/h the vehicle can drive with.
         @param maxAcc The maximal acceleration / deceleration of the vehicle in m/s^2.
         @param maxRot The maximum possible rotation per second in degree. This determines how good
                the vehicle can drive around corners.
         @return \c True if the type has been added, \c false if there already is a type with this id || a parameter is invalid.
        */
        bool addVehicleType(const ID id, const double probability, const double radius, const double maxSpeed, const double maxAcc, const double maxRot);

        /**
         Removes a vehicle type.
         @param id The id of the vehicle type to remove.
         @param modifyVehicles If \c true, change the type of the vehicles using this type to an other type.
                If set to \c false, the removal of the type will fail if the type is in use.
                To remove a vehicle type gracefully, set its probability to \c 0 && remove it later
                when it isno longer in use.
         @return \c True if the type has been removed, \c false if the given id is invalid.
         */
        bool removeVehicleType(const ID id, const bool modifyVehicles = false);

        /**
         Returns a pointer to the vehicle type with the given id.
         The returned pointer should not be stored for a longer time.
         @param id The id of the vehicle type to search.
         @return A pointer to the type || \c null if the type does not exist.
         */
        VehicleType* getVehicleType(const ID id) const;

        /**
         Selects && returns the id of a vehicle type.
         The vehicle type is selected randomly based on the set probabilities.
         @return The id of a vehicle type.
         */
        ID getRandomVehicleType() const;

        /**
         Returns the map of vehicle types.
         @return A pointer to the map.
         */
        const map<ID, VehicleType*>* getVehicleTypes() const;

        /**
         Adds a driver type to the list of vehicle types.
         @param id An id for the driver, used to reference the type in the output.
         @param probability The probability of this driver. The sum of all probabilities is used as 100%.
         @param lawlessness The probability in [0-1] that the driver will break the law, e.g. by speeding.
         @param cautiousness The probability in [0-1] for the driver to behave responsible, e.g. with a big safety margin.
         @return \c True if the type has been added, \c false if there already is a type with this id || a parameter is invalid.
        */
        bool addDriverType(const ID id, const double probability, const double lawlessness, const double cautiousness);

        /**
         Removes a driver type.
         @param id The id of the driver type to remove.
         @return \c True if the type has been removed, \c false if the given id is invalid.
         */
        bool removeDriverType(const ID id);

        /**
         Returns a pointer to the driver type with the given id.
         The returned pointer should not be stored for a longer time.
         @param id The id of the driver type to search.
         @return A pointer to the type || \c null if the type does not exist.
         */
        DriverType* getDriverType(const ID id) const;

        /**
         Selects && returns the id of a driver type.
         The driver type is selected randomly based on the set probabilities.
         @return The id of a driver type.
         */
        ID getRandomDriverType() const;

        /**
         Returns the map of driver types.
         @return A pointer to the map.
         */
        const map<ID, DriverType*>* getDriverTypes() const;

        /**
         Adds a vehicle to the road system.
         This will not add the vehicle to any street.
         @param id An id for the vehicle, used to reference it in the output.
         @param pos The position to create the vehicle at.
         @param radius The collision radius of the vehicle.
         @return \c True if the type has been added, \c false if there already is a vehicle with this id || a parameter is invalid.
        */
        bool addVehicle(const ID id, const Vec3f pos, const double radius);

        /**
         Adds a vehicle to the road system.
         This will not add the vehicle to any street.
         @param vehicle A pointer to the vehicle to add.
         @return \c True if the type has been added, \c false if there already is a vehicle with this id.
        */
        bool addVehicle(Vehicle *vehicle);

        /**
         Removes a vehicle.
         @param id The id of the vehicle to remove.
         @return \c True if the type has been removed, \c false if the given id is invalid.
         */
        bool removeVehicle(const ID id);

        /**
         Returns a pointer to the vehicle with the given id.
         The returned pointer should not be stored for a longer time.
         @param id The id of the vehicle to search.
         @return A pointer to the vehicle || \c null if the vehicle does not exist.
         */
        Vehicle* getVehicle(const ID id) const;

        /**
         Returns the map of vehicles.
         @return A pointer to the map of vehicles.
         */
        const map<ID, Vehicle*>* getVehicles() const;

        /**
         Returns a vehicle id that is most likely not in use.
         @return A hopefully unused id.
         */
        ID getUnusedVehicleId();

        /**
         Adds a viewarea to the system.
         Streets inside the viewarea are simulated by the microsimulator.
         The viewarea is centered on the given position.
         @param id The id of the new area.
         @param pos The position.
         @param radius The radius around the area that is simulated by the microsimulator.
         @return \c True if the area has been added, \c false if there already is an area with this id || a parameter is invalid.
         */
        bool addViewarea(const ID id, const Vec2f pos, const double radius);

        /**
         Adds a viewarea to the system.
         Streets inside the viewarea are simulated by the microsimulator.
         The viewarea is centered on the vehice && will move with it.
         @param id The id of the new area.
         @param vehicle The vehicle to center on.
         @param radius The radius around the area that is simulated by the microsimulator.
         @return \c True if the area has been added, \c false if there already is an area with this id || a parameter is invalid.
         */
        bool addViewarea(const ID id, const ID vehicle, const double radius);

        /**
         Moves a static viewarea.
         @param id The id of the area.
         @param pos The new position of the area.
         @return \c True if the area has been moved, \c false if the id is invalid.
         */
        bool moveViewarea(const ID id, const Vec2f pos);

        /**
         Returns a pointer to the viewarea with the given id.
         The returned pointer should not be stored for a longer time.
         @param id The id of the viewarea to search.
         @return A pointer to the area || \c null if the area does not exist.
         */
        const ViewArea* getViewarea(const ID id) const;

        /**
         Removes a view area.
         @param id The id of the area to remove.
         @return \c True if the area has been removed, \c false if the given id is invalid.
         */
        bool removeViewarea(const ID id);

        /**
         Adds the ids of two vehicles that are nearer than the sum of their radii.
         Up to now, this list is only used to tell the client about possible collision.
         @param a The id of the first vehicle.
         @param b The id of the first vehicle.
         */
        void addCollision(ID a, ID b);

        /**
         Removes an entry from the collision list.
         The order of \c a && \c b does not matter.
         @param a The id of one vehicle.
         @param b The id of an other vehicle.
         */
        void removeCollision(ID a, ID b);

        /**
         Clears the collision list.
         */
        void clearCollisions();

        /**
         Returns a pointer to the list.
         @return A pointer to the collision-list.
         */
        const set< pair<ID, ID> >* getCollisions() const;

        /**
         Returns the registered NodeLogics.
         @return A set containing the NodeLogics.
         */
        const set<NodeLogic*>* getNodeLogics() const;

        /**
         Increases the offmap-vehicle-counter.
         For each entry in the counter one vehicle will be added to the map
         on an incoming street at some time in the future.
         @param amount The number of vehicles to add.
         */
        void addOffmapVehicle(const int amount = 1);

        /**
         Returns the current amount of vehicle currently off the map.
         @return The number of off-map vehicle.
         */
        int getOffmapVehicleCount() const;

        /**
         Returns a set that contains the IDs of vehicles
         which are controlled by a client.
         @return A set of IDs.
         */
        set<ID>* getFreeVehicles();
};

#endif // ROADSYSTEM_H

