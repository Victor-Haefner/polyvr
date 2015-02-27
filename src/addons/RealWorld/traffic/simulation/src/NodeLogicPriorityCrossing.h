#ifndef NODELOGICPRIORITYCROSSING_H
#define NODELOGICPRIORITYCROSSING_H

#include "NodeLogic.h"

class RoadSystem;

/**
 A node logic that represents a crossing between streets of different sizes.
 The vehicles on the bigger street are allowed to drive first.

 This class is quite similar to NodeLogicRightFirst but checks first for street size && only
 then for the direction of streets with the same size.
 */
class NodeLogicPriorityCrossing : public NodeLogic {

    private:
        /// The RoadSystem this logic is part of.
        const RoadSystem *roadSystem;
        /// The node at which the crossing exists.
        const ID nodeId;

    protected:
        /**
         Creates an object of this class.
         Can only be called indirect over makeNodeLogic().
         @param roadSystem The RoadSystem this logic is part of.
         @param nodeId The node at which the crossing exists.
         */
        NodeLogicPriorityCrossing(const RoadSystem *roadSystem, const ID nodeId);

        /// The time in seconds a approaching vehicle has to be away to drive before it.
        const static int DRIVE_THROUGH_DISTANCE = 10;

        /**
         Returns whether a vehicle on the given street is arriving at the given node.
         @param street The street to check.
         @param direction The direction the vehicle could come from.
         @param node The node to arrive at.
         @return \c True if a vehicle is approaching, \c false otherwise.
         */
        bool getStreetHasIncomming(Street *street, const int direction, const Node *node) const;

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
        virtual ~NodeLogicPriorityCrossing() { };
        virtual void tick();
        virtual Vec2f getPosition() const;
        virtual void addStreet(const Node* node, const Street* street);
        virtual void removeStreet(const Node* node, const Street* street);
        virtual int canEnter(const Node* node, const ID streetId, const int lane, const ID nextStreetId, const int nextLane) const;
        virtual string toString(const bool extendedOutput) const;
};

#endif // NODELOGICPRIORITYCROSSING_H

