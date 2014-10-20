#include <math.h>
#include <boost/lexical_cast.hpp>

using boost::lexical_cast;

#include "Street.h"

#include "RoadSystem.h"
#include "Vehicle.h"
#include "Routing.h"

Street::Lanes::Lanes()
    : flags(), cars(), maxVehicles(0), arrivalTimes() {
}

size_t Street::getNextInterestingNode(size_t currentNodeI, const int direction) const {

    if (direction > 0) {
        for (size_t i = currentNodeI + 1; i < nodes.size(); ++i) {
            if (roadSystem->getNode(nodes[i])->getNodeLogic() != NULL) {
                return i;
            }
        }
        return nodes.size() - 1;
    } else {
        // If the current node is 0, start at the end
        // Needed for cyclic streets
        if (currentNodeI == 0)
            currentNodeI = getNodeIndexFromBack(nodes[currentNodeI]);

        for (size_t i = currentNodeI; i > 0; --i) {
            if (roadSystem->getNode(nodes[i - 1])->getNodeLogic() != NULL) {
                return i - 1;
            }
        }
        return 0;
    }
}

Street::Street(RoadSystem *roadSystem, const ID id, const vector<ID>& nodeIds)
    : flags(0), forward(), backward() , id(id), maxSpeed(-1), roadSystem(roadSystem), type(DEFAULT), nodes(nodeIds), nodeDistances() {

    assert(nodes.size() >= 2 && "A street needs to have at least 2 nodes.");

    Vec2f oldPos;
    Vec2f newPos = roadSystem->getNode(nodes[0])->getPosition();
    // Fist node has obviously distance 0 to the beginning
    // Use 0.001 instead of 0 to avoid div0 crashes
    nodeDistances.push_back(0.001);
    for (size_t nodeI = 1; nodeI < nodes.size(); ++nodeI) {
        oldPos = newPos;
        newPos = roadSystem->getNode(nodes[nodeI])->getPosition();
        nodeDistances.push_back(nodeDistances.back() + calcDistance(oldPos, newPos));
    }
}

ID Street::getId() const {
    return id;
}

RoadSystem* Street::getRoadSystem() const {
    return roadSystem;
}

void Street::setMicro(const bool micro) {

    ptime now = timer.getTime();
    const int oldVehicleCount = getLaneVehicleCount(-1) + getLaneVehicleCount(1);

    if (micro && !getIsMicro()) {

        // Transform meso to micro

        flags |= IS_MICRO;

        // Estimate the position from the arrival time and add a vehicle to a random lane
        for (multiset< pair<ptime, ID> >::iterator iter = forward.arrivalTimes.begin();
             iter != forward.arrivalTimes.end(); ++iter) {

            double distance = nodeDistances[getNodeIndex(iter->second)];

            time_duration restTime = iter->first - now;
            time_duration requiredTime = getLaneTravelTime(1) * (distance / getLength());

            double offset = (restTime - requiredTime).total_seconds() / getLaneTravelTime(1).total_seconds();

            createRandomVehicle(1, offset);
        }
        forward.arrivalTimes.clear();

        for (multiset< pair<ptime, ID> >::iterator iter = backward.arrivalTimes.begin();
             iter != backward.arrivalTimes.end(); ++iter) {

            double distance = nodeDistances[getNodeIndex(iter->second)];

            time_duration restTime = iter->first - now;
            time_duration requiredTime = getLaneTravelTime(-1) * ((getLength() - distance) / getLength());

            double offset = (restTime - requiredTime).total_seconds() / getLaneTravelTime(-1).total_seconds();

            createRandomVehicle(-1, offset);
        }
        backward.arrivalTimes.clear();

    } else if (!micro && getIsMicro()) {
        // Transform micro to meso

        flags &= ~IS_MICRO;

        // Take all vehicles, calculate their estimated arrival times and put it into arrivalTimes

        // Forward
        for (vector< set<ID> >::iterator laneIter = forward.cars.begin(); laneIter != forward.cars.end(); ++laneIter) {
            for (set<ID>::iterator vehicleIter = laneIter->begin(); vehicleIter != laneIter->end(); ++vehicleIter) {
                ID currentDest;
                Vehicle *v = roadSystem->getVehicle(*vehicleIter);
                if (v->getRoute()->size() < 2)
                    // If the vehicle has no route (damn tourists...), let it drive until the end
                    currentDest = nodes.back();
                else
                    currentDest = v->getRoute()->at(1);
                // Very bad approximation: Take the air-distance between vehicle and destination as the
                // driving distance and the current speed for all this way
                double distance = calcDistance(v->getPosition(), roadSystem->getNode(currentDest)->getPosition());
                ptime time = now + seconds(distance * v->getCurrentSpeed() * 60);
                forward.arrivalTimes.insert(make_pair(time, currentDest));
                roadSystem->removeVehicle(*vehicleIter);
            }
            laneIter->clear();
        }

        // Backward
        for (vector< set<ID> >::iterator laneIter = backward.cars.begin(); laneIter != backward.cars.end(); ++laneIter) {
            for (set<ID>::iterator vehicleIter = laneIter->begin(); vehicleIter != laneIter->end(); ++vehicleIter) {
                ID currentDest;
                Vehicle *v = roadSystem->getVehicle(*vehicleIter);
                if (v->getRoute()->empty())
                    currentDest = nodes.back();
                else
                    currentDest = v->getRoute()->at(1);
                double distance = calcDistance(v->getPosition(), roadSystem->getNode(currentDest)->getPosition());
                ptime time = now + seconds(distance * v->getCurrentSpeed() * 60);
                backward.arrivalTimes.insert(make_pair(time, currentDest));
                roadSystem->removeVehicle(*vehicleIter);
            }
            laneIter->clear();
        }

        // Remove all reservations
        for (size_t i = 0; i < nodes.size(); ++i) {
            Node *node = roadSystem->getNode(nodes[i]);
            node->setReservation(0);
        }
    }

    const int vehicleDelta = oldVehicleCount - getLaneVehicleCount(-1) - getLaneVehicleCount(1);
    roadSystem->addOffmapVehicle(vehicleDelta);
}

bool Street::getIsMicro() const {
    return flags & IS_MICRO;
}

const vector<ID>* Street::getNodeIds() const {
    return &nodes;
}

double Street::getLength() const {
    // Returns 244 for one street, should be ~190m
    return nodeDistances.back();
}

void Street::setMaxSpeed(const double speed) {
    maxSpeed = speed;
}

double Street::getMaxSpeed() const {
    if (maxSpeed <= 0)
        // If roadSystem is NULL, the crash is your own fault ;-)
        return roadSystem->getDefaultSpeed();
    else
        return maxSpeed;
}

void Street::setType(const TYPE type) {
    this->type = type;
}

Street::TYPE Street::getType() const {
    if (type == DEFAULT)
        return roadSystem->getDefaultType();
    else
        return type;
}

void Street::setLaneCount(const int direction, unsigned int count) {

    if (direction > 0) {
        while (forward.flags.size() < count) {
            forward.flags.push_back(0);
            forward.cars.push_back(set<ID>());
        }
        while (forward.flags.size() > count) {
            forward.flags.pop_back();

            // Remove vehicles that are on the lane
            for (set<ID>::iterator iter = forward.cars.back().begin(); iter != forward.cars.back().end(); ++iter)
                roadSystem->removeVehicle(*iter);

            // Remove the lane itself
            forward.cars.pop_back();
        }
        // Calculate a new maximal vehicle count
        // +1 to avoid counts of 0 vehicles
        forward.maxVehicles = ((getLength() * count) / (VEHICLE_LENGTH * 1.5)) + 1;

    } else if (direction < 0) {

        while (backward.flags.size() < count) {
            backward.flags.push_back(0);
            backward.cars.push_back(set<ID>());
        }
        while (backward.flags.size() > count) {
            backward.flags.pop_back();

            // Remove vehicles that are on the lane
            for (set<ID>::iterator iter = backward.cars.back().begin(); iter != backward.cars.back().end(); ++iter)
                roadSystem->removeVehicle(*iter);

            // Remove the lane itself
            backward.cars.pop_back();
        }
        // Calculate a new maximal vehicle count
        backward.maxVehicles = ((getLength() * count) / (VEHICLE_LENGTH * 1.5)) + 1;
    }
}

unsigned int Street::getLaneCount(const int direction) const {
    if (direction > 0)
        return forward.flags.size();
    else
        return backward.flags.size();
}

void Street::setLaneFlags(const int lane, const LANEFLAG flags) {

    if (lane > 0 && lane - 1 < (int) forward.flags.size())
        forward.flags[lane - 1] = flags;
    else if (lane < 0 && (-1*lane) - 1 < (int) backward.flags.size())
        backward.flags[(-1*lane) - 1] = flags;
}

Street::LANEFLAG Street::getLaneFlags(const int lane) const {
    if (lane > 0 && lane - 1 < (int) forward.flags.size())
        return forward.flags[lane - 1];
    else if (lane < 0 && (-1*lane) - 1 < (int) backward.flags.size())
        return backward.flags[(-1*lane) - 1];
    else {
        cerr << "FATAL ERROR: Request for flags of lane 0.\n";
        return backward.flags[(-1*lane) - 1];
    }
}

const set<ID>* Street::getLaneVehicles(const int lane) const {
    if (lane > 0 && lane - 1 < (int) forward.cars.size())
        return &forward.cars[lane - 1];
    else if (lane < 0 && (-1*lane) - 1 < (int) backward.cars.size())
        return &backward.cars[(-1*lane) - 1];
    else {
        cerr << "FATAL ERROR: Request for vehicles on lane 0.\n";
        return &backward.cars[(-1*lane) - 1];
    }
}

bool Street::removeVehicle(const ID vehicleId, const int lane) {
    if (lane > 0) {
        return (forward.cars[lane - 1].erase(vehicleId) > 0);
    } else if (lane < 0) {
        return (backward.cars[-1 * lane - 1].erase(vehicleId) > 0);
    }
    return false;
}

double Street::getLaneDensity(const int direction) const {

    return static_cast<double>(getLaneVehicleCount(direction)) / getLaneMaxVehicleCount(direction);
}

time_duration Street::getLaneTravelTime(const int direction) const {

    if (direction == 0)
        return hours(0);

    // Time if the road would be empty
    double lengthInMeter = getLength();
    double lengthInKilometer = lengthInMeter / 1000;
    double travelTimeInHours = lengthInKilometer / getMaxSpeed();
    time_duration travelTimeInSeconds(seconds(travelTimeInHours * 60 * 60));

    // Now take a multiple of it depending on the number of vehicles
    // If the street is full, the travel will take four times as long
    travelTimeInSeconds *= (1 + (4 * getLaneDensity(direction)));

    if (travelTimeInSeconds.total_seconds() < 1)
        return seconds(1);
    else
        return travelTimeInSeconds;
}

unsigned int Street::getLaneVehicleCount(const int direction) const {

    if (getIsMicro()) {

        // In micro mode, calculate the sum over all lanes

        if (direction > 0) {
            unsigned int count = 0;
            for (size_t i = 0; i < forward.cars.size(); ++i) {
                count += forward.cars[i].size();
            }
            return count;
        } else {
            unsigned int count = 0;
            for (size_t i = 0; i < backward.cars.size(); ++i) {
                count += backward.cars[i].size();
            }
            return count;
        }


    } else {
        if (direction > 0)
            return forward.arrivalTimes.size();
        else
            return backward.arrivalTimes.size();
    }

}

unsigned int Street::getLaneMaxVehicleCount(const int direction) const {
    if (direction > 0)
        return forward.maxVehicles;
    else
        return backward.maxVehicles;
}

multiset< pair<ptime, ID> >* Street::getLaneArrivalTimes(const int direction) {
    if (direction > 0)
        return &forward.arrivalTimes;
    else
        return &backward.arrivalTimes;
}

Vec2f Street::getRelativeNodePosition(const ID nodeId, const int lane) const {

    // Find the node in the list
    unsigned int idPos = getNodeIndex(nodeId);
    if (idPos >= nodes.size()) {
        // Not found? Then the parameter has been wrong
        // If the user gives trash, he deserves this return value
        if (!nodes.empty())
            return roadSystem->getNode(nodes[0])->getPosition();
        else
            return Vec2f(0,0);
    }

    // Get the neighbor-nodes and calculate the gradient between them
    // One of these checks will be true since the street has at least two nodes
    Vec2f posPrev, posNext;
    if (idPos > 0)
        posPrev = roadSystem->getNode(nodes[idPos - 1])->getPosition();
    else
        posPrev = roadSystem->getNode(nodes[idPos])->getPosition();
    if (idPos < nodes.size() - 1)
        posNext = roadSystem->getNode(nodes[idPos + 1])->getPosition();
    else
        posNext = roadSystem->getNode(nodes[idPos])->getPosition();

    Vec2f gradiant(posNext - posPrev);

    // Calculate normal to the gradient
    Vec2f normal(gradiant[1], -gradiant[0]);
    normal.normalize();

    // Based on the normal and the lane count, add an offset to the node position
    Vec2f posNode = roadSystem->getNode(nodeId)->getPosition();
    double offset = lane;
    // Reduce the hole in the middle of the street
    if (offset > 0) offset -= 0.3; else offset += 0.3;
    // Center streets
    offset -= (double)(forward.flags.size() - backward.flags.size()) / 2;
    offset *= roadSystem->getLaneWidth();
    Vec2f ret(posNode[0] - normal[0] * offset, posNode[1] - normal[1] * offset);
    return ret;

}

Vec2f Street::getPositionOnLane(const int lane, const double offset) const {

    if (offset <= 0)
        return getRelativeNodePosition(nodes.front(), lane);

    if (offset >= getLength())
        return getRelativeNodePosition(nodes.back(), lane);

    for (unsigned int i = 1; i < nodes.size(); i++) {

        // If the offset is less than the distance to the next node, the position is on this part
        if (offset <= nodeDistances[i]) {
            // Interpolate on the part and return the position
            const double percentage = (offset - nodeDistances[i - 1]) / (nodeDistances[i] - nodeDistances[i - 1]);
            Vec2f last = getRelativeNodePosition(nodes[i - 1], lane);
            Vec2f next = getRelativeNodePosition(nodes[i], lane);

            return last * percentage + next * (1 - percentage);
        }
    }

    // Should never happen
    return getRelativeNodePosition(nodes.back(), lane);
}

pair<size_t, size_t> Street::getNearestNodeIndices(const Vec2f& position) const {

    pair<size_t, size_t> nearestNodes;
    pair<double, double> minDistances(numeric_limits<double>::max(), numeric_limits<double>::max());

    for (unsigned int i = 0; i < nodes.size(); i++) {

        Vec2f nodePos = roadSystem->getNode(nodes[i])->getPosition();
        double distance = calcDistance(nodePos, position);
        if (distance < minDistances.first) {
            nearestNodes.second = nearestNodes.first;
            minDistances.second = minDistances.first;
            nearestNodes.first = i;
            minDistances.first = distance;
        } else if (distance < minDistances.second) {
            nearestNodes.second = i;
            minDistances.second = distance;
        }
    }

    if (nearestNodes.first + 1 != nearestNodes.second && nearestNodes.first - 1 != nearestNodes.second) {
        // Should be the case, but if not, fix it
        minDistances = make_pair(numeric_limits<double>::max(), numeric_limits<double>::max());
        if (nearestNodes.first + 1 < nodes.size())
            minDistances.first = calcDistance(position, roadSystem->getNode(nodes[nearestNodes.first + 1])->getPosition());
        if (nearestNodes.first > 1)
            minDistances.second = calcDistance(position, roadSystem->getNode(nodes[nearestNodes.first - 1])->getPosition());
        if (minDistances.first < minDistances.second)
            nearestNodes.second = nearestNodes.first + 1;
        else
            nearestNodes.second = nearestNodes.first - 1;
    }

    return nearestNodes;
}

void Street::applyTrafficDensity(const double density) {

    const double densityForType = (density / 20) * ((double)getType() / 100);

    // Do this for both directions
    for (int direction = -1; direction <= 1; direction += 2) {

        // If there are no lanes, skip this direction
        if (getLaneCount(direction) == 0)
            continue;

        // Calculate current and new density
        const double shouldBeVehicleCount = densityForType * getLaneMaxVehicleCount(direction);

        // While the amount is not matching, adapt it
        // Add vehicles if not full
        while (getLaneVehicleCount(direction) < shouldBeVehicleCount) {
            // Add vehicle with random arrival time
            createRandomVehicle(direction);
        }

        // Remove vehicles if too full
        while (getLaneVehicleCount(direction) > shouldBeVehicleCount) {
            removeRandomVehicle(direction);
        }
    }
}

Vehicle* Street::createRandomVehicle(const int direction, const double offset) {

    // If the street is full, abort
    if (getLaneVehicleCount(direction) >= getLaneMaxVehicleCount(direction))
        return NULL;

    // If there are no lanes abort, too
    if (getLaneCount(direction) == 0)
        return NULL;

    if (!getIsMicro()) {

        // Short case first: In the meso case, just add it somewhere


        // The first node that can be chosen, depending on the given offset
        size_t minNodeI = 0;
        while (nodeDistances[minNodeI] < offset)
            ++minNodeI;
        if (direction > 0 && minNodeI > 0)
            --minNodeI;

        // Select a (nearly) random start node for the vehicle
        size_t startNodeI;
        if (minNodeI < nodes.size() - 1) {

            startNodeI = (rand() % (nodes.size() - 1 - minNodeI)) + minNodeI;
            // Avoid the problem that the vehicle starts at the last node in its direction
            if (direction < 0)
                ++startNodeI;
        } else
            startNodeI = nodes.size() - 1;

        // Get the next interesting node in driving direction as destination
        size_t destNodeI = getNextInterestingNode(startNodeI, direction);

        // Get the distance between them
        double distance = nodeDistances[destNodeI] - nodeDistances[startNodeI];
        // If driving into the backward-direction, keep the distance positive
        if (distance < 0)
            distance *= -1;

        // Calculate the time needed to travel between the nodes
        time_duration travelTime = seconds(getLaneTravelTime(direction).total_seconds() * (distance / getLength())) + seconds(1);

        // Take a random part of it
        travelTime = seconds(rand() % travelTime.total_seconds());

        // Add the data to the arrival-list
        getLaneArrivalTimes(direction)->insert(make_pair(timer.getTime() + travelTime, nodes[destNodeI]));

        // Done, but meso always returns NULL
        return NULL;

    } else {

        // Pick a random lane
        int rLane = 1;
        if (getLaneCount(direction) > 1) {
            rLane = rand() % getLaneCount(direction) + 1;
        }
        int lane = rLane * (direction / abs(direction));
        do {
            // Check if there is space somewhere

            // Pick a random position
            double rOffset = rand() % static_cast<int>(getLength());
            if (offset >= 0)
                rOffset = offset;
            if (rOffset == 0 && direction < 0) rOffset = 0.001;
            if (rOffset == getLength() && direction > 0) rOffset = getLength() - 0.001;

            // Check rest of the street
            for (double offset = rOffset; offset < getLength(); offset += VEHICLE_LENGTH) {
                // Iterate over vehicles on lane and check distances
                Vec2f pos = getPositionOnLane(lane, offset);

                double minDistance = getNearestVehicleDistance(roadSystem, pos, this, 3 * VEHICLE_LENGTH);

                if (minDistance > 1.5 * VEHICLE_LENGTH) {


                    size_t prevNodeI, nextNodeI;

                    for (unsigned int i = 1; i < nodes.size(); i++) {

                        // If the offset is less than the distance to the next node, the position is on this part
                        if (offset <= nodeDistances[i]) {
                            prevNodeI = i - 1;
                            nextNodeI = i;
                            break;
                        }
                    }

                    if (direction < 0)
                        swap(prevNodeI, nextNodeI);


                    Vec2f dest = getRelativeNodePosition(nodes[nextNodeI], lane);


                    Vec2f vec = dest - pos;
                    if (vec == Vec2f(0, 0)) {
                        vec = Vec2f(1, 0);
                        dest = Vec2f(dest[0] + 0.01, dest[1]);
                    }

                    Quaternion rotation(Vec3f(1, 0, 0), Vec3f(vec[0], 0, vec[1]));


                    // Create a new vehicle
                    Vehicle *v = new Vehicle(roadSystem->getUnusedVehicleId(), Vec3f(pos[0], 0, pos[1]), rotation);
                    v->setController(0);
                    v->setStreet(getId(), lane);
                    v->setCurrentDestination(dest);
                    v->getRoute()->push_back(nodes[prevNodeI]);
                    v->getRoute()->push_back(nodes[nextNodeI]);
                    v->setCurrentSpeed(0);
                    v->setVehicleType(roadSystem->getRandomVehicleType());
                    v->setDriverType(roadSystem->getRandomDriverType());

                    if (!roadSystem->addVehicle(v)) {
                        delete v;
                        return NULL;
                    }

                    if (lane > 0)
                        forward.cars[lane - 1].insert(v->getId());
                    else
                        backward.cars[(-1 * lane) - 1].insert(v->getId());

                    return v;
                }
            } // End offset iteration
            if (getLaneCount(direction) > 1) {
                // Increase the lane to one further outside
                if (lane > 0) {
                    ++lane;
                    if (lane > (int)getLaneCount(1))
                        lane = 1;
                } else if (lane < 0) {
                    --lane;
                    if (lane < -1 * (int)getLaneCount(-1))
                        lane = -1;
                }
            }
        } while (lane != rLane * (direction / abs(direction)));
        // No space found
        return NULL;
    }
}

bool Street::removeRandomVehicle(const int direction) {

    if (getLaneArrivalTimes(direction)->empty())
        return false;

    if (!getIsMicro()) {

        // Remove random vehicle
        multiset< pair<ptime, ID> >::iterator iter2 = getLaneArrivalTimes(direction)->begin();
        int r = rand() % getLaneVehicleCount(direction);
        for (int i = 0; i < r; ++i) {
            ++iter2;
        }
        getLaneArrivalTimes(direction)->erase(iter2);
        return true;
    } else {

        // Pick a random lane
        int r = rand() % (getLaneCount(direction) - 1);
        int lane = r;
        do {
            // Check if there is a vehicle to remove
            set<ID> *vehicles;

            if (direction > 0)
                vehicles = &forward.cars[lane];
            else
                vehicles = &backward.cars[lane];

            if (!vehicles->empty()) {
                // Pick a random vehicle
                r = rand() % vehicles->size();
                set<ID>::iterator iter = vehicles->begin();
                for (int i = 0; i < r; ++i) {
                    ++iter;
                }
                // And remove it
                vehicles->erase(iter);
                return true;
            }
            lane = (lane + 1) % getLaneCount(direction);
        } while (lane != r);
        // Nothing to delete found: Return.
        return false;
    }
}

string Street::toString(const bool extendedOutput) const {

    string str = string("Street #") + lexical_cast<string>(id) + ((extendedOutput)?"\n  ":" [")
        + "lanes=" + lexical_cast<string>(forward.flags.size()) + "/" + lexical_cast<string>(backward.flags.size()) + ((extendedOutput)?"\n  ":"; ")
        + "maxSpeed=" + ((maxSpeed < 0)?"default:":"") + lexical_cast<string>(getMaxSpeed()) + ((extendedOutput)?"\n  ":"; ")
        + "flags=";

    for (unsigned int i = 0; i < forward.flags.size(); ++i) {
            if (forward.flags[i] & TURN_LEFT)
                str += "Tl";
            if (forward.flags[i] & TURN_RIGHT)
                str += "Tr";
            if (forward.flags[i] & TURN_THROUGH)
                str += "Tt";
            if (forward.flags[i] & CHANGE_LEFT)
                str += "Cl";
            if (forward.flags[i] & CHANGE_RIGHT)
                str += "Cr";
            str += "(" + lexical_cast<string>((int)forward.flags[i]) + ")";
            if (i < forward.flags.size() - 1)
                str += "|";
    }

    str += "/";

    for (unsigned int i = 0; i < backward.flags.size(); ++i) {
            if (backward.flags[i] & TURN_LEFT)
                str += "Tl";
            if (backward.flags[i] & TURN_RIGHT)
                str += "Tr";
            if (backward.flags[i] & TURN_THROUGH)
                str += "Tt";
            if (backward.flags[i] & CHANGE_LEFT)
                str += "Cl";
            if (backward.flags[i] & CHANGE_RIGHT)
                str += "Cr";
            str += "(" + lexical_cast<string>((int)backward.flags[i]) + ")";
            if (i < backward.flags.size() - 1)
                str += "|";
    }

    str += string((extendedOutput)?"\n  ":"; ") + "micro=" + ((flags & IS_MICRO)?"true":"false") + ((extendedOutput)?"\n  ":"; ")
        + "type=";

    if (type == DEFAULT)
            str += "default:";

    switch(getType()) {
        case MOTORWAY:
            str += "motorway";
            break;
        case TRUNK:
            str += "trunk";
            break;
        case PRIMARY:
            str += "primary";
            break;
        case SECONDARY:
            str += "secondary";
            break;
        case TERTIARY:
            str += "tertiary";
            break;
        case UNCLASSIFIED:
            str += "unclassified";
            break;
        case SERVICE:
            str += "service";
            break;
        case ROAD:
            str += "road";
            break;
        case TRACK:
            str += "track";
            break;
        case LIVING_STREET:
            str += "living_street";
            break;
        case RESIDENTIAL:
            str += "residential";
            break;
        default:
            str += "unknown";
    }
    str += ((extendedOutput)?"\n  ":"; ");

    str += "#nodes=" + lexical_cast<string>(nodes.size()) + ((extendedOutput)?":\n    ":"");
    for (unsigned int i = 0; i < nodes.size() && extendedOutput; ++i)
        str += lexical_cast<string>(nodes[i]) + ((i < nodes.size() - 1)?"\n    ":"\n  ");

    str += "#Vehicles=" + lexical_cast<string>(getLaneVehicleCount(1)) + "/" + lexical_cast<string>(getLaneMaxVehicleCount(1)) + " / "
        + lexical_cast<string>(getLaneVehicleCount(-1)) + "/" + lexical_cast<string>(getLaneMaxVehicleCount(-1)) + ((extendedOutput)?":\n  ":"");

    if (extendedOutput && !getIsMicro()) {
        cout << "  Vehicle positions forward:\n";
        for (multiset< pair<ptime, ID> >::iterator iter = forward.arrivalTimes.begin(); iter != forward.arrivalTimes.end(); ++iter) {
            cout << "    To " << iter->second << " arrival in " << ((iter->first - timer.getTime())).total_seconds() << " seconds\n";
        }
        cout << "  Vehicle positions backward:\n";
        for (multiset< pair<ptime, ID> >::iterator iter = backward.arrivalTimes.begin(); iter != backward.arrivalTimes.end(); ++iter) {
            cout << "    To " << iter->second << " arrival in " << ((iter->first - timer.getTime())).total_seconds() << " seconds\n";
        }
    }

    str += ((extendedOutput)?"":"]");

    return str;
}

bool Street::changeVehicleLane(Vehicle *vehicle, const int newLane) {

    // Update reference to vehicle
    removeVehicle(vehicle->getId(), vehicle->getLaneNumber());
    if (newLane > 0)
        forward.cars[newLane - 1].insert(vehicle->getId());
    else if (newLane < 0)
        backward.cars[(-1 * newLane) - 1].insert(vehicle->getId());

    // Calculate offset of vehicle on the street
    double nodeOffset = nodeDistances[getNodeIndex(vehicle->getRoute()->at(1))];
    double distanceVehicleNode = calcDistance(vehicle->getPosition(), roadSystem->getNode(vehicle->getRoute()->at(1))->getPosition());
    double offset;
    double distanceDestination = calcDistance(vehicle->getPosition(), vehicle->getCurrentDestination());
    // This is the distance in which the vehicle tries to change its lane
    double changingRange = min(20.0, distanceDestination);
    if (vehicle->getLaneNumber() > 0) {
        offset = nodeOffset - distanceVehicleNode + changingRange;
    } else {
        offset = nodeOffset + distanceVehicleNode - changingRange;
    }

    Vec2f newDest = getPositionOnLane(newLane, offset);
    vehicle->setCurrentDestination(newDest);

    vehicle->setStreet(id, newLane);

    return true;
}

Vehicle* Street::transferVehicle(Street *destStreet, const ID node, const int destLane, Vehicle *vehicle) {

    // Find the node on the other street
    size_t nodeI = destStreet->getNodeIndex(node);
    // If not found, abort
    if (nodeI > destStreet->nodes.size()) {
        return NULL;
    }

    if (destStreet->getIsMicro()) {
        // Destination street is in micro mode

        // Three cases:
        // A) It might be the current street and we are continuing to drive into the same direction
        // b) We reached the end of the street (or so) and are now driving in the opposite direction
        // C) We are changing to another street
        // while B and C are the same code

        if (destStreet->getId() == id && destLane == vehicle->getLaneNumber()) {

            // Update the route and set a new destination
            vehicle->getRoute()->pop_front();
            vehicle->setCurrentDestination(destStreet->getRelativeNodePosition(vehicle->getRoute()->at(1), destLane));

            return vehicle;

        } else {

            // Remove vehicle from this lane
            if (vehicle->getLaneNumber() > 0)
                forward.cars[vehicle->getLaneNumber() - 1].erase(vehicle->getId());
            else if (vehicle->getLaneNumber() < 0)
                backward.cars[(-1 * vehicle->getLaneNumber()) - 1].erase(vehicle->getId());

            // Drop a part of the route
            vehicle->getRoute()->pop_front();

            // Assign new street to vehicle
            vehicle->setStreet(destStreet->getId(), destLane);
            Vec2f oldDest = destStreet->getRelativeNodePosition(vehicle->getRoute()->at(0), destLane);
            Vec2f newDest = destStreet->getRelativeNodePosition(vehicle->getRoute()->at(1), destLane);
            vehicle->setCurrentDestination(oldDest * 0.9 + newDest * 0.1);

            // Assign vehicle to new street
            if (destLane > 0)
                destStreet->forward.cars[destLane - 1].insert(vehicle->getId());
            else
                destStreet->backward.cars[-1 * destLane - 1].insert(vehicle->getId());

            return vehicle;

        }
    } else {
        // Destination street is in meso mode

        // Remove all traces of the micro-vehicle
        if (vehicle->getLaneNumber() > 0)
            forward.cars[vehicle->getLaneNumber() - 1].erase(vehicle->getId());
        else if (vehicle->getLaneNumber() < 0)
            backward.cars[(-1 * vehicle->getLaneNumber()) - 1].erase(vehicle->getId());
        roadSystem->removeVehicle(vehicle->getId());

        // Get the next crossing into the direction of the vehicle
        size_t destNodeI = destStreet->getNextInterestingNode(nodeI, destLane);

        // Calculate the time to get there
        double travelTime = destStreet->getLaneTravelTime(destLane).total_seconds()
            * (destStreet->getNodeDistanceI(nodeI, destNodeI) / destStreet->getLength());

        // Add a meso-vehicle
        destStreet->getLaneArrivalTimes(destLane)->insert(make_pair(timer.getTime() + seconds(travelTime), destStreet->nodes[destNodeI]));

        return NULL;
    }
}

bool Street::transferVehicle(Street *destStreet, const ID node, const int destLane) {

    // Find the node on the other street
    // We "should" check if the node is part of this street, too, but that is not so important
    size_t nodeI = destStreet->getNodeIndex(node);
    // If not found, abort
    if (nodeI > destStreet->nodes.size()) {
        return false;
    }

    if (destStreet->getIsMicro()) {
        // Destination street is in micro mode

        // Create a new vehicle for the street

        // Check if there is space

        // Calculate the position of the vehicle if it would be added
        Vec2f newPos = destStreet->getRelativeNodePosition(node, destLane);

        double minDistance = getNearestVehicleDistance(roadSystem, newPos, this, 3 * VEHICLE_LENGTH);

        // If there is a big enough gap at the moment, add a new vehicle
        if (minDistance > 1.5 * VEHICLE_LENGTH) {

            // Put a new vehicle in the gap

            // Find out which is the next node
            size_t nextNodeI = nodeI;
            if (destLane > 0) {
                assert(nextNodeI < destStreet->nodes.size() - 1);
                ++nextNodeI;
            } else {
                assert(nextNodeI > 0);
                --nextNodeI;
            }


            Vec2f dest = destStreet->getRelativeNodePosition(destStreet->nodes[nextNodeI], destLane);

            Vec2f vec = dest - newPos;
            if (vec == Vec2f(0, 0)) {
                vec = Vec2f(1, 0);
                dest = Vec2f(dest[0] + 0.01, dest[1]);
            }

            Quaternion rotation(Vec3f(1, 0, 0), Vec3f(vec[0], 0, vec[1]));

            // Create a new vehicle
            Vehicle *v = new Vehicle(roadSystem->getUnusedVehicleId(), Vec3f(newPos[0], 0, newPos[1]), rotation);
            v->setController(0);
            v->setStreet(destStreet->getId(), destLane);
            v->getRoute()->push_back(node);
            v->getRoute()->push_back(destStreet->nodes[nextNodeI]);
            Vec2f oldDest = destStreet->getRelativeNodePosition(node, destLane);
            Vec2f newDest = destStreet->getRelativeNodePosition(destStreet->nodes[nextNodeI], destLane);
            v->setCurrentDestination(oldDest * 0.9 + newDest * 0.1);
            v->setCurrentSpeed(0);
            v->setVehicleType(roadSystem->getRandomVehicleType());
            v->setDriverType(roadSystem->getRandomDriverType());

            if (!roadSystem->addVehicle(v)) {
                delete v;
                return false;
            }

            if (destLane > 0)
                destStreet->forward.cars[destLane - 1].insert(v->getId());
            else
                destStreet->backward.cars[-1 * destLane - 1].insert(v->getId());

            return true;
        }

        // No gap found
        return false;

    } else {
        // Destination street is in meso mode

        // Get the next crossing into the direction of the vehicle
        size_t destNodeI = destStreet->getNextInterestingNode(nodeI, destLane);

        // Calculate the time to get there
        double travelTime = destStreet->getLaneTravelTime(destLane).total_seconds()
            * (destStreet->getNodeDistanceI(nodeI, destNodeI) / destStreet->getLength());

        // Add a meso-vehicle
        destStreet->getLaneArrivalTimes(destLane)->insert(make_pair(timer.getTime() + seconds(travelTime), destStreet->nodes[destNodeI]));

        return true;
    }
}

double Street::getRoutingCost() const {
    // type and maxSpeed could be "default" (-1 for maxSpeed) so the functions have to be used
    return getType() * (forward.flags.size() + backward.flags.size()) * (getMaxSpeed() / 100);
}

double Street::getNodeDistance(const ID start, const ID end, const bool directed) const {

    return getNodeDistanceI(getNodeIndex(start), getNodeIndex(end), directed);
}

double Street::getNodeDistanceI(const size_t startI, const size_t endI, const bool directed) const {

    assert(startI < nodes.size() && endI < nodes.size());

    // Check whether the indices are valid
    if (startI >= nodes.size() || endI >= nodes.size()) // <0 check not needed since unsigned
        return numeric_limits<double>::max();

    // The distance traveled
    double distance = nodeDistances[endI] - nodeDistances[startI];

    if (!directed && distance < 0)
        distance *= -1;

    if (distance == 0)
        distance = 0.1;

    return distance;
}

size_t Street::getNodeIndex(ID node) const {
    size_t nodeI = 0;
    for (; nodeI < nodes.size(); ++nodeI) {
        if (nodes[nodeI] == node)
            break;
    }
    // If not found, return an error
    if (nodeI >= nodes.size())
        return nodes.size() * 2;
    return nodeI;
}


size_t Street::getNodeIndexFromBack(ID node) const {
    size_t nodeI = nodes.size();
    for (; nodeI > 0; --nodeI) {
        if (nodes[nodeI - 1] == node)
            break;
    }
    // If not found, return an error
    if (nodeI == 0)
        return nodes.size() * 2;
    return nodeI;
}
