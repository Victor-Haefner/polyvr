#ifndef NODE_H
#define NODE_H

#include <vector>
#include <set>
#include <string>

#include "types.h"
#include "Street.h"

class NodeLogic;
class RoadSystem;

using namespace std;


/**
 Represents a point in the road system.
 This point might have an abitrary number of streets connected to it.
 */
class Node {

    public:
        /// The features a node might have.
        typedef uint8_t FEATURE;
        /// No features, test for equality.
        static const FEATURE NONE = 0;
        /// The node contains traffic lights, test bit wise.
        static const FEATURE TRAFFICLIGHTS = 1 << 0;
        //static const FEATURE TRASHBIN = 1 << 1;
        // Don't worry, just joking

    private:

        /// A structure to sort the connected streets to directions.
        struct Connection {
            /// The id of the street.
            ID street;
            /// The next node in this direction.
            ID node;
            /// A type for the Connection flags.
            typedef uint8_t FLAG;
            /// Whether vehicles on this lane can come from the node.
            static const FLAG INCOMING = 1 << 0;
            /// Whether vehicles on this lane can drive to the node.
            static const FLAG OUTGOING  = 1 << 1;
            /// Whether the incoming direction is in forward || backward direction of the street.
            static const FLAG FORWARD   = 1 << 2;
            /// Bit reserved for future use.
            static const FLAG RESERVED3 = 1 << 3;
            /// A set of bitwise flags. The upper 4 bits can be used by the logic.
            FLAG flags;
            /// The degree of this lane if 0° is north, clockwise.
            double degree;
        };

        /**
         A helper method to compare two Connection objects by their degree.
         @param lhs The first object to compare.
         @param rhs The second object to compare.
         @return \c True if \c lhs is less than \c rhs.
         */
        static bool connectionComparator(const Connection& lhs, const Connection& rhs);

        /// The id of this node.
        const ID id;

        /// The position of the node.
        const Vec2f pos;

        /// The ids of the street connected to the node.
        vector<ID> streetIds;

        /// The lanes that are connected to this node.
        multiset<Connection, bool(*)(const Connection& lhs, const Connection& rhs)> connections;

        /// The NodeLogic that controls passage through this node.
        NodeLogic *logic;

        /// The features that are present at this location.
        FEATURE features;

        /// The RoadSystem this node is part of.
        const RoadSystem *roadSystem;

        /// The ID of the vehicle that has reserved this crossing.
        /// A value of \c 0 means no reservation.
        // Since the vehicle-ids start at 300, 0 can never be a reservation
        ID reservationId;

        friend class NodeLogicTrafficLight;
        friend class NodeLogicRightFirst;
        friend class NodeLogicPriorityCrossing;

    public:

        /**
         Constructs an instance of this class.
         @param roadSystem The RoadSystem this node should be part of.
         @param id The id of the new node.
         @param position The position of the node.
         @param features (Optional) features at this position.
         */
        Node(const RoadSystem *roadSystem, const ID id, const Vec2f position, const FEATURE features = NONE);

        /**
         Returns the id of this node.
         @return Its id.
         */
        ID getId() const;

        /**
         Sets the features at this position.
         @param features The features to set.
         */
        void setFeatures(FEATURE features);

        /**
         Returns the features at this position.
         @return Returns the features.
         */
        FEATURE getFeatures() const;

        /**
         Sets the logic responsible for this location.
         If a vehicle tries to enter the node, the logic is asked whether it should be permitted.
         If no logic is set, it will always be allowed.
         @param logic The logic to use.
         */
        void setNodeLogic(NodeLogic* logic);

        /**
         Returns the current logic.
         @return The logic currently in use.
         */
        NodeLogic* getNodeLogic() const;


        /**
         Gets the position of the node.
         @return The current position of the node.
         */
        Vec2f getPosition() const;

        /**
         Adds a street to this node.
         Only the id of the street is stored.
         @param street The street to add.
         @return \c True if the node is part of the street && the street has been added.
         */
        bool addStreet(const Street* street);

        /**
         Removes a street from this node.
         If the street has not been part of this node, nothing happens.
         @param street The street to remove.
         */
        void removeStreet(const Street* street);

        /**
         Returns a vector with the connected street-ids.
         @return A vector with the connected street-ids.
         */
        const vector<ID>& getStreetIds() const;

        /**
         Checks whether a vehicle is allowed to enter the crossroad.
         @param streetId The id of the street the vehicle is coming from.
         @param lane The number of the lane the vehicle is on.
         @param nextStreetId The next Street the vehicle wants to drive on.
         @param nextLane The lane on the next Street.
         @return \c 0 if the vehicle is allowed to drive through, a positive value
                 if the vehicle should stop at the returned distance.
         */
        // The distance is used to avoid the effect that all vehicles stop
        // at the middle of the crossroad since that is the position of the node.
        int canEnter(const ID streetId, const int lane, const ID nextStreetId, const int nextLane) const;

        /**
         Writes some information about this object into a string.
         @param extendedOutput If \c true, more information will be returned as a multi-line string.
         @return A string describing this object.
         */
        string toString(bool extendedOutput = true) const;

        /**
         Returns the ids of the streets a vehicle can drive on after reaching this node.
         @param street The street the vehicle is driving on.
         @param direction The direction the vehicle has been driving into.
         @return A vector containing pairs of IDs && directions of streets going away
            from this node that are available coming from the current street.
         */
        vector< pair<ID, int> > calculatePossibleStreets(const Street *street, const int direction) const;

        /**
         Returns the id of the street containing the given node.
         If the given node is not a neighbor of this node following one of the connected
         streets, the return value is undefined.
         @param nextNode The ID of the Node to find the Street to.
         @return A pair containing the ID of the Street with the given node && the direction to travel to reach it.
         */
        pair<ID, int> findNextStreet(const ID nextNode) const;

        /**
         Returns the id of the next node when following the given street starting at this node.
         @param street The ID of the street to get a node from.
         @param direction The direction to drive on the street.
         @return The ID of the next Node with on the given Street coming from this Node.
         */
        ID getNextNode(const ID street, const int direction) const;

        /**
         Returns the ids of the nodes that can be reached after driving through this node.
         If only one connection exists on this node, that connection is returned regardless
         of other considerations.
         @note This method was written for the routing algorithms.
         @param nodeComingFrom The node the vehicle is coming from.
         @return A vector containing the IDs of the neighbor-nodes && streets that can be reached after
            driving through this node when coming from the given node.
         */
        vector< pair<ID, ID> > getNextNodes(const ID nodeComingFrom) const;

        /**
         Returns the lane numbers a vehicle has to drive on while on the current street to reach a specific node.
         @param street The street the vehicle is driving on.
         @param direction The direction the vehicle is driving into.
         @param nextNode The next node the vehicle wants to drive to after reaching this node.
         @return A set containing the lane numbers on the current street which turn to the given \c nextNode.
                 That could be multiple lanes, for example a big crossroad with two lanes going right.
         */
        set<int> calculatePossibleLanes(const ID street, const int direction, const ID nextNode) const;

        /**
         Reserves the passage through this node for a vehicle.
         Note that this does not influence the result of canEnter()
         but has to be checked manually.
         @param newReservation The id of the vehicle to reserve the node for.
         */
        void setReservation(const ID newReservation);

        /**
         Returns the current reservation of this node.
         @return The reservation.
         */
        ID getReservation() const;
};

#endif // NODE_H
