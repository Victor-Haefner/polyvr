#ifndef TRAFFICSIMULATION_H
#define TRAFFICSIMULATION_H

#include "core/objects/VRObjectFwd.h"
#include "core/utils/VRFunctionFwd.h"
#include "JsonClient.h"

#include <set>
#include <map>
#include <boost/thread/mutex.hpp>
#include <OpenSG/OSGVector.h>

class OSMMap;
class OSMNode;
class OSMWay;

OSG_BEGIN_NAMESPACE;
using namespace std;
using namespace boost;

class VRThread;
class MapCoordinator;

/// @addtogroup module_traffic
/// @{

/**
 * Simulates && draws traffic in the world.
 * @note Since this class communicates with an extern process over a network connection,
 *       most of its method are quite slow.
 * @todo The drawing code is missing completely.
 */
class TrafficSimulation {
    private:
        VRThreadCbPtr threadFkt;

    public:

        /**
         * A structure that is given to the collision handler to describe a vehicle.
         * Also used internally to display the vehicles.
         */
        struct Vehicle {

            /**
             * The id of the vehicle.
             * Can be used in \c setVehiclePosition() to set the position if a crash has occurred.
             */
            unsigned int id;

            /**
             * The id of the type of the vehicle
             * This is the same as passed to \c addVehicleType().
             */
            unsigned int vehicleTypeId;

            /**
             * The id of the type of the vehicle
             * This is the same as passed to \c addVehicleType().
             */
            unsigned int driverTypeId;

            /**
             * The position of the vehicle.
             */
            Vec3f pos;

            /**
             * The movement which is added to the vehicle every second.
             */
            Vec3f deltaPos;

            /**
             * The orientation of the vehicle.
             */
            Vec3f orientation;

            /**
             * The rotation which is added to the orientation every second.
             */
            Vec3f deltaOrientation;

            /**
             * Possible state-flags for this vehicle.
             * To check for them, use a bit-wise "and".
             */
            typedef uint8_t VehicleState;
            /// No special state, check for equality.
            static const VehicleState NONE            = 0;
            /// The right indicator is blinking.
            static const VehicleState RIGHT_INDICATOR = 1 << 0;
            /// The left indicator is blinking.
            static const VehicleState LEFT_INDICATOR  = 1 << 1;
            /// The vehicle is accelerating.
            static const VehicleState ACCELERATING    = 1 << 2;
            /// The vehicle is waiting for something, e.g. for a red traffic light.
            static const VehicleState WAITING         = 1 << 3;
            /// The vehicles path is blocked by another vehicle.
            static const VehicleState BLOCKED         = 1 << 4;
            /// The vehicle is braking (note: This is active breaking as opposed to only decelerating, e.g. the stoplight is on).
            static const VehicleState BRAKING         = 1 << 5;
            /// The vehicle has collided in the past && is not moved by the simulator at the moment.
            static const VehicleState COLLIDED        = 1 << 6;

            /**
             The current state of this vehicle.
             */
            VehicleState state;

            /**
             * The model used to display this vehicle.
             */
            VRTransformPtr model;
        };

    private:

        VRMaterialPtr a_red = 0;
        VRMaterialPtr a_orange = 0;
        VRMaterialPtr a_green = 0;

        /**
         * The collision radius of the user controlled vehicle.
         */
        double driverVehicleRadius;

        /**
         * Set of pointers to currently loaded maps.
         * If a map should be added/removed, its pointer is first looked after in this
         * set && only if (not) found, the simulator is notified.
         */
        set<const OSMMap*> loadedMaps;

        /**
         * The handler that is called if there might be a collision
         * between two vehicles.
         */
        bool (*collisionHandler) (Vehicle& a, Vehicle& b);

        /**
         * Handles the network connection to the server.
         */
        JsonClient client;

        /**
         * The MapCoordinator used to convert GPS-coordinates to pixels.
         */
        MapCoordinator *mapCoordinator;

        /**
         * The maximum distance to draw vehicle in.
         * This attribute is only needed once if the drawing distance is
         * set before the driver position is specified.
         */
        double viewDistance;

        /**
         * The VRTransform that describes the position of the player vehicle.
         */
        VRTransformPtr player;

        /**
         * Whether the vehicle of the player && the viewarea around it has
         * been created inside the simulation.
         */
        bool playerCreated;

        /**
         * The ID of the communication thread.
         * Needed to stop the thread when destroying this object.
         */
        int communicationThreadId;

        /**
         * A mutex to block access to the data that is transfered to && from the network.
         */
        mutex networkDataMutex;

        /**
         * The JSON-data that has been received by the last query for the view area.
         */
        Value receivedData;

        /**
         * The data that will be send to the server shortly.
         */
        Value dataToSend;

        /**
         * A map to store the meshes for the vehicle types.
         */
        map<unsigned int, VRTransformPtr> meshes;

        /**
         * A map that stores the ids && vehicles that are moved around by the simulator.
         */
        map<unsigned int, Vehicle> vehicles;

        /**
         * A vector containing VRGeometry objects used to represent the traffic light phases.
         * @todo Make something prettier.
         */
        vector<VRGeometryPtr> lightBulbs;

        /**
         * A flag to suppress constant error output if there is no connection.
         */
        bool noConnection;

        /**
         Keeps some vehicle geometry on stock since it can not be created in the thread.
         */
        //map<unsigned int, vector<VRGeometryPtr> > geometryCache;

        /**
         * Converts an OSM node to a JSON representation.
         * @param node The node to convert
         * @return A JSON value that contains data from the node.
         */
        Value convertNode(OSMNode *node);

        /**
         * Converts an OSM street to a JSON representation.
         * @param street The street to convert
         * @return A JSON value that contains data from the street.
         */
        Value convertStreet(OSMWay *street);

        /**
         A helper method to fetch the data from the server.
         Will be called repetitive while the simulator is running.
         */
        void communicationThread(std::weak_ptr<VRThread> t);

        /**
         * A mutex that will be locked if the communication thread is running.
         * Is used to have a join() functionality for the threads.
         */
        mutex communicationThreadMutex;

        /**
         * Prints an error message if an error occurred.
         * Checks the given \c Value if it contains an error message.
         * If it does, a warning will be printed.
         * @param action The action which has been done. Will be printed in the error
         *     message as "... error while doing <action> ... ".
         * @param value The result from the server to check.
         * @return \c true if an error occurred, \c false otherwise.
         */
        bool errorMessage(const string& action, const Value& value);

    public:

        /// @name Construction && initialization
        /// @{

        /**
         * Creates an object of this class.
         * Initial, the simulator does not have any streets || vehicles.
         * The simulation starts paused && has to be continued with \c start().
         * @param mapCoordinator A MapCoordinator used to convert GPS-coordinates to pixels.
         * @param host The name of the host the traffic simulation server is running on. The port number can be appended as "address:port".
         */
        TrafficSimulation(MapCoordinator *mapCoordinator, const string& host = "localhost:5550");

        /**
         * The destructor.
         */
        ~TrafficSimulation();

        /**
         * Sets the server to connect to.
         * @param host The name of the host the traffic simulation server is running on. The port number can be appended as "address:port".
         */
        void setServer(const string& host);

        /**
         * Adds a map consisting of streets that should be simulated.
         * @param map The map to simulate.
         */
        void addMap(const OSMMap* map);

        /**
         * Removes a map from the simulator.
         * All vehicles on this map will be deleted && no further vehicles will drive on these streets.
         * @param map The map to delete.
         */
        void removeMap(const OSMMap* map);

        /**
         * Sets the distance vehicles can have to the player && still be drawn.
         * Default is 200.
         * @param distance The maximal distance to draw a vehicle at.
         */
        void setDrawingDistance(const double distance);

        /**
         * Set the density of the traffic.
         * The real traffic density will vary based on the type of particular street.
         * You can interpret this value as the number of vehicles on a 100m norm-street,
         * e.g. 20 will be quite stuffed, 0.1 is nearly empty.
         * @note This implies that if you add streets to the street network the overall number of vehicles increases.
         * @param density The non-negative traffic density. Determines the number of vehicles on the streets.
         */
        void setTrafficDensity(const double density);

        /**
         * Adds a vehicle type to the simulation.
         * These influence the physical abilities of a vehicle.
         * If no vehicle types are added, random ones will be created.
         * @param id The id of this vehicle type.
         * @param probability The probability that this vehicle is choosen. The sum of all probabilities is used as 100%.
         * @param collisionRadius The size of this vehicle if represented as a sphere. If two vehicles are nearer than the sum of their radii, the collision-callback will be called.
         * @param maxSpeed The maximum speed the vehicle can drive in km/h.
         * @param maxAcceleration The maximum acceleration of this vehicle in m/s^2.
         * @param maxRotation The maximum rotation in degree per second.
         * @param geometry The geometry to use to draw a vehicle of this type.
         */
        void addVehicleType(const unsigned int id, const double probability, const double collisionRadius, const double maxSpeed, const double maxAcceleration, const double maxRoration, VRTransformPtr model);

        /**
         * Adds a driver type to the simulation.
         * These influence the behaviour of the driver of a vehicle.
         * If no driver types are added, random ones will be created.
         * @param id The id of this driver type.
         * @param probability  The probability that this driver is choosen. The sum of all probabilities is used as 100%.
         * @param lawlessness  The probability from 0.0 to 1.0 that the driver will break the law, e.g. speeding.
         * @param cautiousness The probability from 0.0 to 1.0 that the driver will behave responsible, e.g. greater values result in a bigger safety margin.
         */
        void addDriverType(const unsigned int id, const double probability, const double lawlessness, const double cautiousness);

        /**
         Sets the speed of the simulation.
         This affects the speed of the vehicles.
         The given speed is a factor that is multiplied with the time,
         e.g. a value of 20 means the simulation time runs 20 times as fast as the normal time.
         @param speed The speed factor to set.
         */
        void setSimulationSpeed(const double speed);

        /**
         * Sets the transformation that describes the position of the user vehicle.
         * The simulated cars are only drawn in an area around this object.
         * @param transform The vehicle of the user. Can be \c NULL to unset it.
         */
        void setPlayerTransform(VRTransformPtr transform);

        /// @}
        /// @name Run control
        /// @{

        /**
         * Continues a paused simulation.
         * @note On startup the simulation is paused.
         */
        void start();

        /**
         * Pauses the simulator.
         */
        void pause();

        /**
         * Returns if the simulator is running.
         * @note Even if no streets || vehicles have been added yet, this can return \c true.
         * @return \c True if the simulator is running at the moment, \c false otherwise.
         */
        bool isRunning();

        /**
         * Updates the vehicle positions.
         * Moves all active vehicles a bit depending on their current speed && direction.
         */
        void update();
        void tick();
        void updateScene();

        /// @}
        /// @name Vehicle interactions
        /// @{

        /**
         * Sets the collision handler.
         * The set function is called if the distance between two vehicles is less than the sum of their radii.
         * The handler receives two \c Vehicle structures for the two vehicles which might have collided. If the handler
         * returns \c true, a collision has occurred && the vehicles are not moved for some time. If the handler
         * returns \c false, no collision has occurred && the vehicles continue driving.
         *
         * @warning The handler will be called in a separate thread by the simulator,
         *     so you should not access outside data structures from it.
         *
         * @note The given structures might be modified but this will only change their current representation
         *     until the next update by the simulator. If you want to achieve longer lasting results,
         *     you have to use \c setVehiclePosition(). An exception to this is the model used by the vehicle,
         *     this will not change until the vehicle leaves the viewarea.
         * @param handler The handler to call. If set to \c NULL, vehicles will not collide.
         */
        void setCollisionHandler(bool (*handler) (Vehicle& a, Vehicle& b));

        /**
         * Sets the position of a vehicle.
         * This method might be used to move two vehicles around after they have crashed.
         * @param id The id of the vehicle to move.
         * @param pos The position to move to.
         * @param orientation The new orientation of the vehicle.
         */
        void setVehiclePosition(const unsigned int id, const Vec3f& pos, const Vec3f& orientation);

        /// @}
};

/// @}

OSG_END_NAMESPACE;

#endif
