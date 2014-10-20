#include "Node.h"

#include <boost/lexical_cast.hpp>

using boost::lexical_cast;

#include "NodeLogic.h"
#include "Street.h"
#include "RoadSystem.h"

bool Node::connectionComparator(const Connection& lhs, const Connection& rhs) {
    return lhs.degree < rhs.degree;
}

Node::Node(const RoadSystem *roadSystem, const ID id, const Vec2f position, const FEATURE features)
    : id(id), pos(position), streetIds(), connections(&connectionComparator), logic(NULL), features(features), roadSystem(roadSystem), reservationId(0) {
}

ID Node::getId() const {
    return id;
}

void Node::setFeatures(FEATURE features) {
    this->features = features;
}

Node::FEATURE Node::getFeatures() const {
    return features;
}

void Node::setNodeLogic(NodeLogic* logic) {
    this->logic = logic;
}

NodeLogic* Node::getNodeLogic() const {
    return logic;
}

Vec2f Node::getPosition() const {
    return pos;
}

bool Node::addStreet(const Street* street) {

    // If the given street is not yet known, add it and add it to the connection-list, too
    for (vector<ID>::iterator iter = streetIds.begin(); iter != streetIds.end(); ++iter) {
        if (*iter == street->getId())
            // Street already registered: abort
            return true;
    }

    // Find the positions of this node on the street
    // There can be multiple positions if the street is a loop or is connected to its middle
    const vector<ID> *nodes = street->getNodeIds();
    bool found = false;
    for (size_t nodeI = 0; nodeI < nodes->size(); ++nodeI) {
        if ((*nodes)[nodeI] == this->id) {

            // Remember that we have found at least one node
            found = true;

            // Create and add the connections
            // If the node is not the first one on the street, there has to be one before it
            // If so, calculate the direction to it and add a Connection to it.
            if (nodeI > 0) {
                Connection con;
                con.node = (*nodes)[nodeI - 1];
                Vec2f prevNodePos = roadSystem->getNode(con.node)->getPosition();
                con.degree = calcAngle(prevNodePos - pos);
                con.street = street->getId();
                con.flags = (street->getLaneCount(1)?Connection::INCOMING:0) | (street->getLaneCount(-1)?Connection::OUTGOING:0) | Connection::FORWARD;
                connections.insert(con);
            }
            if (nodeI < nodes->size() - 1) {
                Connection con;
                con.node = (*nodes)[nodeI + 1];
                Vec2f prevNodePos = roadSystem->getNode(con.node)->getPosition();
                con.degree = calcAngle(prevNodePos - pos);
                con.street = street->getId();
                con.flags = (street->getLaneCount(-1)?Connection::INCOMING:0) | (street->getLaneCount(1)?Connection::OUTGOING:0);
                connections.insert(con);
            }

        }
    }

    // If no node has been found, abort
    if (!found)
        return false;

    // Add the street
    streetIds.push_back(street->getId());

    if (logic != NULL)
        logic->addStreet(this, street);

    return true;
}

void Node::removeStreet(const Street* street) {

    if (logic != NULL)
        logic->removeStreet(this, street);


    // Delete the street id from the list
    for (vector<ID>::iterator iter = streetIds.begin(); iter != streetIds.end(); ++iter) {
        if (*iter == street->getId()) {
            // To speed up the removal, overwrite the to-be-removed entry with the last one
            *iter = streetIds.back();
            // Delete the last entry
            streetIds.pop_back();
            break;
        }
    }

    // Delete all connections using the street
    for (multiset<Connection>::iterator iter = connections.begin(); iter != connections.end(); ) {
        if (iter->street == street->getId()) {
            connections.erase(iter++);
        } else {
            ++iter;
        }
    }
}

const vector<ID>& Node::getStreetIds() const {
    return streetIds;
}

int Node::canEnter(const ID streetId, const int lane, const ID nextStreetId, const int nextLane) const {

    // If we are changing street to a meso-street, check if there is any space left
    if (streetId != nextStreetId) {

        const Street *nextStreet = roadSystem->getStreet(nextStreetId);

        // Check if the next street has some space left
        if (!nextStreet->getIsMicro() && nextStreet->getLaneVehicleCount(nextLane) >= nextStreet->getLaneMaxVehicleCount(nextLane))
            return 1;
    } else if (/* streetId == nextStreetId && */ connections.size() == 1 && connections.begin()->flags & Connection::OUTGOING) {
        const Street *nextStreet = roadSystem->getStreet(nextStreetId);
        if (nextStreet->getIsMicro()) {
            // It is a micro-street and a dead-end, but the vehicles can turn around and drive back here
            // Only allow entering if there is no other vehicle near the node
            int nearVehicleCount = 0;
            for (set<ID>::iterator idIter = nextStreet->getLaneVehicles(nextLane)->begin(); idIter != nextStreet->getLaneVehicles(nextLane)->end(); ++idIter) {
                const Vehicle *otherVehicle = roadSystem->getVehicle(*idIter);
                if (calcDistance(otherVehicle->getPosition(), pos) < 10) {
                    ++nearVehicleCount;
                }
            }
            if (nearVehicleCount > 1)
                return 10;
        }
    }

    if (logic == NULL)
        return 0;
    else
        return logic->canEnter(this, streetId, lane, nextStreetId, nextLane);
}

string Node::toString(const bool extendedOutput) const {

    string str = string("Node #") + lexical_cast<string>(id) + ((extendedOutput)?"\n  ":" [")
        + "pos=" + lexical_cast<string>(pos[0]) + " / " + lexical_cast<string>(pos[1]) + ((extendedOutput)?"\n  ":"; ")
        + "logic=" + lexical_cast<string>(logic) + ((extendedOutput)?"\n  ":"; ")
        + "features=" + ((features & TRAFFICLIGHTS)?"traffic_lights":"none") + ((extendedOutput)?"\n  ":"; ")
        + "reservation=" + lexical_cast<string>(reservationId) + ((extendedOutput)?"\n  ":"; ")
        + "#streets=" + lexical_cast<string>(streetIds.size()) + ((extendedOutput)?"\n    ":"; ");

    for (unsigned int i = 0; i < streetIds.size() && extendedOutput; ++i)
        str += lexical_cast<string>(streetIds[i]) + ((i < streetIds.size() - 1)?"\n    ":"\n  ");

    str += "#connections=" + lexical_cast<string>(connections.size()) + ((extendedOutput)?"\n    ":"; ");
    for (multiset<Connection>::iterator iter = connections.begin(); iter != connections.end() && extendedOutput; ++iter)
        str += lexical_cast<string>(iter->street) + " (" + lexical_cast<string>((int)iter->flags) + ") at " + lexical_cast<string>(iter->degree) + ((iter != --connections.end())?" degree\n    ":" degree\n  ");

    str += ((extendedOutput)?"":"]");

    return str;
}

vector< pair<ID, int> > Node::calculatePossibleStreets(const Street *street, const int direction) const {

    // Quite similar to calculatePossibleLanes()
    // (Well, actually it is adapted copy&paste)
    vector< pair<ID, int> > result;

    // If the street is not connected to the node, return an empty vector
    vector<ID>::const_iterator streetIdIter = streetIds.begin();
    for (; streetIdIter != streetIds.end(); ++streetIdIter) {
        if (*streetIdIter == street->getId())
            break;
    }
    if (streetIdIter == streetIds.end())
        return result;

    // Find the connection the vehicle is on
    multiset<Connection>::iterator connectionIter = connections.begin();
    for (; connectionIter != connections.end(); ++connectionIter) {
        if (connectionIter->street != street->getId())
            continue;
        if (!(connectionIter->flags & Connection::INCOMING))
            continue;
        if (connectionIter->flags & Connection::FORWARD && direction > 0)
            break;
        else if(!(connectionIter->flags & Connection::FORWARD) && direction < 0)
            break;
    }

    // If not found, return an empty vector
    if (connectionIter == connections.end())
        return result;

    // Get the directions where turning is allowed to on the current street
    Street::LANEFLAG flags = 0;
    for (int lane = 1; lane <= (int)street->getLaneCount(direction); ++lane)
        flags |= street->getLaneFlags(direction * lane);


    // Iterate through the connections and check which ones can be used with the allowed turnings
    for (multiset<Connection>::iterator connectionNextIter = connections.begin(); connectionNextIter != connections.end(); ++connectionNextIter) {

        // If not outgoing, ignore this connection
        if (!(connectionNextIter->flags & Connection::OUTGOING))
            continue;

        int angle = (int)(connectionNextIter->degree - connectionIter->degree + 360) % 360;
        if ((135 < angle && angle <= 225 && flags & Street::TURN_THROUGH)
            || (45 < angle && angle <= 135 && flags & Street::TURN_LEFT)
            ||  (225 < angle && angle <= 315 && flags & Street::TURN_RIGHT)) {
            result.push_back(make_pair(connectionNextIter->street, (connectionNextIter->flags & Connection::FORWARD) ?  -1 : 1));
        }
    }

    return result;
}

pair<ID, int> Node::findNextStreet(const ID nextNode) const {

    // Find the connection with the node
    multiset<Connection>::iterator connectionIter = connections.begin();
    for (; connectionIter != connections.end(); ++connectionIter)
        if (connectionIter->node == nextNode && connectionIter->flags & Connection::OUTGOING)
            return make_pair(connectionIter->street, (connectionIter->flags & Connection::FORWARD)? -1 : 1);

    // If not found, return trash that will likely lead to a crash
    return make_pair(0, 0);
}

ID Node::getNextNode(const ID street, const int direction) const {

    // Find the connection with the street and direction
    multiset<Connection>::iterator connectionIter = connections.begin();
    for (; connectionIter != connections.end(); ++connectionIter)
        if (connectionIter->street == street
            && ((connectionIter->flags & Connection::FORWARD && direction > 0)
                || (!(connectionIter->flags & Connection::FORWARD) && direction < 0)))
            return connectionIter->node;

    // If not found, return trash
    return 42;
}

vector< pair<ID, ID> > Node::getNextNodes(const ID nodeComingFrom) const {

    // The same as calculatePossibleStreets() but returning the nodes instead of the streets
    vector< pair<ID, ID> > result;

    // Find the connection the vehicle is on
    multiset<Connection>::iterator connectionIter = connections.begin();
    for (; connectionIter != connections.end(); ++connectionIter) {
        if (connectionIter->node == nodeComingFrom
            && connectionIter->flags & Connection::INCOMING) // This is quite likely the case anyway
            break;
    }

    // If not found, return an empty vector
    if (connectionIter == connections.end())
        return result;

    // If there is only one outgoing lane (e.g. at all the dead ends), return it
    // This special case is needed since there is no TURN_BACK that would us
    // allow to go there otherwise
    for (multiset<Connection>::iterator connectionNextIter = connections.begin(); connectionNextIter != connections.end(); ++connectionNextIter) {
        // Make sure we are allowed to drive on the opposite lane
        if (connectionNextIter->flags & Connection::OUTGOING)
            result.push_back(make_pair(connectionNextIter->node, connectionNextIter->street));
    }
    if (result.size() == 1) {
        return result;
    } else {
        result.clear();
    }

    const Street *street = roadSystem->getStreet(connectionIter->street);
    const int direction = (connectionIter->flags & Connection::FORWARD) ? 1 : -1;

    // Get the directions where turning is allowed to on the current street
    Street::LANEFLAG flags = 0;
    for (int lane = 1; lane <= (int)street->getLaneCount(direction); ++lane)
        flags |= street->getLaneFlags(direction * lane);

    // Iterate through the connections and check which ones can be used with the allowed turnings
    for (multiset<Connection>::iterator connectionNextIter = connections.begin(); connectionNextIter != connections.end(); ++connectionNextIter) {

        // If not outgoing, ignore this connection
        if (!(connectionNextIter->flags & Connection::OUTGOING))
            continue;

        int angle = (int)(connectionNextIter->degree - connectionIter->degree + 360) % 360;
        if ((135 < angle && angle <= 225 && flags & Street::TURN_THROUGH)
            || (45 < angle && angle <= 135 && flags & Street::TURN_LEFT)
            ||  (225 < angle && angle <= 315 && flags & Street::TURN_RIGHT)) {
            // This line is the primary change
            result.push_back(make_pair(connectionNextIter->node, connectionNextIter->street));
        }
    }

    return result;
}

set<int> Node::calculatePossibleLanes(const ID street, const int direction, const ID nextNode) const {

    set<int> result;

    // If the street is not connected to the node, return an empty set
    vector<ID>::const_iterator streetIdIter = streetIds.begin();
    for (; streetIdIter != streetIds.end(); ++streetIdIter) {
        if (*streetIdIter == street)
            break;
    }
    if (streetIdIter == streetIds.end())
        return result;

    // Find the connection the vehicle is on
    multiset<Connection>::iterator connectionIter = connections.begin();
    for (; connectionIter != connections.end(); ++connectionIter)
        if (connectionIter->street == street && connectionIter->flags & Connection::INCOMING
            && ((connectionIter->flags & Connection::FORWARD && direction > 0)
                || (!(connectionIter->flags & Connection::FORWARD) && direction < 0)))
            break;

    // If not found, return an empty set
    if (connectionIter == connections.end())
        return result;

    // If there is only one connection (e.g. at all the dead ends), return it
    // This special case is needed since there is no TURN_BACK that would us
    // allow to go there otherwise
    if (connections.size() == 1) {
        // Make sure we are allowed to drive on the opposite direction
        if (connectionIter->flags & Connection::OUTGOING) {
            Street *streetPtr = roadSystem->getStreet(connectionIter->street);
            // Add all lanes since it does not matter
            for (size_t i = 1; i <= streetPtr->getLaneCount(direction); ++i)
                result.insert(i);
        }
        return result;
    }


    // Find the connection with nextNode
    multiset<Connection>::iterator connectionNextIter = connections.begin();
    for (; connectionNextIter != connections.end(); ++connectionNextIter)
        if (connectionNextIter->node == nextNode)
            break;

    // If not found, return an empty set
    if (connectionNextIter == connections.end())
        return result;


    // Find out whether left, through or right is matching
    // Calculate the angle between the directions
    int angle = (int)(connectionNextIter->degree - connectionIter->degree + 360) % 360;
    // An angle of ~180° means straight through, lower values means left, higher means right
    Street::LANEFLAG possibleDirections;
    if (135 < angle && angle <= 225)
        possibleDirections = Street::TURN_THROUGH;
    else if (45 < angle && angle <= 135)
        possibleDirections = Street::TURN_LEFT;
    else if (225 < angle && angle <= 315)
        possibleDirections = Street::TURN_RIGHT;
    // else ... TURN_BACK ? Sorry, not supported. :(

    // Find the matching lanes on the current street
    Street *streetPtr = roadSystem->getStreet(street);

    if (direction > 0) {
        for (unsigned int laneI = 1; laneI <= streetPtr->getLaneCount(1); ++laneI) {
            if (streetPtr->getLaneFlags(laneI) & possibleDirections)
                result.insert(laneI);
        }
    } else {
        for (unsigned int laneI = 1; laneI <= streetPtr->getLaneCount(-1); ++laneI) {
            if (streetPtr->getLaneFlags(-1 * laneI) & possibleDirections)
                result.insert(laneI);
        }
    }

    return result;
}

void Node::setReservation(const ID newReservation) {
    reservationId = newReservation;
}

ID Node::getReservation() const {
    return reservationId;
}
