#include "NodeLogicRightFirst.h"

#include <boost/lexical_cast.hpp>

using boost::lexical_cast;

#include "types.h"
#include "timer.h"

#include "Node.h"
#include "RoadSystem.h"

bool NodeLogicRightFirst::getStreetHasIncomming(Street *street, const int direction, const Node *node) const {

    if (street->getIsMicro()) {

        // Block if a vehicle is nearer as a certain distance
        // To have a bigger distance on big streets, just use half of the maxSpeed as distance in meter

        for (unsigned int i = 1; i <= street->getLaneCount(direction); ++i) {
            const set<ID>* vehicles = street->getLaneVehicles(direction * i);
            for (set<ID>::const_iterator v = vehicles->begin(); v != vehicles->end(); ++v) {

                Vehicle *otherVehicle = roadSystem->getVehicle(*v);

                if (otherVehicle->getState() & (Vehicle::WAITING | Vehicle::BLOCKED))
                    continue;

                if (otherVehicle->getRoute()->at(1) != node->getId() && (otherVehicle->getRoute()->size() <= 2 || otherVehicle->getRoute()->at(2) != node->getId()))
                    continue;

                double distance = calcDistance(otherVehicle->getPosition(), node->getPosition());

                double meterPerSecond = max(otherVehicle->getCurrentSpeed(), 10.0) * 1000 / (60 * 60);

                double timeUntilArrival = distance / meterPerSecond;

                // Return true if a vehicle is too near
                if (timeUntilArrival < DRIVE_THROUGH_DISTANCE)
                    return true;
            }
        }
    } else {
        multiset< pair<ptime, ID> >* arrivalTimes = street->getLaneArrivalTimes(direction);
        for (multiset< pair<ptime, ID> >::const_iterator t = arrivalTimes->begin(); t != arrivalTimes->end(); ++t) {
            if (t->second == node->getId())
                // Since the entries are sorted after arrival-time, only the first one for this node has to be checked
                // Only block in 90% of the cases, ignore the other 10%. This is used to break deadlocks
                return (t->first - timer.getTime() < seconds(DRIVE_THROUGH_DISTANCE) && rand() % 10 != 0);
        }
    }
    return false;
}

NodeLogicRightFirst::NodeLogicRightFirst(const RoadSystem *roadSystem, const ID nodeId)
    : NodeLogic(RIGHT_BEFORE_LEFT), roadSystem(roadSystem), nodeId(nodeId) {
}

void NodeLogicRightFirst::tick() {
    // Nothing to do
}

int NodeLogicRightFirst::canEnter(const Node* node, const ID streetId, const int lane, const ID nextStreetId, const int nextLane) const {

    // Speed up processing if only one street is connected (that is the most common case)
    // Actually, not. If there is only one street, there should be no NodeLogicRightFirst.
    if (node->getStreetIds().size() <= 1)
        return 0;

    // Transform nextLane into a direction
    const int nextDirection = (nextLane > 0) ? 1 : -1;

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

    // Whether an all-streets-have-incoming cycle should be broken (10% chance)
    bool breakCycle = rand() % 10 == 0;

    // Search until an incoming lane is found
    for (; rightIncoming != incoming; ++rightIncoming) {

        // On reaching the end, begin again
        if (rightIncoming == node->connections.rend()) {
            rightIncoming = node->connections.rbegin();
            if (rightIncoming == incoming)
                break;
        }

        // If the angle between the next street and "our" street is bigger than 170 degree, abort
        // In that case there is no street to the right and the vehicle can drive through
        if (int(incoming->degree - rightIncoming->degree + 720) % 360 > 170)
            return 0;

        const int direction = (rightIncoming->flags & Node::Connection::FORWARD) ? 1 : -1;

        // If this is the street the vehicle wants to drive into, allow it since turning right is always allowed
        if (rightIncoming->street == nextStreetId && -1 * direction == nextDirection) {
            return 0;
        }

        // If it is an other street and incoming, we found a street
        if (rightIncoming->flags & Node::Connection::INCOMING) {

            // Find the first vehicle on the lane and calculate the time until arrival
            Street *street = roadSystem->getStreet(rightIncoming->street);

            if (getStreetHasIncomming(street, direction, node)) {
                if (!breakCycle) {
                    return CROSSROAD_RADIUS;
                } else {
                    break;
                }
            } else {
                // Street to the right is free: drive through
                return 0;
            }
        }
    }

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

NodeLogic* NodeLogicRightFirst::makeNodeLogic(const RoadSystem *roadSystem, const ID nodeId) {
    Node *node = roadSystem->getNode(nodeId);
    NodeLogic *l = new NodeLogicRightFirst(roadSystem, nodeId);
    node->setNodeLogic(l);
    return l;
}

Vec2f NodeLogicRightFirst::getPosition() const {
    return roadSystem->getNode(nodeId)->getPosition();
}

void NodeLogicRightFirst::addStreet(const Node*, const Street*) {

}

void NodeLogicRightFirst::removeStreet(const Node*, const Street*) {

}

string NodeLogicRightFirst::toString(const bool extendedOutput) const {

    string str = string("NodeLogicRightFirst") + ((extendedOutput)?"\n  ":" [")
        + "pos=" + lexical_cast<string>(getPosition()[0]) + " / " + lexical_cast<string>(getPosition()[1]) + ((extendedOutput)?"\n  ":"; ")
        + "node=" + lexical_cast<string>(nodeId) + ((extendedOutput)?"":"]");

    return str;
}
