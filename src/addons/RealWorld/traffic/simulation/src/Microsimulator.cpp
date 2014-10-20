#include "Microsimulator.h"

#include <boost/lexical_cast.hpp>

#include "Routing.h"

void Microsimulator::handleVehicle(Vehicle *vehicle, bool moveOnly) {

    // Distance to current destination
    const double destinationDistance = calcDistance(vehicle->getPosition(), vehicle->getCurrentDestination());

    // Check if the route is long enough
    // The first three entries of the route are accessed, so they have to exist
    // Do the check at this position so newly created vehicles get their route extended immediately
    deque<ID>* route = vehicle->getRoute();
    if (route->size() < 3)
        extendRoute(vehicle);

    Street *currentStreet = roadSystem->getStreet(vehicle->getStreetId());

    // Remove old node reservations
    Node *lastNode = roadSystem->getNode(route->at(0));
    if (lastNode->getReservation() == vehicle->getId()) {
        lastNode->setReservation(0);
    }

    Node *reachedNode = roadSystem->getNode(route->at(1));
    if (reachedNode->getReservation() == vehicle->getId()) {
        if(vehicle->getState() & (Vehicle::BLOCKED | Vehicle::WAITING) && vehicle->getCurrentSpeed() < 5)
            reachedNode->setReservation(0);
    }

    // If near the destination or already through it, change to the next destination
    const bool drivenPast = (destinationDistance < calcDistance(vehicle->getFuturePosition(), vehicle->getCurrentDestination()));
    if (destinationDistance < 5 || (vehicle->getCurrentSpeed() > 0 && drivenPast)) {


        // If still too short, delete the vehicle
        if (route->size() < 3) {
            // No next-node: We are at a one-way dead-end
            // Just delete the vehicle
            roadSystem->addOffmapVehicle();

            if (!currentStreet->removeVehicle(vehicle->getId(), vehicle->getLaneNumber()))
                cout << "ERROR: Could not remove the vehicle from its street.\n";

            // Remove a possible reservation for the reached node
            if (reachedNode->getReservation() == vehicle->getId())
                reachedNode->setReservation(0);

            roadSystem->removeVehicle(vehicle->getId());
            return;
        }

        // Only for better readability
        const ID reachedNodeId = route->at(1);

        const Vec2f positionNearReachedNode = currentStreet->getRelativeNodePosition(reachedNodeId, vehicle->getLaneNumber());

        // Check if it is a left-turn. In that case, drive past the node to simulate a big turning
        const Node *nextNode = roadSystem->getNode(route->at(2));
        int turningAngle = (int)(calcAngle(nextNode->getPosition() - lastNode->getPosition()) - calcAngle(positionNearReachedNode - lastNode->getPosition()) + 720) % 360;
        const bool leftTurn = turningAngle > 175;

        // Check if the current destination has been the node
        if (calcDistance(vehicle->getPosition(), positionNearReachedNode) < 5 && (drivenPast || !leftTurn)) {

            // If it is, the node has been reached and we can continue to the next node

            const ID nextNodeId = route->at(2);

            // Find the street and direction that is going to the next node
            pair<ID, int> nextStreet = reachedNode->findNextStreet(nextNodeId);
            Street *nextStreetPtr = roadSystem->getStreet(nextStreet.first);
            if (nextStreetPtr == NULL) {
                // Seems like the road is no longer existing
                // Clear the route to enforce a new destination and try again next tick
                route->resize(2);
                return;
            }

            // Select a matching lane number for the next street
            int destLane = selectNextLaneNumber(vehicle, nextStreetPtr, nextStreet.second);

            ID vehicleId = vehicle->getId();

            // The route is long enough and we are near the destination, so move to next street segment
            if (currentStreet->transferVehicle(nextStreetPtr, route->at(1), destLane, vehicle) == NULL) {
                // If the method returns NULL, the vehicle is moved to a meso-street and has no longer be cared for

                // Remove a possible reservation for the reached node
                if (reachedNode->getReservation() == vehicleId)
                    reachedNode->setReservation(0);
                return;
            }

            // Update the current street
            currentStreet = roadSystem->getStreet(vehicle->getStreetId());

        } else {

            // If we are not near the node, set the destination near the destination node and drive on
            vehicle->setCurrentDestination(positionNearReachedNode);
        }
    } // end if(near-destination)

    if (!moveOnly && route->size() > 2) {
        int optLane = findOptimalLaneNumber(vehicle, currentStreet);
        if (optLane != vehicle->getLaneNumber()) {
            currentStreet->changeVehicleLane(vehicle, optLane);
        }
    }

    // Apply current rotation
    vehicle->setOrientation(vehicle->getOrientation() * vehicle->getCurrentRotation());

    const VehicleType *type = roadSystem->getVehicleType(vehicle->getVehicleType());

    // Calculate new rotation
    const Vec3f dest = toVec3f(vehicle->getCurrentDestination());
    const Vec3f& pos  = vehicle->getPosition();

    // Calculate rotation needed to aim at destination
    Vec3f a(1, 0, 0);
    vehicle->getOrientation().multVec(a,a);
    Vec3f b(Vec3f(dest - pos));
    Quaternion neededRotation(a, b);

    float angle; // Seems to be in the range [0, Pi]
    Vec3f axis;
    neededRotation.getValueAsAxisRad(axis, angle);

    // Fix against strange behavior of quaternion
    if (axis[1] < 0)
        axis = Vec3f(0, -1, 0);
    else
        axis = Vec3f(0, 1, 0);

    // If the angle is quite small, do nothing
    if (angle > 0.001) {

        // Only apply rotation if we are moving forward, e.g. by restricting the allowed turning angle depending on the current speed
        angle *= min(vehicle->getCurrentSpeed() / 100, 1.0);

        // Make sure the rotation is less than the physically possible rotation
        if (angle <= 0.5 * M_PI && angle > type->getMaxRotation() / 360 * M_PI) {
            angle = type->getMaxRotation() / 360 * M_PI;
        } else if (angle > 0.5 * M_PI && angle < M_PI - (type->getMaxRotation() / 360 * M_PI)) {
            angle = M_PI - (type->getMaxRotation() / 360 * M_PI);
        }

        neededRotation = Quaternion(axis, angle);

        vehicle->setCurrentRotation(neededRotation);
    }


    if (!moveOnly) {

        // Clear all states that can be set at this point
        vehicle->setState(vehicle->getState() & ~(Vehicle::ACCELERATING | Vehicle::WAITING | Vehicle::BLOCKED | Vehicle::BRAKING | Vehicle::COLLIDING));

        // Calculate new speed for vehicle
        pair<double, SPEEDCHANGE> optSpeed = calculateOptimalSpeed(vehicle);

        vehicle->setDesiredSpeed(optSpeed.first);

        // Adapt vehicle state
        if (optSpeed.second & ACTION_BRAKING) {
            vehicle->setState(vehicle->getState() | Vehicle::BRAKING);
        }
        if (optSpeed.second & REASON_DESTINATION) {
            // If waiting at a node, wait for up to 4 seconds
            vehicle->setController(rand() % 40);
            vehicle->setState(vehicle->getState() | Vehicle::WAITING);
        }
        if (optSpeed.second & REASON_OTHERVEHICLES) {
            vehicle->setState(vehicle->getState() | Vehicle::BLOCKED);
        }
        if (optSpeed.second & ACTION_ACCELERATING) {
            vehicle->setState(vehicle->getState() | Vehicle::ACCELERATING);
        }
    }


    // Adaption of the current speed
    double newSpeed = vehicle->getDesiredSpeed();
    // Get the maximal acceleration that is possible
    double maxAcc = type->getMaxAcceleration() * ((double)timer.getDelta().total_milliseconds() / 1000);
    // While setting the new speed, stay within the possible acceleration/braking of the current speed
    if ((vehicle->getCurrentSpeed() + maxAcc * 2) < newSpeed) {
        newSpeed = (vehicle->getCurrentSpeed() + maxAcc * 2);
    } else if ((vehicle->getCurrentSpeed() - sqrt(vehicle->getCurrentSpeed()) * 2) > newSpeed && newSpeed >= 0) {
        newSpeed = (vehicle->getCurrentSpeed() - sqrt(vehicle->getCurrentSpeed()) * 2);
    }

    if (newSpeed < 1)
        newSpeed = 0;

    vehicle->setCurrentSpeed(newSpeed);


    // Move vehicle forward
    Vec3f movement(vehicle->getCurrentSpeed() * kmhToUnits, 0, 0);
    vehicle->getOrientation().multVec(movement, movement);

    vehicle->setPosition(vehicle->getPosition() + movement);


    // Store future movement of vehicle
    // One step more to ease collision avoidance

    movement = Vec3f((vehicle->getCurrentSpeed() + 1) * kmhToUnits, 0, 0);
    vehicle->getOrientation().multVec(movement, movement);
    vehicle->setFuturePosition(vehicle->getPosition() + movement);
}


void Microsimulator::extendRoute(Vehicle *vehicle) {

    // Select a random node
    // Calculate the vector from the current position to the last entry in the routing list.
    // If there is no entry, create the vector based on the current destination.
    // Now, pick a random node some distance away in the direction where the vehicle is driving into.
    // If it is far enough away and there is a route to it, add the route. Otherwise, increase
    // the searching angle, reduce the min-distance and try again.
    deque<ID> *route = vehicle->getRoute();

    // The nodes to start routing from
    const ID routingFrom = route->at(route->size() - 2);
    const ID routingStart = route->back();
    const ID destinationNode = getRandomDestinationNode(roadSystem, routingFrom, routingStart);

    if (routingStart == destinationNode) {
        return;
    }

    // Now, calculate a route to it
    vector<ID> newRoute = calculateRoute(roadSystem, routingFrom, routingStart, destinationNode);
    // If the route is empty, one of the two methods does not work correctly
    assert(!newRoute.empty());
    // We have a route: Add it to the vehicle-route
    for (size_t i = 1; i < newRoute.size(); ++i)
        route->push_back(newRoute[i]);

}

int Microsimulator::selectNextLaneNumber(const Vehicle *vehicle, const Street *otherStreet, const int otherDirection) const {

    if (!otherStreet->getIsMicro()) {
        // If the next street is a meso-street, all this doesn't matters
        return otherDirection;
    }

    const deque<ID> *route = const_cast<Vehicle*>(vehicle)->getRoute(); // My first const_cast<>() :D

    // If we stay on this street, keep the lane number
    if (otherStreet->getId() == vehicle->getStreetId()) {
        // Check if we are turning while in a dead-end
        if (route->at(0) == route->at(2))
            return -1 * vehicle->getLaneNumber();
        else
            return vehicle->getLaneNumber();
    }

    // Find out our direction
    const Street *ownStreet = roadSystem->getStreet(vehicle->getStreetId());
    int ownDirection = (vehicle->getLaneNumber() > 0) ? 1 : -1;
    size_t laneCount = otherStreet->getLaneCount(otherDirection);

    // If the next street has the same amount of lanes and is the only other street at this node, stay on the current lane
    const Node *reachedNode = roadSystem->getNode(route->at(1));
    if (reachedNode->getStreetIds().size() <= 2) {
        if (ownStreet->getLaneCount(ownDirection) == laneCount) {
            if (ownDirection == otherDirection)
                return vehicle->getLaneNumber();
            else
                return -1 * vehicle->getLaneNumber();
        }
    }


    // If we are entering an other street, select a "good" lane


    // Small shortcut for a ... quite common case ;)
    if (laneCount == 1)
        return otherDirection;

    // Create a set with all possible lanes
    // The double-value is used to store the minimum-distance
    // of another vehicle to the beginning of the street
    set< pair<double, size_t> > lanes;
    for (size_t i = 1; i <= laneCount; ++i)
        lanes.insert(make_pair(numeric_limits<double>::max(), i));


    // === First criteria: For which lanes is the turning-cycle of the vehicle small enough? === //

    // Retrieve the maximal possible rotation
    double maxPossibleRotation = roadSystem->getVehicleType(vehicle->getVehicleType())->getMaxRotation();

    // Calculate the angle between the streets
    const double ownAngle = ((int)calcAngle(roadSystem->getNode(route->at(0))->getPosition() - roadSystem->getNode(route->at(1))->getPosition()) + 180) % 360;
    const double otherAngle = calcAngle(roadSystem->getNode(route->at(2))->getPosition() - roadSystem->getNode(route->at(1))->getPosition());
    double angle = (int)(ownAngle - otherAngle + 360) % 360;
    if (angle > 180)
        angle = 360 - angle;


    // Throw out lanes which are so near that we can not reach them because of the turning-cycle
    for (set< pair<double, size_t> >::iterator setIter = lanes.begin(); setIter != lanes.end();) {

        double ratio;
        if (otherDirection == ownDirection) {
            ratio = (ownStreet->getLaneCount(ownDirection) - ownDirection * vehicle->getLaneNumber() + 1) / (laneCount - setIter->second + 1);
        } else {
            ratio = (ownDirection * vehicle->getLaneNumber()) / setIter->second;
        }

        if ((angle / 2) / ratio > maxPossibleRotation)
            lanes.erase(setIter++);
        else
            ++setIter;

        if (lanes.size() == 1)
            return otherDirection * lanes.begin()->second;
    }


    // === Second criteria: Which lanes already have vehicles at their beginning? === //

    // Check if the start of the lane is empty
    for (set< pair<double, size_t> >::iterator setIter = lanes.begin(); setIter != lanes.end();) {

        Vec2f laneStartPosition = otherStreet->getRelativeNodePosition(route->at(1), setIter->second);

        // Iterate over the vehicles on this lane
        const set<ID> *vehicles = otherStreet->getLaneVehicles(setIter->second);
        set< pair<double, size_t> >::iterator currentIter = setIter++;

        for (set<ID>::iterator vehicleIter = vehicles->begin(); vehicleIter != vehicles->end(); ++vehicleIter) {

            Vehicle *laneVehicle = roadSystem->getVehicle(*vehicleIter);
            double crossingDist = calcDistance(laneVehicle->getPosition(), laneStartPosition);
            if (crossingDist < CROSSROAD_RADIUS * 2) {
                // The vehicle is near the start so it blocks the lane
                lanes.erase(currentIter);
                break;
            }
        }

        if (lanes.size() == 1)
            return otherDirection * lanes.begin()->second;
    }


    // === Third criteria: Which lane is nearest to our current lane? === //
    // This tries to minimize (traffic-) problems with vehicles on neighbor-lanes

    // Of the lanes that are still available, select one which is near to our current lane number
    int min = numeric_limits<int>::max();
    int lane = otherDirection;
    for (set< pair<double, size_t> >::iterator iter = lanes.begin(); iter != lanes.end(); ++iter) {
        if (abs(vehicle->getLaneNumber() - iter->second) < min) {
            min = abs(vehicle->getLaneNumber() - iter->second);
            lane = otherDirection * iter->second;
        }
    }
    return lane;
}

int Microsimulator::findOptimalLaneNumber(Vehicle *vehicle, const Street *street) const {

    // If there is nothing to turn to, stay on the current lane
    if (street->getLaneCount(vehicle->getLaneNumber()) < 2)
        return vehicle->getLaneNumber();

    // Get the possible lanes
    const set<int> turningLanes = roadSystem->getNode(vehicle->getRoute()->at(1))->calculatePossibleLanes(street->getId(), vehicle->getLaneNumber(), vehicle->getRoute()->at(2));

    if (turningLanes.count(vehicle->getLaneNumber()) > 0)
        return vehicle->getLaneNumber();

    // Set with all acceptable lanes
    set<int> lanes;

    // Add neighbor-lanes if they are in turningLanes
    // Range checks are not needed since turningLanes only contains valid lanes anyway
    if (turningLanes.count(vehicle->getLaneNumber() + 1))
        lanes.insert(vehicle->getLaneNumber() + 1);
    if (turningLanes.count(vehicle->getLaneNumber() - 1))
        lanes.insert(vehicle->getLaneNumber() - 1);

    if (turningLanes.empty()) {
        // No turning possible?? Strange...
        return vehicle->getLaneNumber();
    }

    // If the set is still empty, add neighbor-lanes in direction of a turningLane
    // The direction in which the vehicle is driving at the moment does not matter (beautiful, isn't it?)
    if (lanes.empty()) {
        if (*(turningLanes.begin()) < vehicle->getLaneNumber())
            lanes.insert(vehicle->getLaneNumber() - 1);
        if (*(turningLanes.rbegin()) > vehicle->getLaneNumber())
            lanes.insert(vehicle->getLaneNumber() + 1);
    }

    // Find out if changing is allowed and drop lanes where it is not
    if (!(street->getLaneFlags(vehicle->getLaneNumber()) & ((vehicle->getLaneNumber()) ? Street::CHANGE_LEFT : Street::CHANGE_RIGHT)))
        lanes.erase(vehicle->getLaneNumber() - 1);
    if (!(street->getLaneFlags(vehicle->getLaneNumber()) & ((vehicle->getLaneNumber()) ? Street::CHANGE_RIGHT : Street::CHANGE_LEFT)))
        lanes.erase(vehicle->getLaneNumber() + 1);

/*    const DriverType *driver = roadSystem->getDriverType(vehicle->getDriverType());

    // If the vehicle is blocked at the moment, check if one of the neighbor-lanes is free
    if (vehicle->getState() & Vehicle::BLOCKED) {
        for (set<int>::iterator iter = lanes.begin(); iter != lanes.end();) {
            pair<const Vehicle*, double> nearestVehicle = findNearestVehicleOnLane(vehicle, 130, street, *iter);
            if (nearestVehicle.first != NULL && nearestVehicle.second < 10 + 10 * driver->getCautiousness()) {
                lanes.erase(iter++);
            } else
                ++iter;
        }
    } else {
    Not quite done...
*/
    // Add current lane if it matches the desired turning
    if (turningLanes.count(vehicle->getLaneNumber()))
        lanes.insert(vehicle->getLaneNumber());

    if (lanes.empty()) {
        // If there are no lanes left, return the current lane
        return vehicle->getLaneNumber();
    } else {
        // Pick a random lane out of the set
        int randI = rand() % lanes.size();
        int i = 0;
        for (set<int>::iterator iter = lanes.begin(); iter != lanes.end(); ++iter) {
            if (i >= randI)
                return *iter;
            ++i;
        }
        // Should never be reached
        return vehicle->getLaneNumber();
    }
}

pair<double, Microsimulator::SPEEDCHANGE> Microsimulator::calculateOptimalSpeed(Vehicle *vehicle) const {

#ifdef WITH_GUI
    vehicle->speedInfluences = "\n";
#endif // WITH_GUI

    const VehicleType *type  = roadSystem->getVehicleType(vehicle->getVehicleType());
    const DriverType *driver = roadSystem->getDriverType(vehicle->getDriverType());
    const Street *street = roadSystem->getStreet(vehicle->getStreetId());

    // Start with maximal speed of the vehicle
    double newSpeed = type->getMaxSpeed() * kmhToUnits;
    SPEEDCHANGE reason = REASON_VEHICLELIMIT;

    // Reduce speed in sharp turns
    float angle;
    Vec3f axis;
    vehicle->getCurrentRotation().getValueAsAxisRad(axis, angle);
    // Transfer angle to degree
    angle = angle * 180 / M_PI;
    if (angle > type->getMaxRotation() / 4) {
#ifdef WITH_GUI
        vehicle->speedInfluences += "    turningReduction from "  + boost::lexical_cast<string>(newSpeed);
#endif // WITH_GUI
        // A turn of 90 degree is allowed with 30 km/h
        newSpeed = min(newSpeed, max(10.0, 10.0 * (type->getMaxRotation() / angle)) * kmhToUnits);
#ifdef WITH_GUI
        vehicle->speedInfluences += " to "  + boost::lexical_cast<string>(newSpeed) + " at " + boost::lexical_cast<string>(angle) + " degree\n";
#endif // WITH_GUI
    }

    // Maximal speed of the street
    double allowedSpeed = street->getMaxSpeed() * kmhToUnits;
    // Add something for speeding
    if (rand() % 100 <= driver->getLawlessness() * 100)
        allowedSpeed *= 1.2;
    if (allowedSpeed < newSpeed) {
        newSpeed = allowedSpeed;
        reason = REASON_STREETLIMIT;
    }

    // Random variation
    // Add/Take a random 5% variation
    double variation = ((rand() % 10) - 5) / 100;
    newSpeed *= 1 + variation;



    // The distance needed to come to a full stop in meter (simple driving school approximation)
    // Includes reaction time
    double distStop = pow(vehicle->getCurrentSpeed() / 10, 2) + (vehicle->getCurrentSpeed() / 10) * 3;


    // Distance to destination: brake when near it
    // Normally this should not happen since the destination is reset on time. If it is not,max
    // the node does not allow entering at the moment so stop in front of it

    double nearestBlockingNode = findFirstBlockingNode(vehicle, distStop, max(distStop * 2, 100.0));


    distStop = max(distStop, 5.59); // = (13.0 / 10.0) * (13.0 / 10.0) + (13.0 / 10.0) * 3.0; // With reaction time and min-value

    double vehicleSearchDistance = 20 + distStop * 2;

    if (nearestBlockingNode <= distStop * 1.1 || nearestBlockingNode < 7) {
        // Stop in front of the node
        newSpeed = 0;
        vehicleSearchDistance = nearestBlockingNode;
        reason = REASON_DESTINATION;

    }

    // Store the minimal distance to an obstacle where the vehicle has to stop at
    // This is the distance between the center of the vehicle and the obstacle
    double minStopDistance = nearestBlockingNode;


    vehicle->getWaitingFor().clear();




    Vec2f p = toVec2f(vehicle->getPosition());
    Vec2f r = ((toVec2f(vehicle->getFuturePosition()) + vehicle->getCurrentDestination()) / 2) - toVec2f(vehicle->getPosition()) * 100; // *100 results in a 10-second line


    set<pair<double, Vehicle*> > nearVehicles;

    // After being blocked for some time ignore other vehicles,
    // so only search for them if the time is not long enough yet
    if (vehicle->getBlockedForTicks() < ticksTillIgnoreOthers)
        nearVehicles = findNearVehicles(vehicle, vehicleSearchDistance);

    Vehicle *nearestBlockingVehicle = NULL;

#ifdef WITH_GUI
    vehicle->nearVehicles.clear();
    for (set<pair<double, Vehicle*> >::iterator vehicleIter = nearVehicles.begin(); vehicleIter != nearVehicles.end(); ++vehicleIter)
        vehicle->nearVehicles.insert(make_pair(vehicleIter->first, vehicleIter->second->getId()));
#endif

    int nDone = 0;
    for (set<pair<double, Vehicle*> >::iterator vehicleIter = nearVehicles.begin(); vehicleIter != nearVehicles.end(); ++vehicleIter) {

        // Only check the nearest ones
        if (nDone > 10 && vehicleIter->first > vehicle->getCurrentSpeed() + (vehicle->getId() % 50))
            break;
        ++nDone;

        if (vehicleIter->second->getWaitingFor().count(vehicle->getId()) > 0 && vehicleIter->second->getCurrentSpeed() == 0) {
#ifdef WITH_GUI
            vehicle->speedInfluences += "    ignoring "  + boost::lexical_cast<string>(vehicleIter->second->getId()) + "\n";
#endif // WITH_GUI
            continue;
        }

        // Check if we should stop because the vehicle is near
        double distance = vehicleIter->first;
        double futureDistance = calcDistance(vehicle->getFuturePosition(), vehicleIter->second->getPosition());

        double radius = type->getRadius();
        double otherRadius = roadSystem->getVehicleType(vehicleIter->second->getVehicleType())->getRadius();

        double deltaAngle = 360 - (int)(calcAngle(toVec2f(vehicle->getFuturePosition() - vehicle->getPosition()))
                                  - calcAngle(toVec2f(vehicleIter->second->getPosition() - vehicle->getPosition())) + 720) % 360;


        // Calculate distance to future course of this vehicle
        double dx = vehicle->getCurrentDestination()[0] - vehicle->getPosition()[0];
        double dy = vehicle->getCurrentDestination()[1] - vehicle->getPosition()[2];

        double a = dy;
        double b = -dx;

        double c = -(a * vehicle->getCurrentDestination()[0] + b * vehicle->getCurrentDestination()[1]);

        double distanceToRoute = abs((a*vehicleIter->second->getPosition()[0] + b*vehicleIter->second->getPosition()[2] + c) / sqrt(a*a + b*b));


        // Calculate time until collision
        Vec2f q = toVec2f(vehicleIter->second->getPosition());
        Vec2f s = toVec2f(vehicleIter->second->getFuturePosition() - vehicleIter->second->getPosition()) * 100;

        Vec2f qp = q - p;

        // Position on course of current vehicle
        double t = (qp[0] * s[1] - qp[1] * s[0]) / (r[0] * s[1] - r[1] * s[0]);

        // Position on course of other vehicle
        double u = (qp[0] * r[1] - qp[1] * r[0]) / (r[0] * s[1] - r[1] * s[0]);


        // Calculate the angle between the driving directions
        int deltaDirections = (int)(calcAngle(toVec2f(vehicle->getFuturePosition() - vehicle->getPosition())) - calcAngle(toVec2f(vehicleIter->second->getFuturePosition() - vehicleIter->second->getPosition())) + 720) % 180;
        if (deltaDirections > 90) deltaDirections = 180 - deltaDirections;

        // If they are driving (nearly) parallel, reduce the radius
        if ((deltaDirections < 15) && distanceToRoute > radius) {
            const double oldDist = distance;
            distance += radius + otherRadius - 2 * 1.25; // 2.5m is the maximal allowed vehicle-width
#ifdef WITH_GUI
            vehicle->speedInfluences += "    applied parallel reduction of "  + boost::lexical_cast<string>(distance - oldDist) + " for vehicle #" + boost::lexical_cast<string>(vehicleIter->second->getId()) + "\n";
#endif // WITH_GUI
        } else if (deltaDirections > 80 && deltaDirections < 100) {
            const double oldDist = distance;
            distance += radius - 1.25; // 2.5m is the maximal allowed vehicle-width
#ifdef WITH_GUI
            vehicle->speedInfluences += "    applied orthogonal reduction of "  + boost::lexical_cast<string>(distance - oldDist) + " for vehicle #" + boost::lexical_cast<string>(vehicleIter->second->getId()) + "\n";
#endif // WITH_GUI
        }


        // Calculate distance from vehicle to point of collision
        double distanceCollisionPoint = distance;
        if (deltaDirections > 30)
            distanceCollisionPoint = calcDistance(vehicle->getPosition(), p + r * t) - type->getRadius() - otherRadius;
        else
            distanceCollisionPoint -= type->getRadius() + otherRadius;

#ifdef WITH_GUI
vehicle->speedInfluences += "  Vehicle #" + boost::lexical_cast<string>(vehicleIter->second->getId()) + ":"
    + "\n    dist=" + boost::lexical_cast<string>(distance)
    + "\n    distanceCollisionPoint=" + boost::lexical_cast<string>(distanceCollisionPoint)
    + "\n    t=" + boost::lexical_cast<string>(t) + " u=" + boost::lexical_cast<string>(u)
    + "\n    distanceToRoute=" + boost::lexical_cast<string>(distanceToRoute)
    + "\n    deltaDirections=" + boost::lexical_cast<string>(deltaDirections)
    + "\n    deltaAngle=" + boost::lexical_cast<string>(deltaAngle)
    + "\n";
#endif // WITH_GUI


        // Cheating block-crasher. Cheating since it drives through other vehicles
        if (vehicleIter->second->getCurrentSpeed() == 0 && vehicleIter->second->getState() & Vehicle::BLOCKED
            && ((deltaAngle > 60 && deltaAngle < 290) || vehicleIter->second->getWaitingFor().count(vehicle->getId()))) {
#ifdef WITH_GUI
            vehicle->speedInfluences += "      ignoreCheat\n";
#endif // WITH_GUI
            continue;
        }

        // Other vehicle is standing still outside of our course => ignore
        if (vehicleIter->second->getCurrentSpeed() == 0 && distanceToRoute > radius + otherRadius) {
#ifdef WITH_GUI
            vehicle->speedInfluences += "      standing still\n";
#endif // WITH_GUI
            continue;
        }

        // Other vehicle is standing still outside of our course => ignore
        else if (vehicleIter->second->getCurrentSpeed() == 0 && (deltaDirections > 60) && distanceToRoute > radius / 2 + otherRadius) {
#ifdef WITH_GUI
            vehicle->speedInfluences += "      orthogonal\n";
#endif // WITH_GUI
            continue;
        }

        // Is driving parallel so we don't have to care about it
        // A width of 2.5m is the maximal allowed width of a vehicle in Germany, so 3m should be enough
        else if (deltaDirections < 10 && distanceToRoute > 3) {
#ifdef WITH_GUI
            vehicle->speedInfluences += "      parallel\n";
#endif // WITH_GUI
            continue;
        }

        if (vehicleIter->second->getId() < vehicle->getId() &&
            distanceToRoute >= (type->getRadius() + otherRadius) / 3 &&
            (deltaAngle > 30 && deltaAngle < 360 - 30) &&
            vehicleIter->second->getState() & Vehicle::BLOCKED &&
            vehicleIter->second->getCurrentSpeed() < 10) {

#ifdef WITH_GUI
            vehicle->speedInfluences += "      ignore "  + boost::lexical_cast<string>(vehicleIter->second->getId()) + "\n";
#endif // WITH_GUI
            continue;
        }



        if (distance < type->getRadius() + otherRadius && distance >= futureDistance) {
            // Crash
            newSpeed = 0;
            reason = REASON_OTHERVEHICLES;
            nearestBlockingVehicle = vehicleIter->second;
            minStopDistance = min(minStopDistance, distance);

            if (vehicleIter->second->getWaitingFor().count(vehicle->getId()) == 0)
                vehicle->getWaitingFor().insert(vehicleIter->second->getId());
#ifdef WITH_GUI
                    vehicle->speedInfluences += "      collision with " + boost::lexical_cast<string>(vehicleIter->second->getId()) + "\n";
#endif // WITH_GUI

            roadSystem->addCollision(vehicle->getId(), vehicleIter->second->getId());
            vehicle->setState(vehicle->getState() | Vehicle::COLLIDING);
            vehicleIter->second->setState(vehicleIter->second ->getState() | Vehicle::COLLIDING);
            vehicle->getWaitingFor().insert(vehicleIter->second->getId());

        } else if (distance < (type->getRadius() + otherRadius) * 1.05 && distance >= futureDistance) {
            // Near crash, brake
            newSpeed = 0;
            reason = REASON_OTHERVEHICLES;
            nearestBlockingVehicle = vehicleIter->second;
            minStopDistance = min(minStopDistance, distance);
            if (vehicleIter->second->getWaitingFor().count(vehicle->getId()) == 0)
                vehicle->getWaitingFor().insert(vehicleIter->second->getId());
#ifdef WITH_GUI
                    vehicle->speedInfluences += "      near collision with "  + boost::lexical_cast<string>(vehicleIter->second->getId()) + "\n";
                    vehicle->speedInfluences += "      goAwayAngle: " + boost::lexical_cast<string>(M_PI - deltaAngle * M_PI / 360) + "\n";
#endif // WITH_GUI
                vehicle->getWaitingFor().insert(vehicleIter->second->getId());
        }


        if (vehicleIter->second->getCurrentSpeed() == 0) {
            // Skip this vehicle if far enough away
            if (distanceToRoute > type->getRadius() / 2 + otherRadius) {
#ifdef WITH_GUI
                vehicle->speedInfluences += "      notOnRouteStopped " + boost::lexical_cast<string>(vehicleIter->second->getId()) + "\n";
#endif // WITH_GUI
                continue;
            }
        }

        // Calculate a distance where the driver feels save
        double requiredDistance = vehicle->getCurrentSpeed() / 10 * type->getRadius() + type->getRadius() / 2;
        // Lawless drivers use a smaller distance (up to 20% smaller)
        requiredDistance *= 1 - (driver->getLawlessness() / 5);
        // Cautious drivers use a bigger distance (up to 50% bigger)
        requiredDistance *= 1 + (driver->getCautiousness() / 2);
        requiredDistance = max(requiredDistance, distStop * 1.5);


        if (u < 0 && deltaAngle > 45  && deltaAngle < 360 - 45 && distance > type->getRadius() * 2 + otherRadius * 2) {
#ifdef WITH_GUI
            vehicle->speedInfluences += "      ignoreUlessthan0 "  + boost::lexical_cast<string>(vehicleIter->second->getId()) + "\n";
#endif // WITH_GUI
            continue;
        }


        if (t < u && deltaAngle > 30 && deltaAngle < 360 - 30) {
#ifdef WITH_GUI
            vehicle->speedInfluences += "      ignoreNearer1 "  + boost::lexical_cast<string>(vehicleIter->second->getId()) + "\n";
#endif // WITH_GUI
            continue;
        }

        if ((distance < type->getRadius() || distanceCollisionPoint < type->getRadius()) && distance >= futureDistance) {

#ifdef WITH_GUI
            vehicle->speedInfluences += "      stop " + boost::lexical_cast<string>(vehicleIter->second->getId()) + "\n";
#endif // WITH_GUI
            // Stop
            newSpeed = 0;
            reason = REASON_OTHERVEHICLES;
            nearestBlockingVehicle = vehicleIter->second;
            minStopDistance = min(minStopDistance, distanceCollisionPoint + type->getRadius() + otherRadius);
            vehicle->getWaitingFor().insert(vehicleIter->second->getId());

        }

        if (t < u && deltaAngle > 30 && deltaAngle < 360 - 30) {
#ifdef WITH_GUI
            vehicle->speedInfluences += "      ignoreNearer2 "  + boost::lexical_cast<string>(vehicleIter->second->getId()) + "\n";
#endif // WITH_GUI
            continue;
        }

        if (distanceCollisionPoint < distStop) {
            // Brake
            if (10 < newSpeed) {
#ifdef WITH_GUI
                vehicle->speedInfluences += "      brake "  + boost::lexical_cast<string>(vehicleIter->second->getId()) + "\n";
#endif // WITH_GUI
                newSpeed = 10;
                reason = REASON_OTHERVEHICLES;
                nearestBlockingVehicle = vehicleIter->second;;
                minStopDistance = min(minStopDistance, distanceCollisionPoint + type->getRadius() + otherRadius);
            vehicle->getWaitingFor().insert(vehicleIter->second->getId());
            }
        } else if (distanceCollisionPoint < requiredDistance * 1.5 && vehicleIter->second->getCurrentSpeed() > 0) {
            // Reduce own speed to a bit slower than the speed of the nearest vehicle
            double possibleSpeed = max(10.0, 0.8 * max(vehicleIter->second->getCurrentSpeed(), vehicle->getCurrentSpeed())) * kmhToUnits;
            if (possibleSpeed < newSpeed) {
#ifdef WITH_GUI
                vehicle->speedInfluences += "      slow "  + boost::lexical_cast<string>(vehicleIter->second->getId()) + "\n";
#endif // WITH_GUI
                newSpeed = possibleSpeed;
                reason = REASON_OTHERVEHICLES;
                nearestBlockingVehicle = vehicleIter->second;
                minStopDistance = min(minStopDistance, distanceCollisionPoint + type->getRadius() + otherRadius);
            vehicle->getWaitingFor().insert(vehicleIter->second->getId());
            }
        } else if (distanceCollisionPoint < requiredDistance * 2 && vehicleIter->second->getCurrentSpeed() > 0) {
            // Hold the speed
            double possibleSpeed = max(10.0, max(vehicleIter->second->getCurrentSpeed(), 0.8 * vehicle->getCurrentSpeed())) * kmhToUnits;
            if (possibleSpeed < newSpeed) {
#ifdef WITH_GUI
                vehicle->speedInfluences += "      hold "  + boost::lexical_cast<string>(vehicleIter->second->getId()) + "\n";
#endif // WITH_GUI
                newSpeed = possibleSpeed;
                reason = REASON_OTHERVEHICLES;
                nearestBlockingVehicle = vehicleIter->second;
                minStopDistance = min(minStopDistance, distanceCollisionPoint + type->getRadius() + otherRadius);
            vehicle->getWaitingFor().insert(vehicleIter->second->getId());
            }
        } else if (distance < requiredDistance * 4) {
            // Speedup a bit
            double possibleSpeed = max(vehicleIter->second->getCurrentSpeed() * 1.5, 10.0) * kmhToUnits;
            if (possibleSpeed < newSpeed) {
#ifdef WITH_GUI
                vehicle->speedInfluences += "      approach "  + boost::lexical_cast<string>(vehicleIter->second->getId()) + "\n";
#endif // WITH_GUI
                newSpeed = possibleSpeed;
                reason = REASON_OTHERVEHICLES;
                nearestBlockingVehicle = vehicleIter->second;
                minStopDistance = min(minStopDistance, distanceCollisionPoint + type->getRadius() + otherRadius);
            }
        }
    } // end for(nearVehicles)

    Node *nextNode = roadSystem->getNode(vehicle->getRoute()->at(1));
    // If a node logic exists, it is probably a crossroad and not only a some position-node
    if (nextNode->getNodeLogic() != NULL) {
        double nextNodeDistance = calcDistance(nextNode->getPosition(), vehicle->getPosition());

        // If the crossing is not reserved yet, reserve it
        if (nextNodeDistance < minStopDistance && nextNodeDistance < distStop && nextNode->getReservation() == 0 && vehicle->getCurrentSpeed() > 0)
            nextNode->setReservation(vehicle->getId());


        // If the vehicle is near a crossroad, stop either in front or behind the crossing but not on it
        if (minStopDistance > nextNodeDistance - 3 * type->getRadius() && minStopDistance < nextNodeDistance + 4 * type->getRadius()) {
            // We would stop on the crossing. Stop a bit earlier
            if (nextNodeDistance - CROSSROAD_RADIUS - type->getRadius() < distStop) {

#ifdef WITH_GUI
                vehicle->speedInfluences += "    stop before crossroad\n";
#endif // WITH_GUI
                // Stop
                newSpeed = 0;
            } else if (nextNodeDistance - CROSSROAD_RADIUS * 2 - type->getRadius() < distStop) {
                // Brake
                if (10 < newSpeed) {
#ifdef WITH_GUI
                    vehicle->speedInfluences += "    brake before crossroad\n";
#endif // WITH_GUI
                    newSpeed = 10;
                }
            }
        }
    }

    if (newSpeed > vehicle->getCurrentSpeed() * kmhToUnits * 1.02)
        reason |= ACTION_ACCELERATING;
    else if (newSpeed < vehicle->getCurrentSpeed() * kmhToUnits * 0.96)
        reason |= ACTION_BRAKING;
    else if (newSpeed < vehicle->getCurrentSpeed() * kmhToUnits * 0.98)
        reason |= ACTION_DECELERATING;


    if (vehicle->getBlockedForTicks() > ticksTillIgnoreOthers + ticksWhileIgnoreOthers + (int)(vehicle->getId() % 50)) {
        vehicle->setBlockedForTicks(0);
    }
    else if (vehicle->getBlockedForTicks() > ticksTillIgnoreOthers + (int)(vehicle->getId() % 50)) {
        vehicle->setBlockedForTicks(vehicle->getBlockedForTicks() + 1);
    }
    else if (newSpeed <= 0 && reason & REASON_OTHERVEHICLES) {
             // && the vehicles will get even nearer to each other. Added to avoid increasing while waiting in a queue
        if (calcDistance(vehicle->getPosition(), nearestBlockingVehicle->getFuturePosition()) < calcDistance(vehicle->getPosition(), nearestBlockingVehicle->getPosition())) {
            vehicle->setBlockedForTicks(vehicle->getBlockedForTicks() + 1);
        } else {
            double ownAngle = calcAngle(toVec2f(vehicle->getFuturePosition() - vehicle->getPosition()));
            double otherAngle = calcAngle(toVec2f(nearestBlockingVehicle->getFuturePosition() - nearestBlockingVehicle->getPosition()));
            double delta = ownAngle - otherAngle;
            if (delta < -45 || delta > 45)
                vehicle->setBlockedForTicks(vehicle->getBlockedForTicks() + 1);
        }
    } else {
        vehicle->setBlockedForTicks(0);
    }

    // Transform to km/h
    newSpeed /= kmhToUnits;

    // Catch near-zero speeds
    if (newSpeed > -1 && newSpeed < 1)
        newSpeed = 0;

    return make_pair(newSpeed, reason);
}

double Microsimulator::findFirstBlockingNode(Vehicle *vehicle, const double minDistance, const double maxDistance) const {

    // If the route is empty, abort
    const deque<ID> *route = vehicle->getRoute();

    Vec2f lastPosition = toVec2f(vehicle->getPosition());
    pair<ID, int> lastStreet(vehicle->getStreetId(), vehicle->getLaneNumber());
    double distance = 0;

    // Iterate over the rest of the route
    for (size_t routeIndex = 1; routeIndex < route->size() - 1; ++routeIndex) {

        const Node *currentNode = roadSystem->getNode(route->at(routeIndex));
        // Calculate and add the distance
        distance += calcDistance(lastPosition, currentNode->getPosition());

        // Store the current position for the next iteration
        lastPosition = currentNode->getPosition();

        // If the distance becomes too big, abort
        if (distance > maxDistance)
            break;

        // Check if the node is reserved
        if (currentNode->getReservation() != 0 && currentNode->getReservation() != vehicle->getId())
            return distance - CROSSROAD_RADIUS;

        // Get the street that has to be taken to go to the next node
        pair<ID, int> nextStreet = currentNode->findNextStreet(route->at(routeIndex + 1));

        // If the distance is too small, skip the test in this iteration
        if (distance >= minDistance) {

            // Ask the node whether traversal is allowed
            const double canEnterResult = currentNode->canEnter(lastStreet.first, lastStreet.second, nextStreet.first, nextStreet.second);
            // If it is not allowed, return the distance where the vehicle has to stop at
            if (canEnterResult > 0)
                return distance - canEnterResult;
        }

        lastStreet = nextStreet;
    }

    // Nothing blocks so return an unlimited distance
    return numeric_limits<double>::max();
}

set<pair<double, Vehicle*> > Microsimulator::findNearVehicles(Vehicle *vehicle, const double maxDistance) const {

    set<pair<double, Vehicle*> > result;

    const Street *street = roadSystem->getStreet(vehicle->getStreetId());

    size_t nearestNodeIndex = street->getNearestNodeIndices(toVec2f(vehicle->getPosition())).first;

    const int direction = (vehicle->getLaneNumber() > 0) ? 1 : -1;

    // Get the orientation of this vehicle
    const double ownAngle = calcAngle(toVec2f(vehicle->getFuturePosition()) - toVec2f(vehicle->getPosition()));

    const double allowedAngleSameLane = 95;
    const double allowedAngleOtherLane = 95;

    // Add vehicles on current street
    for (int streetDirection = -1; streetDirection < 2; streetDirection += 2) {
        for (unsigned int lane = 1; lane <= street->getLaneCount(streetDirection); ++lane) {

            const set<ID> *vehicles = street->getLaneVehicles(streetDirection * lane);

            for (set<ID>::const_iterator vehicleIter = vehicles->begin(); vehicleIter != vehicles->end(); ++vehicleIter) {

                // Don't add self
                if (*vehicleIter == vehicle->getId())
                    continue;

                Vehicle *otherVehicle = roadSystem->getVehicle(*vehicleIter);

                // Only check vehicle in front of us
                int deltaAngle = (int)(calcAngle(toVec2f(otherVehicle->getPosition() - vehicle->getPosition())) - ownAngle + 360) % 360;


                    if (deltaAngle < 360 - allowedAngleSameLane && deltaAngle > allowedAngleSameLane)
                        continue;

                    double distance = calcDistance(otherVehicle->getPosition(), vehicle->getPosition());

                    if (distance <= maxDistance)
                        result.insert(make_pair(distance, otherVehicle));

            } // for(vehicle)
        } // for(lanes)
    } // for(directions)


    // Add vehicles on connected streets
    for (size_t index = nearestNodeIndex + 1; index > 0 && index <= street->getNodeIds()->size(); index += direction) {

        const Node *node = roadSystem->getNode(street->getNodeIds()->operator[](index - 1));

        double maxDistanceFromNode = maxDistance - calcDistance(vehicle->getPosition(), node->getPosition());

        // Ignore if too far
        if (maxDistanceFromNode <= 0)
            continue;

        // Get all streets going through this node
        const vector<ID>& streets = node->getStreetIds();

        for (size_t i = 0; i < streets.size(); ++i) {

            // Do not check current street
            if (streets[i] == vehicle->getStreetId())
                continue;

            const Street *otherStreet = roadSystem->getStreet(streets[i]);

            if (!otherStreet->getIsMicro())
                continue;

            // Get all vehicles on this street
            for (int streetDirection = -1; streetDirection < 2; streetDirection += 2) {
                for (unsigned int lane = 1; lane <= otherStreet->getLaneCount(streetDirection); ++lane) {

                    const set<ID> *vehicles = otherStreet->getLaneVehicles(streetDirection * lane);

                    for (set<ID>::const_iterator vehicleIter = vehicles->begin(); vehicleIter != vehicles->end(); ++vehicleIter) {

                        Vehicle *otherVehicle = roadSystem->getVehicle(*vehicleIter);

                        // Only check vehicle in front of us
                        int deltaAngle = (int)(calcAngle(toVec2f(otherVehicle->getPosition() - vehicle->getPosition())) - ownAngle + 360) % 360;

                        if (deltaAngle < 360 - allowedAngleOtherLane && deltaAngle > allowedAngleOtherLane)
                            continue;

                        double distance = calcDistance(otherVehicle->getPosition(), vehicle->getPosition());

                        if (distance <= maxDistance)
                            result.insert(make_pair(distance, otherVehicle));

                    } // for(vehicle)
                } // for(lanes)
            } // for(directions)
        } // for(streets)
    } // for(nodes)


    // Add street-less vehicles
    for (set<ID>::const_iterator streetlessIter = roadSystem->getFreeVehicles()->begin(); streetlessIter != roadSystem->getFreeVehicles()->end(); ++streetlessIter) {

        Vehicle *otherVehicle = roadSystem->getVehicle(*streetlessIter);

        // Only check vehicle in front of us
        int deltaAngle = (int)(calcAngle(toVec2f(otherVehicle->getPosition() - vehicle->getPosition())) - ownAngle + 360) % 360;

        if (deltaAngle < 360 - allowedAngleOtherLane && deltaAngle > allowedAngleOtherLane)
            continue;

        double distance = calcDistance(otherVehicle->getPosition(), vehicle->getPosition());

        if (distance <= maxDistance)
            result.insert(make_pair(distance, otherVehicle));
    }

    return result;
}


Microsimulator::Microsimulator(RoadSystem *roadSystem)
    : kmhToUnits(1), lastTickMilliseconds(1000), roadSystem(roadSystem) {

}

void Microsimulator::tick() {

    // Update km/h to units factor
    kmhToUnits = ((double)1000 / (60 * 60 * 1000)) * timer.getDelta().total_milliseconds();
    if (kmhToUnits <= 0) {
        // I don't really understand why, but it seems as the delta can be 0. Are computer really THAT fast??
        kmhToUnits = (double)1000 / (60 * 60 * 1000);
    }

    // Only do the collision-checks twice per second
    bool moveOnly = true;
    if (lastTickMilliseconds > 500) {
        moveOnly = false;
        lastTickMilliseconds -= 500;
    }

    lastTickMilliseconds += timer.getDelta().total_milliseconds();

    // Run over the vehicles and handle each of it
    for (map<ID, Vehicle*>::const_iterator vehicleIter = roadSystem->getVehicles()->begin();
         vehicleIter != roadSystem->getVehicles()->end();) {

        Vehicle *vehicle = vehicleIter->second;
        ++vehicleIter;

        // Only do something if the vehicle is simulator controlled
        if (vehicle->getController() == 0)
            handleVehicle(vehicle, moveOnly);
        else if (vehicle->getController() > 0) {
            vehicle->setController(vehicle->getController() - 1);
            handleVehicle(vehicle, true);
        }
    }
}
