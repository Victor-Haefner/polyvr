#include "NodeLogicPriorityCrossing.h"

#include <boost/lexical_cast.hpp>

using boost::lexical_cast;

#include "types.h"
#include "timer.h"

#include "Node.h"
#include "RoadSystem.h"

bool NodeLogicPriorityCrossing::getStreetHasIncomming(Street *street, const int direction, const Node *node) const {

    if (street->getIsMicro()) {

        for (unsigned int i = 1; i < street->getLaneCount(direction); ++i) {
            const set<ID>* vehicles = street->getLaneVehicles(direction * i);
            for (set<ID>::const_iterator v = vehicles->begin(); v != vehicles->end(); ++v) {
                double time = calcDistance(roadSystem->getVehicle(*v)->getPosition(), node->getPosition()) / roadSystem->getVehicle(*v)->getCurrentSpeed();

                // Stop if a vehicle from the right is too near
                if (time < DRIVE_THROUGH_DISTANCE)
                    return true;
            }
        }
    } else {
        // Ignore this sometimes since it leads to jams
        if (rand() % 10 < 2)
            return false;
        multiset< pair<ptime, ID> >* arrivalTimes = street->getLaneArrivalTimes(direction);
        for (multiset< pair<ptime, ID> >::const_iterator t = arrivalTimes->begin(); t != arrivalTimes->end(); ++t) {
            if (t->second == node->getId() && t->first - timer.getTime() < seconds(DRIVE_THROUGH_DISTANCE))
                return true;
        }
    }
    return false;
}

NodeLogicPriorityCrossing::NodeLogicPriorityCrossing(const RoadSystem *roadSystem, const ID nodeId)
    : NodeLogic(PRIORITY_CROSSING), roadSystem(roadSystem), nodeId(nodeId) {
}

void NodeLogicPriorityCrossing::tick() {
    // Nothing to do
}

int NodeLogicPriorityCrossing::canEnter(const Node* node, const ID streetId, const int lane, const ID nextStreetId, const int nextLane) const {

    // Speed up processing if only one street is connected (that is the most common case)
    if (node->getStreetIds().size() <= 1)
        return 0;

    // Transform nextLane into a direction
    const int nextDirection = (nextLane > 0) ? 1 : -1;

    // Our type. Needed to recognize bigger streets
    Street::TYPE ourType = roadSystem->getStreet(streetId)->getType();

    // Find the Connection matching the given parameters
    set<Node::Connection>::reverse_iterator incoming = node->connections.rbegin();

    for (; incoming != node->connections.rend(); ++incoming) {
        if (incoming->street == streetId && incoming->flags & Node::Connection::INCOMING
            && ((incoming->flags & Node::Connection::FORWARD && lane > 0)
                || (!(incoming->flags & Node::Connection::FORWARD) && lane < 0)))
            break;
    }
    // If not found, allow entering and hope for the best
    if (incoming == node->connections.rend())
        return 0;

    // Find the next incoming node on the right of this one
    // Connections right of this connection should be reached later by the iterator
    set<Node::Connection>::reverse_iterator rightIncoming = incoming;
    ++rightIncoming;

    // Whether a all-streets-incoming cycle should be broken (5% chance)
    bool breakCycle = rand() % 20 == 0;

    // Search until an incoming lane is found
    for (; rightIncoming != incoming; ++rightIncoming) {

        // On reaching the end, begin again
        if (rightIncoming == node->connections.rend()) {
            rightIncoming = node->connections.rbegin();
            if (rightIncoming == incoming)
                break;
        }

        // If it is an other street and incoming, we found a street
        if (rightIncoming->flags & Node::Connection::INCOMING) {

            Street *street = roadSystem->getStreet(rightIncoming->street);
            const int direction = (rightIncoming->flags & Node::Connection::FORWARD) ? 1 : -1;

            // If the street (is smaller) or (it is the same type and angle <= 170°), check for incoming vehicles

            // If it is smaller, we can drive before it
            if (street->getType() < ourType)
                continue;

            // If the size is the same as our size, behave like NodeLogicRightFirst
            if (street->getType() == ourType) {

                // Check if the angle is bigger than 170°. In that case, no right-first check is done
                if (int(incoming->degree - rightIncoming->degree + 720) % 360 > 170)
                    continue;

                // If this is the street the vehicle wants to drive to, allow it since turning right is always allowed
                if (rightIncoming->street == nextStreetId && direction == nextDirection)
                    return 0;
            }

            // If we come here, the other street is either (bigger) or (equal sized and to the right)
            // In both cases, check for incoming vehicles

            // Find the first vehicle on the lane and calculate the time until arrival

            if (getStreetHasIncomming(street, direction, node)) {
                if (!breakCycle)
                    return CROSSROAD_RADIUS;
                else
                    break;
            }
        }
    }
    if (rightIncoming == incoming)
        // Checked all streets, found no vehicles: Allow driving
        return 0;


    // Search until an incoming lane is found
    rightIncoming = incoming;
    ++rightIncoming;
    for (; rightIncoming != incoming; ++rightIncoming) {

        // On reaching the end, begin again
        if (rightIncoming == node->connections.rend()) {
            rightIncoming = node->connections.rbegin();
            if (rightIncoming == incoming)
                break;
        }

        // If it is an other street and incoming, we found a street
        if (rightIncoming->flags & Node::Connection::INCOMING) {

            // Find the first vehicle on the lane and calculate the time until arrival
            Street *street = roadSystem->getStreet(rightIncoming->street);
            int direction = (rightIncoming->flags & Node::Connection::FORWARD) ? 1 : -1;

            // If there is no vehicle coming, we have no cycle and can simply wait
            if (!getStreetHasIncomming(street, direction, node))
                return CROSSROAD_RADIUS;
        }
    }

    // A cycle should be broken and there is a cycle, so allow driving through
    return 0;
}

NodeLogic* NodeLogicPriorityCrossing::makeNodeLogic(const RoadSystem *roadSystem, const ID nodeId) {
    Node *node = roadSystem->getNode(nodeId);
    NodeLogic *l = new NodeLogicPriorityCrossing(roadSystem, nodeId);
    node->setNodeLogic(l);
    return l;
}

Vec2f NodeLogicPriorityCrossing::getPosition() const {
    return roadSystem->getNode(nodeId)->getPosition();
}

void NodeLogicPriorityCrossing::addStreet(const Node*, const Street*) {

}

void NodeLogicPriorityCrossing::removeStreet(const Node*, const Street*) {

}

string NodeLogicPriorityCrossing::toString(const bool extendedOutput) const {

    string str = string("NodeLogicPriorityCrossing") + ((extendedOutput)?"\n  ":" [")
        + "pos=" + lexical_cast<string>(getPosition()[0]) + " / " + lexical_cast<string>(getPosition()[1]) + ((extendedOutput)?"\n  ":"; ")
        + "node=" + lexical_cast<string>(nodeId) + ((extendedOutput)?"":"]");

    return str;
}
