#include "Mesosimulator.h"
#include "timer.h"

void Mesosimulator::handleStreet(Street *street) {

    ptime now = timer.getTime();

    // Check both directions
    for (int direction = -1; direction < 2; direction += 2) {

        // Get arrival times
        multiset< pair<ptime, ID> >* arrivalTimes = street->getLaneArrivalTimes(direction);

        // Handle only a restricted number of vehicles per tick,
        // simulates a bottleneck or so
        unsigned int handledVehicles = 0;

        // The maximum amount of vehicles handled per tick
        const unsigned int maxHandledVehicles = street->getLaneCount(direction) * 3;

        // Run through all "vehicles" that are driving into this direction
        for (multiset< pair<ptime, ID> >::iterator iter = arrivalTimes->begin(); iter != arrivalTimes->end(); ) {

            // Check if they have arrived
            if (iter->first <= now) {

                // If they have, move them to (an other?) street
                if (moveVehicle(iter->second, street, direction)) {
                    // Remove the entry from the list
                    arrivalTimes->erase(iter++);
                } else {
                    ++iter;
                }

                handledVehicles++;
                if (handledVehicles > maxHandledVehicles)
                    break;
            } else {
                // Since the set is sorted after the time, there can be no further waiting vehicles
                break;
            }
        }
    }
}

bool Mesosimulator::moveVehicle(const ID node, Street *street, const int direction) {

    // Get list of outgoing streets
    vector< pair<ID, int> > possibleStreets = roadSystem->getNode(node)->calculatePossibleStreets(street, direction);

    // Transfer <ID, direction> vector to <pointer, direction> vector
    vector< pair<Street*, int> > nextStreetPointers;
    for (size_t i = 0; i < possibleStreets.size(); ++i) {
        Street *street = roadSystem->getStreet(possibleStreets[i].first);
        // Don't add streets that are full
        if (street->getLaneVehicleCount(possibleStreets[i].second) < street->getLaneMaxVehicleCount(possibleStreets[i].second))
            nextStreetPointers.push_back(make_pair(street, possibleStreets[i].second));
    }

    Street *nextStreet = NULL;
    int nextDirection = 1;

    if (nextStreetPointers.empty()) {

        // Whaaa!! Nowhere to run!!! O_O
        // Try to send the vehicle back on the same street
        if (street->getLaneCount(-1 * direction) > 0
            && street->getLaneVehicleCount(-1 * direction) < street->getLaneMaxVehicleCount(-1 * direction)) {
            nextStreet = street;
            nextDirection = -1 * direction;
        } else {
            // Else remove the vehicle and add another one somewhere
            roadSystem->addOffmapVehicle();
            return true;
        }
    } else {

        // Select one of the streets with probabilities depending on their size

        // Calculate the maximum probability
        int maxProb = 1;
        for (size_t i = 0; i < nextStreetPointers.size(); ++i)
            maxProb += nextStreetPointers[i].first->getRoutingCost() * 1000;

        int randProb = rand() % (maxProb - 1);

        for (size_t i = 0; i < nextStreetPointers.size(); ++i) {
            int prob = nextStreetPointers[i].first->getRoutingCost() * 1000;
            if (prob > randProb) {
                nextStreet = nextStreetPointers[i].first;
                nextDirection = nextStreetPointers[i].second;
                break;
            }
            randProb -= prob;
        }

        if (nextStreet == NULL) {
            cerr << "Error which can not happen in meso-simulator, line " << __LINE__ << "\n";
            nextStreet = nextStreetPointers.back().first;
            if (nextStreet->getLaneCount(1) > 0)
                nextDirection = 1;
            else
                nextDirection = -1;
        }
    }

    // Check if the next street has some space left
    if (nextStreet->getLaneVehicleCount(nextDirection) >= nextStreet->getLaneMaxVehicleCount(nextDirection))
        return false;

    // Select a random lane
    int nextLane = nextDirection * ((rand() % nextStreet->getLaneCount(nextDirection)) + 1);

    // Check whether the node can be traversed
    if (roadSystem->getNode(node)->canEnter(street->getId(), direction, nextStreet->getId(), nextLane) > 0) {
        return false;
    }

    // Add vehicle to new street
    return street->transferVehicle(nextStreet, node, nextLane);
}

Mesosimulator::Mesosimulator(RoadSystem *roadSystem)
    : roadSystem(roadSystem) {

}

void Mesosimulator::tick() {

    // Iterate through the streets and call handleStreet() for each meso street
    const map<ID, Street*>* streets = roadSystem->getStreets();
    for (map<ID, Street*>::const_iterator streetIter = streets->begin(); streetIter != streets->end(); ++streetIter) {

        if (!streetIter->second->getIsMicro())
            handleStreet(streetIter->second);
    }
}
