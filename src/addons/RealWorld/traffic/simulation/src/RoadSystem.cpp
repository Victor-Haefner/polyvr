#include "RoadSystem.h"

#include "NodeLogicRightFirst.h"
#include "NodeLogicPriorityCrossing.h"
#include "NodeLogicTrafficLight.h"
#include "timer.h"

void RoadSystem::findSourceStreets() {

    sourceStreets.clear();

    // The sum of all source-street-probabilities
    double sumSourceStreetProbabilities = 0;

    for (map<ID, Street*>::const_iterator streetIter = streets.begin(); streetIter != streets.end(); ++streetIter) {
        // Check if it is a one-way street into the right direction
        if (streetIter->second->getLaneCount(1) == 0 || streetIter->second->getLaneCount(-1) > 0)
            continue;
        // If it is, check if its first node is unconnected.
        const Node *node = nodes[streetIter->second->getNodeIds()->front()];
        if (node->getStreetIds().size() == 1) {
            sumSourceStreetProbabilities += sqrt(streetIter->second->getRoutingCost());
            // sqrt(cost) to avoid that all traffic is on big streets
            sourceStreets.push_back(make_pair(streetIter->first, sqrt(streetIter->second->getRoutingCost())));
        }
    }

    // Have now run through all streets. If none has been found, drop the one-way requirement
    if (sourceStreets.empty()) {
        for (map<ID, Street*>::const_iterator streetIter = streets.begin(); streetIter != streets.end(); ++streetIter) {
            // If it is, check if its first node is unconnected.
            const Node *node = nodes[streetIter->second->getNodeIds()->front()];
            if (node->getStreetIds().size() == 1) {
                sumSourceStreetProbabilities += sqrt(streetIter->second->getRoutingCost());
                sourceStreets.push_back(make_pair(streetIter->first, streetIter->second->getRoutingCost()));
            }
            // Note that this is not quite correct. Since the fillSourceStreets() method always adds to the first lane,
            // some vehicles might be driving away from the map if added here. But it makes no difference in bigger terms
            node = nodes[streetIter->second->getNodeIds()->back()];
            if (node->getStreetIds().size() == 1) {
                sumSourceStreetProbabilities += sqrt(streetIter->second->getRoutingCost());
                sourceStreets.push_back(make_pair(streetIter->first, sqrt(streetIter->second->getRoutingCost())));
            }
        }
    }

    if (sourceStreets.empty()) {
        // Still empty??? Just add the first few streets
        for (map<ID, Street*>::const_iterator streetIter = streets.begin(); streetIter != streets.end(); ++streetIter) {
            if (sourceStreets.size() > 10)
                break;
            sumSourceStreetProbabilities += sqrt(streetIter->second->getRoutingCost());
            sourceStreets.push_back(make_pair(streetIter->first, sqrt(streetIter->second->getRoutingCost())));
        }
    }

    // Transform the probabilities. Up to now, they are the routing-costs of the streets.
    // Now make them percentages of the sum so each get its part of the vehicles to add
    for (vector< pair<ID, double> >::iterator sourceIter = sourceStreets.begin(); sourceIter != sourceStreets.end(); ++sourceIter) {
        sourceIter->second = (sourceIter->second / sumSourceStreetProbabilities) ;
    }
}

void RoadSystem::fillSourceStreets() {

    // Do not use the source-streets always but create vehicle on other streets, too
    if (rand() % 2 == 0) {
        // If there are no source streets, try to find some
        if (sourceStreets.empty())
            findSourceStreets();

        unsigned int vehiclesAdded = 0;

        // Add each street its share of the vehicles
        for (vector< pair<ID, double> >::const_iterator sourceIter = sourceStreets.begin(); sourceIter != sourceStreets.end(); ++sourceIter) {

            Street *street = streets[sourceIter->first];

            const unsigned int vehiclesToAdd = sourceIter->second * offmapVehicleCount;
            const unsigned int vehiclesPrev = street->getLaneVehicleCount(1);

            for (size_t i = 0; i < vehiclesToAdd; ++i) {
                Vehicle *newVehicle = street->createRandomVehicle(1, 0);

                if (street->getIsMicro()) {
                    if (newVehicle == NULL)
                        // No more space, abort
                        break;

                    // Check if inside a view area. If it is, remove it and abort
                    for (vector<ViewArea*>::iterator iter = viewAreas.begin(); iter != viewAreas.end(); ++iter) {
                        if (calcDistance(toVec2f(newVehicle->getPosition()), (*iter)->position) < (*iter)->radius + VEHICLE_LENGTH) {
                            street->removeVehicle(newVehicle->getId(), newVehicle->getLaneNumber());
                            removeVehicle(newVehicle->getId());
                            i = vehiclesToAdd; // Break vehicle-add loop
                            break;
                        }
                    }
                }

            }
            vehiclesAdded += street->getLaneVehicleCount(1) - vehiclesPrev;
        }

        offmapVehicleCount -= vehiclesAdded;

    } else {
        // Add vehicles to a random group of meso-streets


        if (streets.empty() || offmapVehicleCount <= 0)
            return;


        int vehiclesAdded = 0;

        // Number of vehicles that should be added per street
        double vehiclePerStreet = (double)offmapVehicleCount / streets.size();

        // Add each street its share of the vehicles
        for (map<ID, Street*>::const_iterator sourceIter = streets.begin(); vehiclesAdded < offmapVehicleCount && sourceIter != streets.end(); ++sourceIter) {

            if (sourceIter->second->getIsMicro())
                continue;

            // Throw a dice to find out if vehicles should be added here
            int r = rand() % streets.size();
            // Value in 0..1
            double r2 = (double)r / streets.size();

            int vehiclesToAdd = r2 * 2 * vehiclePerStreet;

            int vehiclesPrevious = sourceIter->second->getLaneVehicleCount(-1) + sourceIter->second->getLaneVehicleCount(1);

            for (int i = 0; i < vehiclesToAdd; ++i)
                sourceIter->second->createRandomVehicle((rand() % 1)?1:-1);

            vehiclesAdded += (sourceIter->second->getLaneVehicleCount(-1) + sourceIter->second->getLaneVehicleCount(1)) - vehiclesPrevious;
        }

        offmapVehicleCount -= vehiclesAdded;
    }
}

RoadSystem::ViewArea::ViewArea(const ID id, const Vec2f pos, const double radius)
    : id(id), radius(radius), position(pos) {
}

RoadSystem::RoadSystem()
    : laneWidth(VEHICLE_LENGTH), defaultSpeed(50), defaultType(Street::RESIDENTIAL), trafficDensity(3),
      nodes(), streets(), vehicleTypes(), vehicleTypesProbability(0), driverTypes(), driverTypesProbability(0),
      maxVehicleId(300), vehicles(), nodeLogics(), viewAreas(), viewAreasVehicles(), collisions(),
      freeVehicles(), sourceStreets(), offmapVehicleCount(0) {

      // Create a dummy driver and vehicle type if the user does not register any themselves
      driverTypes.insert(make_pair(404, DriverType::createDriverType(404, 0, 0, 1)));
      vehicleTypes.insert(make_pair(404, VehicleType::createVehicleType(404, 0, VEHICLE_LENGTH / 2, 130, 10, 20)));
}

RoadSystem::~RoadSystem() {

    for (vector<ViewArea*>::iterator iter = viewAreas.begin(); iter != viewAreas.end(); ++iter)
        delete *iter;
    viewAreas.clear();
    viewAreasVehicles.clear();

    for (map<ID, VehicleType*>::iterator iter = vehicleTypes.begin(); iter != vehicleTypes.end(); ++iter)
        delete iter->second;
    vehicleTypes.clear();

    for (map<ID, DriverType*>::iterator iter = driverTypes.begin(); iter != driverTypes.end(); ++iter)
        delete iter->second;
    driverTypes.clear();

    for (map<ID, Vehicle*>::iterator iter = vehicles.begin(); iter != vehicles.end(); ++iter)
        delete iter->second;
    vehicles.clear();

    for (set<NodeLogic*>::iterator iter = nodeLogics.begin(); iter != nodeLogics.end(); ++iter)
        delete *iter;
    nodeLogics.clear();

    for (map<ID, Street*>::iterator iter = streets.begin(); iter != streets.end(); ++iter)
        delete iter->second;
    streets.clear();

    for (map<ID, Node*>::iterator iter = nodes.begin(); iter != nodes.end(); ++iter)
        delete iter->second;
    nodes.clear();
}

void RoadSystem::tick() {

    // Tick node logics
    for (set<NodeLogic*>::iterator iter = nodeLogics.begin(); iter != nodeLogics.end(); ++iter) {
        (*iter)->tick();
    }

    // Update view areas

    // Update positions of viewareas bound to vehicles
    for (map<ViewArea*, ID>::iterator areaIter = viewAreasVehicles.begin(); areaIter != viewAreasVehicles.end(); ++areaIter) {
        Vec3f vehiclePos = vehicles[areaIter->second]->getPosition();
        areaIter->first->position = Vec2f(vehiclePos[0], vehiclePos[2]);
    }

    // Update micro-flags of streets
    // Possible problem: If a street is very long without any nodes in the middle, it is possible that a viewarea is in the middle and does not see it
    // A possible fix would be to replace the big loop-construct with a line-circles-intersection test

    // Create a copy of the map<ID, Street*> to avoid calls to Street.setMicro()
    map<ID, bool> micros;
    for (map<ID, Street*>::iterator streetIter = streets.begin(); streetIter != streets.end(); ++streetIter) {
        micros.insert(make_pair(streetIter->first, false));
    }
    // For each view area, check the distances to all nodes
    // If the distance is smaller than their radius, set the micro-flag for all adjacent streets to true
    for (vector<ViewArea*>::const_iterator areaIter = viewAreas.begin(); areaIter != viewAreas.end(); ++areaIter) {
        for (map<ID, Node*>::const_iterator nodeIter = nodes.begin(); nodeIter != nodes.end(); ++nodeIter) {
            if (calcDistance(nodeIter->second->getPosition(), (*areaIter)->position) < (*areaIter)->radius) {
                for (vector<ID>::const_iterator streetIdIter = nodeIter->second->getStreetIds().begin();
                     streetIdIter != nodeIter->second->getStreetIds().end();
                     ++streetIdIter) {
                    // Anyone still any idea what I am doing? I love C++! :D
                    micros[*streetIdIter] = true;
                }
            }
        }
    }

    // Apply new state to all streets
    for (map<ID, bool>::iterator iter = micros.begin(); iter != micros.end(); ++iter) {
        streets[iter->first]->setMicro(iter->second);
    }

    // Add lost vehicles back into the system
    fillSourceStreets();
}

void RoadSystem::setLaneWidth(const double width) {
    if (width < 0)
        laneWidth = VEHICLE_LENGTH;
    else
        laneWidth = width;
}

double RoadSystem::getLaneWidth() const {
    return laneWidth;
}

void RoadSystem::setDefaultSpeed(const double speed) {
    if (speed <= 0)
        defaultSpeed = 50;
    else
        defaultSpeed = speed;
}

double RoadSystem::getDefaultSpeed() const {
    return defaultSpeed;
}

void RoadSystem::setDefaultType(const Street::TYPE type) {
    if (type == Street::DEFAULT)
        defaultType = Street::RESIDENTIAL;
    else
        defaultType = type;
}

Street::TYPE RoadSystem::getDefaultType() const {
    return defaultType;
}

void RoadSystem::setTrafficDensity(const double density) {

    // This could probably also (and maybe prettier?) be implemented
    // by setting the offmapVehicleCount appropriately

    double oldDensity = trafficDensity;

    if (density < 0)
        trafficDensity = 0;
    else
        trafficDensity = density;

    // Apply new density if not equal to the old one
    if (abs(oldDensity - trafficDensity) < 0.001) {

        // Apply the density to all streets in meso mode.
        // This way the vehicles around the client will not disappear
        // immediately but over time it will become empty.
        // Of course this does not set an exact value but it will be good enough

        for (map<ID, Street*>::iterator iter = streets.begin(); iter != streets.end(); ++iter) {
            if (!iter->second->getIsMicro()) {
                iter->second->applyTrafficDensity(trafficDensity);
            }
        }
    }
}

double RoadSystem::getTrafficDensity() const {
    return trafficDensity;
}

bool RoadSystem::addNode(const ID id, const Vec2f& pos, const Node::FEATURE features) {

    if (nodes.count(id) > 0)
        return false;

    Node* node = new Node(this, id, pos, features);
    nodes.insert(make_pair(id, node));

    return true;
}

bool RoadSystem::removeNode(const ID id, const bool removeStreets) {
    map<ID, Node*>::iterator iter = nodes.find(id);
    if (iter == nodes.end())
        return false;

    if (removeStreets) {
        while (!iter->second->getStreetIds().empty()) {
            removeStreet(iter->second->getStreetIds().front());
        }
    }

    delete iter->second;
    return true;
}

bool RoadSystem::hasNode(const ID id) const {
    return nodes.count(id) != 0;
}

Node* RoadSystem::getNode(const ID id) const {
    map<ID, Node*>::const_iterator iter = nodes.find(id);
    if (iter == nodes.end()) {
        cout << "FATAL ERROR: Request for non-existing node with id " << id << ".\n";
        return NULL;
    } else
        return iter->second;
}

const map<ID, Node*>* RoadSystem::getNodes() const {
    return &nodes;
}

Street* RoadSystem::createStreet(const ID id, const vector<ID>& nodeIds) {

    // If found, return the old pointer
    map<ID, Street*>::iterator streetIter = streets.find(id);
    if (streetIter != streets.end())
        return streetIter->second;

    // Check if Node IDs are valid
    for (vector<ID>::const_iterator nodeIter = nodeIds.begin(); nodeIter != nodeIds.end(); ++nodeIter) {
        if (nodes.count(*nodeIter) == 0)
            return NULL;
    }

    return new Street(this, id, nodeIds);
}


bool RoadSystem::addStreet(Street *street) {

    // If found, abort
    if (streets.count(street->getId()) > 0)
        return false;

    // Check if Node IDs are valid
    for (vector<ID>::const_iterator iter = street->getNodeIds()->begin(); iter != street->getNodeIds()->end(); ++iter) {
        if (nodes.count(*iter) == 0)
            return false;
    }

    streets.insert(make_pair(street->getId(), street));

    // Add street to all its nodes
    for (vector<ID>::const_iterator iter = street->getNodeIds()->begin(); iter != street->getNodeIds()->end(); ++iter) {
        nodes[*iter]->addStreet(street);

        // Check if the node becomes a crossroad now
        if (nodes[*iter]->getNodeLogic() == NULL) {

            // If it has traffic lights, make a traffic light out of it
            if (nodes[*iter]->getFeatures() & Node::TRAFFICLIGHTS)
                nodeLogics.insert(NodeLogicTrafficLight::makeNodeLogic(this, *iter));

            // When more than 2 Streets now, it always is a crossroad
            else if (nodes[*iter]->getStreetIds().size() > 2) {
                // If they are of different sizes, create a priority crossing
                for (vector<ID>::const_iterator streetIter = nodes[*iter]->getStreetIds().begin();
                    streetIter != nodes[*iter]->getStreetIds().end(); ++streetIter) {

                    const Street *otherStreet = streets[*streetIter];
                    if (street->getType() != otherStreet->getType()) {
                        nodeLogics.insert(NodeLogicPriorityCrossing::makeNodeLogic(this, *iter));
                        break;
                    }
                }
                // All the same size, create a right-first crossing
                if (nodes[*iter]->getNodeLogic() == NULL)
                    nodeLogics.insert(NodeLogicRightFirst::makeNodeLogic(this, *iter));
            }

            // If there are exactly two streets, check whether they both end here:
            // If they do, it is no crossroad
            else if (nodes[*iter]->getStreetIds().size() == 2) {

                Street *a = streets[ nodes[*iter]->getStreetIds()[0] ];
                Street *b = streets[ nodes[*iter]->getStreetIds()[1] ];

                if (!(a->getNodeIds()->front() == *iter || a->getNodeIds()->back() == *iter)
                 || !(b->getNodeIds()->front() == *iter || b->getNodeIds()->back() == *iter)) {
                    // If they are of different sizes, create a priority crossing
                    for (vector<ID>::const_iterator streetIter = nodes[*iter]->getStreetIds().begin();
                        streetIter != nodes[*iter]->getStreetIds().end(); ++streetIter) {

                        const Street *otherStreet = streets[*streetIter];
                        if (street->getType() != otherStreet->getType()) {
                            nodeLogics.insert(NodeLogicPriorityCrossing::makeNodeLogic(this, *iter));
                            break;
                        }
                    }
                    // All the same size, create a right-first crossing
                    if (nodes[*iter]->getNodeLogic() == NULL)
                        nodeLogics.insert(NodeLogicRightFirst::makeNodeLogic(this, *iter));
                }
            }
        } else if (nodes[*iter]->getNodeLogic()->getType() == NodeLogic::RIGHT_BEFORE_LEFT) {
            // It might be necessary to upgrade to a priority-crossing now
            if (street->getType() != streets[nodes[*iter]->getStreetIds().front()]->getType()) {
                nodeLogics.erase(nodes[*iter]->getNodeLogic());
                nodeLogics.insert(NodeLogicPriorityCrossing::makeNodeLogic(this, *iter));
            }
        }
    }

    street->applyTrafficDensity(trafficDensity);

    // Invoke a recalculation of the source streets
    sourceStreets.clear();

    return true;
}

bool RoadSystem::removeStreet(const ID id, const bool removeVehicles) {

    map<ID, Street*>::iterator iter = streets.find(id);
    if (iter == streets.end())
        return NULL;

    if (removeVehicles) {
        iter->second->applyTrafficDensity(0);
    }

    for (vector<ID>::const_iterator nodeIter = iter->second->getNodeIds()->begin(); nodeIter != iter->second->getNodeIds()->end(); ++nodeIter) {
        nodes.find(*nodeIter)->second->removeStreet(iter->second);
    }

    delete iter->second;
    streets.erase(iter);

    // Invoke a recalculation of the source streets
    sourceStreets.clear();

    return true;
}

bool RoadSystem::hasStreet(const ID id) const {
    return streets.count(id) != 0;
}

Street* RoadSystem::getStreet(const ID id) const {
    map<ID, Street*>::const_iterator iter = streets.find(id);
    if (iter == streets.end()) {
        cout << "FATAL ERROR: Request for non-existing street with id " << id << ".\n";
        return NULL;
    } else
        return iter->second;
}

const map<ID, Street*>* RoadSystem::getStreets() const {
    return &streets;
}

bool RoadSystem::addVehicleType(const ID id, const double probability, const double radius, const double maxSpeed, const double maxAcc, const double maxRot) {

    map<ID, VehicleType*>::iterator iter = vehicleTypes.find(id);
    if (iter != vehicleTypes.end())
        return false;

    VehicleType *type = VehicleType::createVehicleType(id, probability, radius, maxSpeed,maxAcc, maxRot);
    if (type == NULL)
        return false;

    vehicleTypes.insert(make_pair(id, type));
    vehicleTypesProbability += probability;
    return true;
}

bool RoadSystem::removeVehicleType(const ID id, const bool modifyVehicles) {

    map<ID, VehicleType*>::iterator iter = vehicleTypes.find(id);
    if (iter == vehicleTypes.end())
        return false;

    // Check if the id is in use
    for (map<ID, Vehicle*>::iterator vehicleIter = vehicles.begin(); vehicleIter != vehicles.end(); ++vehicleIter) {
        if (vehicleIter->second->getVehicleType() == id) {
            if (!modifyVehicles) {
                return false;
            } else {
                // Set the type to the first or last one
                if (vehicleTypes.begin()->first != id)
                    vehicleIter->second->setVehicleType(vehicleTypes.begin()->first);
                else if (vehicleTypes.rbegin()->first != id)
                    vehicleIter->second->setVehicleType(vehicleTypes.rbegin()->first);
                else
                    // No other type is available: just fail.
                    return false;
            }
        }
    }

    // Remove the type
    vehicleTypesProbability -= iter->second->getProbability();
    delete iter->second;
    vehicleTypes.erase(iter);

    return true;
}

VehicleType* RoadSystem::getVehicleType(const ID id) const {

    map<ID, VehicleType*>::const_iterator iter = vehicleTypes.find(id);
    if (iter == vehicleTypes.end()) {
        cout << "FATAL ERROR: Request for non-existing vehicle type with id " << id << ".\n";
        return NULL;
    } else
        return iter->second;
}

ID RoadSystem::getRandomVehicleType() const {

    if (vehicleTypesProbability <= 0)
        return 404;

    int r = rand() % (int)vehicleTypesProbability;
    for (map<ID, VehicleType*>::const_iterator iter = vehicleTypes.begin(); iter != vehicleTypes.end(); ++iter) {
        r -= iter->second->getProbability();
        if (r < 0)
            return iter->first;
    }

    // Should not come here...
    return 404;

}

const map<ID, VehicleType*>* RoadSystem::getVehicleTypes() const {
    return &vehicleTypes;
}

bool RoadSystem::addDriverType(const ID id, const double probability, const double lawlessness, const double cautiousness) {

    map<ID, DriverType*>::iterator iter = driverTypes.find(id);
    if (iter != driverTypes.end())
        return false;

    DriverType *type = DriverType::createDriverType(id, probability, lawlessness, cautiousness);
    if (type == NULL)
        return false;

    driverTypes.insert(make_pair(id, type));
    driverTypesProbability += probability;
    return true;
}

bool RoadSystem::removeDriverType(const ID id) {

    map<ID, DriverType*>::iterator iter = driverTypes.find(id);
    if (iter == driverTypes.end())
        return false;

    // Check if the id is in use
    for (map<ID, Vehicle*>::iterator vehicleIter = vehicles.begin(); vehicleIter != vehicles.end(); ++vehicleIter) {
        if (vehicleIter->second->getDriverType() == id) {
            // Set the type to the first or last one
            if (driverTypes.begin()->first != id)
                vehicleIter->second->setDriverType(driverTypes.begin()->first);
            else if (driverTypes.rbegin()->first != id)
                vehicleIter->second->setDriverType(driverTypes.rbegin()->first);
            else
                // No other type is available: just fail.
                return false;
        }
    }

    // Remove the type
    driverTypesProbability -= iter->second->getProbability();
    delete iter->second;
    driverTypes.erase(iter);

    return true;
}

DriverType* RoadSystem::getDriverType(const ID id) const {

    map<ID, DriverType*>::const_iterator iter = driverTypes.find(id);
    if (iter == driverTypes.end()) {
        cout << "FATAL ERROR: Request for non-existing driver type with id " << id << ".\n";
        return NULL;
    } else
        return iter->second;
}

ID RoadSystem::getRandomDriverType() const {

    // If there are no drivers registered, return the default driver
    if (driverTypesProbability <= 0)
        return 404;

    int r = rand() % (int)driverTypesProbability;
    for (map<ID, DriverType*>::const_iterator iter = driverTypes.begin(); iter != driverTypes.end(); ++iter) {
        r -= iter->second->getProbability();
        if (r < 0)
            return iter->first;
    }

    // Should not come here...
    return 404;
}

const map<ID, DriverType*>* RoadSystem::getDriverTypes() const {
    return &driverTypes;
}

bool RoadSystem::addVehicle(const ID id, const Vec3f pos, const double radius) {

    if (vehicles.count(id) > 0)
        return false;

    // Create a vehicle type to store the radius
    VehicleType *vt = new VehicleType(id);
    vt->setRadius(radius);
    vehicleTypes.insert(make_pair(500 + id, vt));

    Vehicle *v = new Vehicle(id, pos, Quaternion(0, 1, 0, 0));
    v->setController(-1);
    v->setVehicleType(500 + id);
    vehicles.insert(make_pair(id, v));
    return true;
}


bool RoadSystem::addVehicle(Vehicle *vehicle) {

    if (vehicles.count(vehicle->getId()) > 0)
        return false;

    vehicles.insert(make_pair(vehicle->getId(), vehicle));
    return true;
}

bool RoadSystem::removeVehicle(const ID id) {

    map<ID, Vehicle*>::iterator iter = vehicles.find(id);
    if (iter == vehicles.end())
        return false;

    // Erase the associated type
    map<ID, VehicleType*>::iterator typeIter = vehicleTypes.find(500 + id);
    if (typeIter != vehicleTypes.end()) {
        delete typeIter->second;
        vehicleTypes.erase(typeIter);
    }

    delete iter->second;
    vehicles.erase(iter);
    return true;
}

Vehicle* RoadSystem::getVehicle(const ID id) const {

    map<ID, Vehicle*>::const_iterator iter = vehicles.find(id);
    if (iter == vehicles.end()) {
        cout << "FATAL ERROR: Request for non-existing vehicle with id " << id << ".\n";
        return NULL;
    } else
        return iter->second;
}

const map<ID, Vehicle*>* RoadSystem::getVehicles() const {
    return &vehicles;
}

ID RoadSystem::getUnusedVehicleId() {
    return maxVehicleId++;
}

bool RoadSystem::addViewarea(const ID id, const Vec2f pos, const double radius) {

    // Check if the id already is in use
    for (unsigned int i = 0; i < viewAreas.size(); ++i) {
        if (viewAreas[i]->id == id)
            return false;
    }

    // Create and add the new area
    if (radius < 0)
        return false;

    ViewArea *a = new ViewArea(id, pos, radius);
    viewAreas.push_back(a);
    return true;
}

bool RoadSystem::addViewarea(const ID id, const ID vehicle, const double radius) {

    // Check if the id already is in use
    for (unsigned int i = 0; i < viewAreas.size(); ++i) {
        if (viewAreas[i]->id == id)
            return false;
    }

    // Create and add the new area
    if (radius < 0)
        return false;

    Vehicle *v = getVehicle(vehicle);
    if (v == NULL)
        return false;

    ViewArea *a = new ViewArea(id, Vec2f(v->getPosition()[0], v->getPosition()[2]), radius);
    viewAreas.push_back(a);
    viewAreasVehicles.insert(make_pair(a, vehicle));
    return true;
}

bool RoadSystem::moveViewarea(const ID id, const Vec2f pos) {

    for (unsigned int i = 0; i < viewAreas.size(); ++i) {
        if (viewAreas[i]->id == id) {
            viewAreas[i]->position = pos;
            return true;
        }
    }
    return false;
}

const RoadSystem::ViewArea* RoadSystem::getViewarea(const ID id) const {

    for (vector<ViewArea*>::const_iterator iter = viewAreas.begin(); iter != viewAreas.end(); ++iter) {
        if ((*iter)->id == id)
            return *iter;
    }
    return NULL;
}

bool RoadSystem::removeViewarea(const ID id) {

    for (vector<ViewArea*>::iterator iter = viewAreas.begin(); iter != viewAreas.end(); ++iter) {
        if ((*iter)->id == id) {
            // Note: *iter resolvs to the stored pointer which is used as a key in the map<>
            viewAreasVehicles.erase(*iter);
            delete *iter;
            viewAreas.erase(iter);
            return true;
        }
    }
    return false;
}

void RoadSystem::addCollision(ID a, ID b) {
    collisions.insert(make_pair(a, b));
}

void RoadSystem::removeCollision(ID a, ID b) {
    collisions.erase(make_pair(a, b));
    collisions.erase(make_pair(b, a));
}

void RoadSystem::clearCollisions() {
    collisions.clear();
}

const set< pair<ID, ID> >* RoadSystem::getCollisions() const {
    return &collisions;
}

const set<NodeLogic*>* RoadSystem::getNodeLogics() const {
    return &nodeLogics;
}

void RoadSystem::addOffmapVehicle(const int amount) {
    offmapVehicleCount += amount;
}

int RoadSystem::getOffmapVehicleCount() const {
   return offmapVehicleCount;
}

set<ID>* RoadSystem::getFreeVehicles() {
    return &freeVehicles;
}
