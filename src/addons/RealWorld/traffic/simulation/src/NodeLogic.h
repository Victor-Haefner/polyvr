#ifndef LOGIC_H
#define LOGIC_H

#include <string>

#include "types.h"
#include "Street.h"

using namespace std;

class Node;
class RoadSystem;

/**
 Represents the logic for a node.
 In the easiest case this is a one-node right-before-left crossing.
 Another case is a crossing with multiple nodes containing traffic lights
 that are connected with each other.
 */
class NodeLogic {

    public:
        /// The types a NodeLogic can have
        enum TYPE {
            /// It is a NodeLogicRightFirst object.
            RIGHT_BEFORE_LEFT,
            /// It is a NodeLogicPriorityCrossing object.
            PRIORITY_CROSSING,
            /// It is a NodeLogicTrafficLight object.
            TRAFFIC_LIGHT
        };

    protected:

        /// The type this object has.
        TYPE type;

        /**
         A protected default constructor to enforce the usage
         of the static construction method.
         @param type The type of the new logic.
         */
        NodeLogic(const TYPE type)
            : type(type) { };

    public:

        /**
         Returns a NodeLogic-object for a node.
         Depending on the node && the subclass through which this method is invoked,
         either a new object might be created || an object from a nearby node will be
         used, too.
         @param roadSystem The RoadSystem this logic is part of.
         @param nodeId The id of the node to create a NodeLogic for.
         @return A pointer to an object.
         */
        static NodeLogic* makeNodeLogic(const RoadSystem *roadSystem, const ID nodeId);

        /**
         A virtual destructor.
         */
        virtual ~NodeLogic() { };

        /**
         Returns the type of this NodeLogic.
         @return The type of this logic.
         */
        TYPE getType() const {
            return type;
        }

        /**
         Does some updates of the node when called.
         Should be called regulary.
         */
        virtual void tick() = 0;

        /**
         Returns the center of this crossroad.
         If multiple nodes share one NodeLogic, this will be the center
         of all these nodes.
         @return The position of this logic.
         */
        virtual Vec2f getPosition() const = 0;

        /**
         Should be called if a street is added to a connected node.
         A connected node in this case means one of the nodes that reference
         this NodeLogic.
         @param node The id of the Node the street has been added to.
         @param street The id of Street that has been added.
         */
        virtual void addStreet(const Node* node, const Street* street) = 0;

        /**
         Should be called if a street is removed from a connected node.
         A connected node in this case means one of the nodes that reference
         this NodeLogic.
         @param node The id of the Node the street has been removed from.
         @param street The id of Street that has been removed.
         */
        virtual void removeStreet(const Node* node, const Street* street) = 0;

        /**
         Checks whether a vehicle is allowed to enter the crossroad.
         @param node The node to do the calculation for. This way multiple nodes
                     can share one NodeLogic, e.g. for big crossroads with multiple
                     traffic lights.
         @param streetId The id of the street the vehicle is coming from.
         @param lane The number of the lane the vehicle is on.
         @param nextStreetId The next Street the vehicle wants to drive on.
         @param nextLane The lane on the next Street
         @return \c 0 if the vehicle is allowed to drive through, a positive value
                 if the vehicle should stop at the returned distance.
         */
        // The distance is used to avoid the effect that all vehicles stop
        // at the middle of the crossroad since that is the position of the node.
        virtual int canEnter(const Node* node, const ID streetId, const int lane, const ID nextStreetId, const int nextLane) const = 0;

        /**
         Writes some information about this object into a string.
         @param extendedOutput If \c true, more information will be returned as a multi-line string.
         @return A string describing this object.
         */
        virtual string toString(bool extendedOutput = true) const = 0;
};

#endif // LOGIC_H
