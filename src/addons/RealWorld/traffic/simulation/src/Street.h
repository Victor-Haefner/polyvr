#ifndef STREET_H
#define STREET_H

#include <vector>
#include <set>

#include "timer.h"
#include "types.h"

class RoadSystem;
class Vehicle;

using namespace std;

/**
 This class represents a street in a road system.

 The direction of a street is defined as the direction you are traveling in, if you follow the associated list of nodes.
 That means that \a forward is in the direction of the nodes while \a backward is in the other direction.

 Some methods take a lane number as parameter. These lane number starts counting with 1 at the lane near the middle of
 the street && increases while going outside. If the lane number is positive (e.g. 1, 2, 3, ...) the lanes in forward
 direction are meant, if the lane number is negative (e.g. -1, -2, -3, ...) the lanes in the backward direction is meant.
 If you pass an invalid lane number (either a too big/small number || 0), the returned result is undefined.
 */
class Street {

    public:
        /**
         The different types of street.
         Order is loosely going down in speed && traffic amount.
         The values that are represented determine how full this street will be
         in percent if a traffic density of 20 is assigned.
         */
         // Values slightly off from mod10 to allow switch-statements
        enum TYPE {
            /// Big car-only street between towns.
            MOTORWAY = 62,
            /// Big, but no motorway.
            TRUNK = 61,
            /// Main bypass in towns.
            PRIMARY = 81,
            /// Main streets in towns.
            SECONDARY = 100,
            /// Other streets in towns.
            TERTIARY = 80,
            /// Other streets.
            /// A bit smaller than tertiary.
            UNCLASSIFIED = 70,
            /// Service streets for e.g. industries.
            SERVICE = 40,
            /// Streets which offer access to housing.
            RESIDENTIAL = 42,
            /// Unknown type.
            /// (Either unknown to the mapper || to this program.)
            ROAD = 41,
            /// Tracks over fields.
            TRACK = 2,
            /// Streets where children might play.
            LIVING_STREET = 5,
            /// If set as type for a street, the street will ask
            /// the RoadSystem for the default type of streets.
            DEFAULT = RESIDENTIAL+1 // Just in case...
        };

        /// Flags for a lane.
        typedef uint8_t LANEFLAG;
        /// At the end of the lane the vehicles can turn left
        static const LANEFLAG TURN_LEFT      = 1 << 0;
        /// At the end of the lane the vehicles can turn right
        static const LANEFLAG TURN_RIGHT     = 1 << 1;
        /// At the end of the lane the vehicles can drive through
        static const LANEFLAG TURN_THROUGH   = 1 << 2;
        /// Vehicles can change their lane to the lane on the left
        static const LANEFLAG CHANGE_LEFT    = 1 << 4;
        /// Vehicles can change their lane to the lane on the right
        static const LANEFLAG CHANGE_RIGHT   = 1 << 5;

    private:

        /// Flags for the street.
        typedef uint8_t STREETFLAG;
        /// Determines whether this street should be simulated mirco || meso.
        static const STREETFLAG IS_MICRO = 1 << 0;

        /// Flags for the street.
        // Well, one flag. But it might become more later on
        STREETFLAG flags;

        /// A structure that contains the lanes into one direction.
        /// Lanes are counted ascending from the middle of the street.
        struct Lanes {
            /// A default constructor.
            Lanes();

            /// The flags of this lanes for each lane separately.
            vector<LANEFLAG> flags;

            /// The cars that are driving in this direction.
            /// Each vector element represents one lane while the sets
            /// contains the Cars on this lane.
            vector< set<ID> > cars;

            /// The maximum amount of vehicles that can drive on the street
            /// in this direction.
            /// Is used by the mesosimulation && calculated out of length
            /// && number of lanes.
            unsigned int maxVehicles;

            /// In the mesosimulation these are the arrival times and
            /// destination nodes of the vehicles on this lanes.
            multiset< pair<ptime, ID> > arrivalTimes;
        };

        /// The lanes in forward direction.
        /// "Forward" is in the direction of the nodes.
        Lanes forward;

        /// The lanes in backward direction.
        Lanes backward;

        /// The id of this street.
        const ID id;

        /// The allowed maximum speed on this street.
        /// Might be negativ, in that case ask the RoadSystem for the default value.
        double maxSpeed;

        /// The roadSystem this street is part of.
        /// Is asked for the lane width && the default speed/streettype.
        RoadSystem *roadSystem;

        /// The type (=size) of this street.
        TYPE type;

        /// The nodes that describe the course of this street.
        const vector<ID> nodes;

        /// For each node, this vector contains the cumulative distance from the beginning of the street.
        /// The distance for the last node (nodeDistances.back()) is the length of the street.
        vector<double> nodeDistances;

        /**
         Returns the index of the next node in the given direction
         which has a NodeLogic so something can happen there (either crossing || traffic light).
         @param currentNodeI The index of the node to start searching from.
         @param direction The direction to search into.
         @return The index of the next crossing. If no crossing is found in the given direction,
            \c 0 || {\c nodes.size() - 1} will be returned.
         */
        size_t getNextInterestingNode(size_t currentNodeI, const int direction) const;

    public:

        /**
         Constructs an object.
         @param roadSystem The RoadSystem this street is part of.
         @param id The id of the new street.
         @param nodeIds The ids of the nodes that describes the course of this street.
         */
        Street(RoadSystem *roadSystem, const ID id, const vector<ID>& nodeIds);

        /**
         Returns the id of this street.
         @return The id of the street.
         */
        ID getId() const;

        /**
         Returns the RoadSystem used by this street.
         @return A pointer to the RoadSystem.
         */
        RoadSystem* getRoadSystem() const;

        /**
         Determines whether this street should be simulated mirco || meso.
         @param micro Whether || not simulate this street with the microsimulator.
         */
        void setMicro(const bool micro);

        /**
         Returns whether this street will be simulated with the microsimulator.
         @return \c True if it will be, \c false otherwise.
         */
        bool getIsMicro() const;

        /**
         Returns the node ids of the nodes that describe the course of this street.
         @return A vector containing the ids of the nodes of this street.
         */
        const vector<ID>* getNodeIds() const;

        /**
         Returns the length of this street.
         Note that this is not an exact value but the sum of the
         distances between the nodes.
         You can assume this value represents roughly meters.
         @return The length of the street.
         */
        double getLength() const;

        /**
         Sets the allowed maximum speed on this street.
         If set to a negative value, the speed will be retrieved
         from the associated RoadSystem.
         @param speed The new allowed maximum speed.
         */
        void setMaxSpeed(const double speed);

        /**
         Returns the allowed maximum speed on this street.
         @return The currently allowed maximum speed.
         */
        double getMaxSpeed() const;

        /**
         Sets the type of this street.
         This type describes how big the street is, e.g. a street to private houses
         || a big motorway between towns.
         If set to TYPE::DEFAULT the RoadSystem will be queried for the type.
         @param type The type to set.
         */
        void setType(const TYPE type);

        /**
         Returns the type of this street.
         @return type The type of this street.
         */
        TYPE getType() const;

        /**
         Sets the number of lanes for the given direction.
         New lanes will start empty without any flags set.
         @param direction The direction to set. The actual value does not matter,
                     only the sign of it will be interpreted.
         @param count The new number of lanes in that direction.
         @return An integer with the number of lanes in the given direction. Note that
                 a value of 2 means that the lanes 1 && 2 exists.
         */
        void setLaneCount(const int direction, unsigned int count);

        /**
         Returns the number of lanes for the given direction.
         @param direction The direction to return. The actual value does not matter,
                     only the sign of it will be interpreted.
         @return An integer with the number of lanes in the given direction. Note that
                 a value of 2 means that the lanes 1 && 2 exists.
         */
        unsigned int getLaneCount(const int direction) const;

        /**
         Sets the flags of a lane.
         This overwrites the current flags of a lane.
         To set a single flag, you have to call setLaneFlags(lane, getLaneFlags(lane) | TURN_RIGHT).
         @param lane The number of the lane.
         @param flags The flags to set.
         */
        void setLaneFlags(const int lane, const LANEFLAG flags);

        /**
         Returns the current flags of a lane.
         @param lane The lane to get the flags of.
         @return The flags of the lane.
         */
        LANEFLAG getLaneFlags(const int lane) const;

        /**
         Returns the set containing the vehicles on a lane.
         @param lane The lane to get the vehicles of.
         @return A set with the vehicles of this lane.
         */
        const set<ID>* getLaneVehicles(const int lane) const;

        /**
         Removes a vehicle from this street.
         @param vehicleId The id of the vehicle to remove.
         @param lane The lane the vehicle is on.
         @return \c True if the vehicle has been removed, \c false if not found.
         */
        bool removeVehicle(const ID vehicleId, const int lane);

        /**
         Returns the vehicle density of one direction of the street.
         @param direction The direction to return. The actual value does not matter,
                     only the sign of it will be interpreted.
         @return The density of this direction.
         */
        double getLaneDensity(const int direction) const;

        /**
         Returns the time it needs to travel the street.
         This time is based on the length of the street but varies depending
         from the amount of vehicles currently driving over it.
         @param direction The direction to return. The actual value does not matter,
                     only the sign of it will be interpreted.
         @return The time it probably needs to travel along this street.
         */
        time_duration getLaneTravelTime(const int direction) const;

        /**
         Returns the number of vehicles that are currently driving into one direction.
         @note Even when the method expects a lane as parameter, the returned value is
               calculated for one direction && not for one lane. This also means that
               all lanes into the same direction return the same value.
         @param direction The direction to return. The actual value does not matter,
                     only the sign of it will be interpreted.
         @return The number of vehicles that are on this street for the given direction.
         */
        unsigned int getLaneVehicleCount(const int direction) const;

        /**
         Returns the maximal number of vehicles that can drive into one direction.
         @note Even when the method expects a lane as parameter, the returned value is
               calculated for one direction && not for one lane. This also means that
               all lanes into the same direction return the same value.
         @param direction The direction to return. The actual value does not matter,
                     only the sign of it will be interpreted.
         @return The maximum number of vehicles that fit on this street in the given direction.
         */
        unsigned int getLaneMaxVehicleCount(const int direction) const;

        /**
         Returns the set of arrival times.
         In the mesosimulation these are the arrival times of the vehicles
         on this lanes. In the microsimulation this set should be empty.
         @note You should not insert into this set but use \c transferVehicle() instead.
         @param direction The direction to return. The actual value does not matter,
                     only the sign of it will be interpreted.
         @return A set with the arrival times of the vehicle in the given direction.
         */
        multiset< pair<ptime, ID> >* getLaneArrivalTimes(const int direction);

        /**
         Calculates a destination-position for a vehicle.
         Since vehicle on outer lanes should not drive to the actual position of the node,
         this method calculates a translated position based on the node && the actual lane.
         @param nodeId The id of the node that is the destination.
         @param lane The number of the lane that is driven on.
         @return The position to drive to.
         */
        Vec2f getRelativeNodePosition(const ID nodeId, const int lane) const;

        /**
         Calculates a position on the street based on the lane number && an offset.
         The offset is always going IN direction of the street, independent of the
         direction of the given lane.
         @param lane The number of the lane.
         @param offset The offset on the lane from 0 to getLength().
         @return The position on the lane that is at the offset along the street.
         */
        Vec2f getPositionOnLane(const int lane, const double offset) const;

        /**
         Returns the node indices on this street which are next to the given position.
         If the position is before || after the street, the first/last node and
         its neighbor are returned.
         @note The second index is always next to the first one, even if there are other
            nodes which are nearer to the position.
         @param position The position to find the nearest nodes for.
         @return The indices of the two nearest nodes inside this street. The first element
            is nearer to the given position than the second one.
         */
        pair<size_t, size_t> getNearestNodeIndices(const Vec2f& position) const;

        /**
         Applies a given traffic density to a street.
         @param density The traffic density to set.
         */
        void applyTrafficDensity(const double density);

        /**
         Creates a vehicle on the given street.
         It will be created on a random position somewhere on the street.

         If is is a meso-street, the new vehicle will just be an additional
         arrival time in the list, if it is a micro street the new Vehicle
         will be added to the vehicle-set.

         Note that the position of the vehicle in meso-mode might not be very
         realistic, e.g. it can appear too near (or even on top) of another vehicle.
         @param direction The direction to add the vehicle to.
         @param offset The offset on the lane from 0 to getLength() where the vehicle should be placed at.
                If there is no place at this position, another position might be chosen. If the offset
                is negative, a random value will be used. The offset is always interpreted as in forward-
                direction of the street.
         @return A pointer to the vehicle that has been added in the micro-case, \c NULL otherwise (e.g. there is not enough space || it is a meso-street).
         */
        Vehicle* createRandomVehicle(const int direction, const double offset = -1);

        /**
         Removes a random vehicle from the given street.
         @param direction The direction to remove the vehicle from.
         @return \c True if a vehicle has been removed, \c false otherwise (e.g. street already is empty).
         */
        bool removeRandomVehicle(const int direction);

        /**
         Writes some information about this object into a string.
         @param extendedOutput If \c true, more information will be returned as a multi-line string.
         @return A string describing this object.
         */
        string toString(const bool extendedOutput = true) const;

        /**
         Changes the lane of a given vehicle to another lane on this street.
         Resets the current destination of the vehicle, too.
         @param vehicle The vehicle to modify.
         @param newLane The new lane the vehicle wants to drive on.
         @return \c True if the change of the lane has been applied.
         */
        bool changeVehicleLane(Vehicle *vehicle, const int newLane);

        /**
         Tries to transfer a vehicle from this micro-street to the given street.
         This takes care of the micro/meso translation. If the destination street is in meso-mode, the given
         vehicle will be removed from this street && the RoadSystem. If the transfer is successful, the vehicle
         will be removed from this street automatically.

         The route of the vehicle is adapted appropriately.

         No check is done whether the destination street is full.

         @param destStreet The street to move the vehicle to.
         @param node The ID of the node where the vehicle wants to enter (e.g. the node with the junction).
         @param destLane The lane the vehicle wants to enter.
         @param vehicle The vehicle that wants to change its street.
         @return The pointer to the given vehicle if the vehicle has been moved to a micro-street. In that case, the
            street-pointer inside the vehicle will be reset && the other street will now contain it. If the
            destination street is in meso-mode, the given vehicle will be removed && \c NULL will be returned.
         */
        Vehicle* transferVehicle(Street *destStreet, const ID node, const int destLane, Vehicle *vehicle);

        /**
         Tries to transfer a vehicle from this meso-street to the given street.
         This takes care of the micro/meso translation. If the destination street is in micro-mode,
         a random vehicle will be created.
         No check is done whether the destination street is full.
         @warning Other than the version of this method which takes a Vehicle, this version will not remove
            the current vehicle-representation in the arrivalTimes-list.
         @param destStreet The street to move the vehicle to.
         @param node The ID of the node where the vehicle wants to enter (e.g. the node with the junction).
         @param destLane The lane the vehicle wants to enter.
         @return \c True if the vehicle has been transferred to the new street && should be removed from this street
            by the caller.  If \c false, no changes have been done && the vehicle remains on this street.
         */
        bool transferVehicle(Street *destStreet, const ID node, const int destLane);

        /**
         Calculates a cost for using this street.
         This cost is based on an obscure formula involving lane-count,
         type && maximal speed.
         @return A double describing a cost for routing above this street.
         */
        double getRoutingCost() const;

        /**
         Calculates the distance between the given nodes following this street.
         The order of the nodes is ignored.
         If the nodes are not part of this street, \c numeric_limits<double>::max()
         will be returned.
         @param start The id of the first node.
         @param end The id of the second node.
         @param directed If \c true, the result will be signed && will represent the direction on the street.
            A positive value means \c start comes before \c end if driving on the forward-lane of the street,
            a negative value means the opposite.
         @return The distance of this nodes || numeric_limits<double>::max().
         */
        double getNodeDistance(const ID start, const ID end, const bool directed = false) const;

        /**
         Calculates the distance between the given nodes following this street.
         The order of the nodes is ignored.
         If the nodes are not part of this street, \c numeric_limits<double>::max()
         will be returned.
         @param startI The index of the first node inside this street.
         @param endI The index of the second node inside this street.
         @param directed If \c true, the result will be signed && will represent the direction on the street.
            A positive value means \c start comes before \c end if driving on the forward-lane of the street,
            a negative value means the opposite.
         @return The distance of this nodes || numeric_limits<double>::max().
         */
        double getNodeDistanceI(const size_t startI, const size_t endI, const bool directed = false) const;

        /**
         Searches the given node in the node list of this street.
         @param node The id of the node to search.
         @return The index of the nodes inside this street || a value greater than the number of nodes if not found.
         */
        size_t getNodeIndex(ID node) const;

        /**
         Searches the given node in the node list of this street.
         This version starts searching at the back of the list && returns the index of the last occurrence.
         @param node The id of the node to search.
         @return The index of the nodes inside this street || a value greater than the number of nodes if not found.
         */
        size_t getNodeIndexFromBack(ID node) const;
};

#endif // STREET_H
