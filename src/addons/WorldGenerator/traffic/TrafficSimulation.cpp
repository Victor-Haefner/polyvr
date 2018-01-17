#include "core/scene/VRSceneLoader.h"
#include "core/scene/VRScene.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/utils/toString.h"
#include "TrafficSimulation.h"
#include "RoadSystem.h"
#include "NodeLogic.h"
#include "NodeLogicTrafficLight.h"
#include "addons/WorldGenerator/GIS/OSMMap.h"

#include <boost/bind.hpp>
#include <boost/thread/locks.hpp>
#include <tuple>

using namespace OSG;

OldTrafficSimulation::OldTrafficSimulation() : VRObject("trafficSim"), collisionHandler(NULL), meshes(), vehicles(), lightBulbs() {
    // Add a dummy model for unknown vehicle types
    VRGeometryPtr geo = VRGeometry::create("vehicle_type_unknown");
    geo->setPersistency(0);
    geo->setPrimitive("Box", "1 1 2 1 1 1");
    meshes[404] = geo;

    a_red = VRMaterial::create("a_red");
    a_orange = VRMaterial::create("a_orange");
    a_green = VRMaterial::create("a_green");
    a_red->setDiffuse(Color3f(1,0,0));
    a_orange->setDiffuse(Color3f(1,0.8,0.1));
    a_green->setDiffuse(Color3f(0,1,0.1));
    a_red->setLit(false);
    a_orange->setLit(false);
    a_green->setLit(false);

	timer.setTimeScale(1);
}

OldTrafficSimulation::~OldTrafficSimulation() {
    player = 0;
    playerCreated = false;
}

OldTrafficSimulationPtr OldTrafficSimulation::create() { return OldTrafficSimulationPtr( new OldTrafficSimulation() ); }

void OldTrafficSimulation::setDrawingDistance(const double distance) {
    viewDistance = distance;
}

void OldTrafficSimulation::setTrafficDensity(const double density) {
    sim.getRoadSystem()->setTrafficDensity(density);
}

void OldTrafficSimulation::addVehicleType(const unsigned int id, const double probability, const double collisionRadius, const double maxSpeed, const double maxAcceleration, const double maxRoration, VRTransformPtr model) {


    //void addVehicleType(const unsigned int id, const double probability, const double collisionRadius, const double maxSpeed, const double maxAcceleration, const double maxRoration);
    //OSG::VRFunction<VRThread*>* func = new OSG::VRFunction<VRThread*>("trafficAddVehicleType", boost::bind(&OldTrafficSimulation::addVehicleType, self->obj, id, prob, radius, speed, acc, rot, static_pointer_cast<VRGeometry>(geo->obj));
    //OSG::VRSceneManager::get()->initThread(func, "trafficAddVehicleType", false);


    if (model == NULL) {
        cerr << "Given geometry is invalid!\n";
        return;
    }

    // Store the mesh
    meshes[id] = model;
}

void OldTrafficSimulation::addDriverType(const unsigned int id, const double probability, const double lawlessness, const double cautiousness) {
    ;
}

void OldTrafficSimulation::updateViewAreas() {
    RoadSystem* roadSystem = sim.getRoadSystem();
    const RoadSystem::ViewArea* viewArea = roadSystem->getViewarea(0);

    // Iterate over all vehicles and add all vehicles to the output
    // that are within viewArea->radius of viewArea->position
    cout << "system vehicles: " << roadSystem->getVehicles()->size() << endl;
    for (auto vehicle : *roadSystem->getVehicles()) {
        int ID = vehicle.second->getId();
        float d = calcDistance(viewArea->position, vehicle.second->getPosition());
        if (d > viewArea->radius) vehicles.erase(ID);
        else {
            Vehicle v;
            v.id = ID;
            v.vehicleTypeId = vehicle.second->getVehicleType();
            v.driverTypeId = vehicle.second->getDriverType();

            if (meshes.count(v.vehicleTypeId) == 0) v.vehicleTypeId = 404;
            v.model = static_pointer_cast<VRTransform>( meshes[v.vehicleTypeId]->duplicate(true) );
            v.model->setPersistency(0);
            vehicles.insert(make_pair(v.id, v));
        }

        /*Value v;
        v["id"] = vehicle.second->getId();
        v["vehicle"] = vehicle.second->getVehicleType();
        v["driver"] = vehicle.second->getDriverType();
        v["pos"][0] = vehicle.second->getPosition()[0];
        v["pos"][1] = vehicle.second->getPosition()[1];
        v["pos"][2] = vehicle.second->getPosition()[2];
        v["angle"][0] = vehicle.second->getFuturePosition()[0];
        v["angle"][1] = vehicle.second->getFuturePosition()[1];
        v["angle"][2] = vehicle.second->getFuturePosition()[2];
        v["dPos"][0] = vehicle.second->getFuturePosition()[0] - vehicle.second->getPosition()[0];
        v["dPos"][1] = vehicle.second->getFuturePosition()[1] - vehicle.second->getPosition()[1];
        v["dPos"][2] = vehicle.second->getFuturePosition()[2] - vehicle.second->getPosition()[2];
        v["dAngle"][0] = vehicle.second->getFuturePosition()[0] - vehicle.second->getPosition()[0];
        v["dAngle"][1] = vehicle.second->getFuturePosition()[1] - vehicle.second->getPosition()[1];
        v["dAngle"][2] = vehicle.second->getFuturePosition()[2] - vehicle.second->getPosition()[2];

        Vehicle::STATE state = vehicle.second->getState();
        if (state & Vehicle::TURN_LEFT) v["state"].append("leftIndicator");
        if (state & Vehicle::TURN_RIGHT) v["state"].append("leftIndicator");
        if (state & Vehicle::BRAKING) v["state"].append("braking");
        if (state & Vehicle::COLLIDING) v["state"].append("collision");
        if (state & Vehicle::CRASHED) v["state"].append("crashed");

        for (set< pair<ID, ID> >::iterator iter2 = roadSystem->getCollisions()->begin(); iter2 != roadSystem->getCollisions()->end(); ++iter2) {
            if (iter2->first == vehicle.second->getId()) v["colliding"].append(iter2->second);
            else if (iter2->second == vehicle.second->getId()) v["colliding"].append(iter2->first);
        }

        result["vehicles"].append(v); // Append vehicle to list*/
    }

    // Add the traffic lights
    for (auto logic : *roadSystem->getNodeLogics()) {
        if (logic->getType() != NodeLogic::TRAFFIC_LIGHT) continue;

        NodeLogicTrafficLight* trafficLight = static_cast<NodeLogicTrafficLight*>(logic);
        if (calcDistance(viewArea->position, trafficLight->getPosition()) > viewArea->radius) continue;

        // Add the traffic light
        for (auto light : trafficLight->getTrafficLights()) {
            const vector<NodeLogicTrafficLight::LightPost>& posts = trafficLight->getLightPostState(light);
            for (vector<NodeLogicTrafficLight::LightPost>::const_iterator post = posts.begin(); post != posts.end(); ++post) {
                /*Value postValue;
                postValue["at"] = post->node;
                postValue["facing"] = post->facing;
                postValue["street"] = post->street;

                bool error = false;
                for(vector<NodeLogicTrafficLight::STATE>::const_iterator state = post->laneStates.begin(); state != post->laneStates.end(); ++state) {
                    switch (*state) {
                        case NodeLogicTrafficLight::RED:
                            postValue["state"].append("red");
                            break;
                        case NodeLogicTrafficLight::GREEN:
                            postValue["state"].append("green");
                            break;
                        case NodeLogicTrafficLight::AMBER:
                            postValue["state"].append("amber");
                            break;
                        case NodeLogicTrafficLight::RED_AMBER:
                            postValue["state"].append("redamber");
                            break;
                        default:
                            error = true;
                            break;
                    }
                }

                if (!error) result["trafficlights"].append(postValue);*/
            }

        }

    }

    //roadSystem->clearCollisions();
}

void OldTrafficSimulation::start() {
    updateCb = VRUpdateCb::create("trafficSim", boost::bind(&OldTrafficSimulation::tick, this));
    VRScene::getCurrent()->addUpdateFkt(updateCb);
}

void OldTrafficSimulation::pause() {
    /*if (communicationThreadId > 0) {
        VRScene::getCurrent()->stopThread(communicationThreadId);
        communicationThreadId = -1;
    }*/
}

bool OldTrafficSimulation::isRunning() {
    //return (client.getSimulatorState() == client.RUNNING);
    return false;
}

void OldTrafficSimulation::tick() {
    updateViewAreas();

    vector<int> remV;
    vector<tuple<int, Vec3d, double>> addV;
    vector<tuple<int, Vec3d, Quaterniond>> movV;
    vector<tuple<int, int, int>> relV;
    vector<int> colV;
    vector<int> remA;
    vector<tuple<int, double, Vec2d>> addA;
    vector<tuple<int, Vec2d>> movA;

    if (player != NULL) { // Update the position of the player
        Vec3d pos = player->getFrom();
        movV.push_back(tuple<int, Vec3d, Quaterniond>(0, pos, Quaterniond()));
    }


    /*
        // Get traffic light updates
        if (!receivedData["trafficlights"].isNull() && receivedData["trafficlights"].isArray()) {

            // The light bulbs in the array will be moved around arbitrary whichever light posts are given
            // If there are not enough bulbs in the array, more are added
            // If there are too many bulbs, they are deleted
            size_t bulbIndex = 0;
            static const double postHeight = 2;
            static const double bulbSize   = 1; // Note: If you change this value from 2, change the value further down in VRGeometry::create(), too.

            for (auto lightpost : receivedData["trafficlights"]) {
                if (!lightpost.isObject()) continue;

                if (!lightpost["at"].isConvertibleTo(uintValue)
                 || !lightpost["to"].isConvertibleTo(uintValue)
                 ||  lightpost["state"].isNull()
                 || !lightpost["state"].isArray()) {
                    cout << "OldTrafficSimulation: Warning: Received invalid light post data.\n";
                    continue;
                }

                // Calculate the vector of the street

                // Get the node positions
                Vec2d atPos, toPos;
                string atId = toString(lightpost["at"].asUInt());
                string toId = toString(lightpost["to"].asUInt());
                bool foundAt = false, foundTo = false;
                for (auto mapIter : loadedMaps) {
                    for (auto node : mapIter->getNodes()) {

                        if (!foundAt && node.second->id == atId) {
                            atPos = mapCoordinator->realToWorld(Vec2d(node.second->lat, node.second->lon));
                            foundAt = true;
                        }

                        if (!foundTo && node.second->id == toId) {
                            toPos = mapCoordinator->realToWorld(Vec2d(node.second->lat, node.second->lon));
                            foundTo = true;
                        }

                        if (foundAt && foundTo)
                            break;
                    }
                    if (foundAt && foundTo)
                        break;
                }

                Vec2d streetOffset = toPos - atPos;
                const float prevLength = streetOffset.length();
                streetOffset.normalize();
                streetOffset *= min(prevLength / 2, Config::get()->STREET_WIDTH);

                Vec2d normal(-streetOffset[1], streetOffset[0]);
                normal.normalize();
                normal *= Config::get()->STREET_WIDTH;

                streetOffset += atPos;

                // streetOffset now contains a position in the center of a street a bit away from the crossing
                // normal is a vector that is orthogonal to the street

                // Now iterate over the lanes && hang up the lights
                double lane = -0.5;
                for (auto light : lightpost["state"]) {
                    lane += 1;

                    if (!light.isConvertibleTo(stringValue)) continue;

                    while (bulbIndex+1 >= lightBulbs.size()) {
                        auto scene = VRScene::getCurrent();
                        if (!scene) break;

                        // Create a new light
                        VRGeometryPtr geo = VRGeometry::create("ampel");
                        geo->setPersistency(0);
                        geo->setPrimitive("Sphere", "0.5 2"); // The first value has to be half of bulbSize
                        geo->setMaterial(a_red);

                        scene->add(geo);
                        lightBulbs.push_back(geo);
                    }

                    // color switch
                    VRGeometryPtr bulb = lightBulbs[bulbIndex++];
                    Vec3d p = Vec3d(streetOffset[0] + lane * normal[0], postHeight, streetOffset[1] + lane * normal[1]);
                    string lcol = light.asString();
                    if (lcol == "red") {
                        bulb->setWorldPosition(p+Vec3d(0,3 * bulbSize,0));
                        bulb->setMaterial(a_red);

                    } else if (lcol == "redamber") {
                        bulb->setWorldPosition(p+Vec3d(0,3 * bulbSize,0));
                        bulb->setMaterial(a_red);

                        bulb = lightBulbs[bulbIndex++];
                        bulb->setWorldPosition(p+Vec3d(0,2 * bulbSize,0));
                        bulb->setMaterial(a_orange);
                    } else if (lcol == "amber") {
                        bulb->setWorldPosition(p+Vec3d(0,2 * bulbSize,0));
                        bulb->setMaterial(a_orange);
                    } else if (lcol == "green") {
                        bulb->setWorldPosition(p+Vec3d(0,bulbSize,0));
                        bulb->setMaterial(a_green);
                    }
                }
            }

            // Remove unused lightbulbs
            while (bulbIndex < lightBulbs.size()) {
                lightBulbs.back()->destroy();
                lightBulbs.pop_back();
            }
        }
    }*/

    // Advance the vehicles a bit
    cout << "Update " << vehicles.size() << " vehicles\n";
    for (auto v : vehicles) {
        Vec3d p = v.second.pos;
        p[1] = 0;//TODO: get right street height
        v.second.model->setFrom(p);
        v.second.model->setDir(v.second.pos - v.second.orientation);
        v.second.pos += v.second.deltaPos;
        v.second.orientation += v.second.deltaOrientation;
    }

    sim.tick();

    RoadSystem* roadSystem = sim.getRoadSystem();

    for (auto v : remV) roadSystem->removeVehicle(v);
    for (auto v : addV) {
        int id = get<0>(v);
        Vec3d pos = get<1>(v);
        double radius = get<2>(v);

        roadSystem->addVehicle(id, pos, radius);
        roadSystem->getFreeVehicles()->insert(id);
    }

    for (auto v : movV) { // Move client controlled vehicles
        int id = get<0>(v);
        Vec3d pos = get<1>(v);
        Quaterniond angle = get<2>(v);

        auto vehicle = roadSystem->getVehicle(id);
        vehicle->setPosition(pos);
        vehicle->setOrientation(angle);
    }

    for (auto v : relV) {
        int id = get<0>(v);
        int driverType = get<1>(v);
        int vehicleType = get<2>(v);

        // Try to find it
        auto vehicle = roadSystem->getVehicle(id);
        vehicle->setController(0);
        roadSystem->getFreeVehicles()->erase(vehicle->getId());

        // Find the nearest node and set it as destination
        RSNode* minNode = NULL;
        double minDistance = numeric_limits<double>::max();
        for (auto nodeIter = roadSystem->getNodes()->begin(); nodeIter != roadSystem->getNodes()->end(); ++nodeIter) {
            double dist = calcDistance(vehicle->getPosition(), nodeIter->second->getPosition());
            if (dist < minDistance) {
                minDistance = dist;
                minNode = nodeIter->second;
            }
        }

        Street* street = roadSystem->getStreet(minNode->getStreetIds()[rand() % minNode->getStreetIds().size()]);

        // Find out on which side of the street we are
        // "Evil" hack: Get the lane-offset at the node and look which one is nearer to our position
        int lane = street->getLaneCount(1);
        Vec2d destPosition = street->getRelativeNodePosition(minNode->getId(), lane);
        if (calcDistance(destPosition, vehicle->getPosition()) > minDistance) { // Wrong side
            lane = street->getLaneCount(-1);
            destPosition = street->getRelativeNodePosition(minNode->getId(), lane);
        }

        vehicle->setCurrentDestination(destPosition);
        vehicle->setStreet(street->getId(), lane);
        int minNodeI = street->getNodeIndex(minNode->getId());
        int prevNode;
        if (lane < 0) {
            if (minNodeI > 0) prevNode = (*(street->getNodeIds()))[minNodeI - 1];
            else prevNode = (*(street->getNodeIds()))[minNodeI + 1];
        } else /* lane > 0 */ {
            if (minNodeI < street->getNodeIds()->size() - 1) prevNode = (*(street->getNodeIds()))[minNodeI + 1];
            else prevNode = (*(street->getNodeIds()))[minNodeI - 1];
        }

        vehicle->getRoute()->push_back(prevNode);
        vehicle->getRoute()->push_back(minNode->getId());
        vehicle->setDriverType(driverType);
        vehicle->setVehicleType(vehicleType);
    }

    for (auto id : colV) { // Handle crashed vehicles
        auto v = roadSystem->getVehicle(id);
        if (v != NULL) if (v->getController() >= 0) v->setController(180 + (rand() % 360));
    }

    for (auto id : remA) { // Remove viewarea
        roadSystem->removeViewarea(id);
    }

    for (auto a : addA) { // Add viewarea
        int id = get<0>(a);
        double size = get<1>(a);
        if (size < 0) size = 0;
        Vec2d pos = get<2>(a);
        roadSystem->addViewarea(id, pos, size);
    }

    for (auto a : movA) { // Move a viewarea
        int id = get<0>(a);
        Vec2d pos = get<1>(a);
        roadSystem->moveViewarea(id, pos);
    }

}


void OldTrafficSimulation::setCollisionHandler(bool (*handler) (Vehicle& a, Vehicle& b)) {
    collisionHandler = handler;
}

void OldTrafficSimulation::setVehiclePosition(const unsigned int id, const OSG::Vec3d& pos, const OSG::Vec3d& orientation) {
    ;
}

void OldTrafficSimulation::setSimulationSpeed(const double speed) {
    if (speed > 0) timer.setTimeScale(speed);
}

void OldTrafficSimulation::setPlayerTransform(VRTransformPtr transform) {
    player = transform;

    /*if (player != NULL && !playerCreated) {

        // Create a vehicle && a viewarea around it
        Value value;

        // Create the vehicle
        Value vehicle;
        vehicle["id"] = 0;
        Value pos;
        Vec3d worldPosition = player->getWorldPosition();
        pos[0u] = worldPosition[0];
        pos[1]  = worldPosition[1];
        pos[2]  = worldPosition[2];
        vehicle["pos"] = pos;
        vehicle["radius"] = driverVehicleRadius;

        // Pack as the requested array entry
        value["addVehicles"].append(vehicle);


        // Create the viewarea
        Value area;
        area["id"]        = 0;
        area["vehicleId"] = 0;
        area["size"]      = viewDistance;
        value["addViewareas"].append(area);

        value = client.sendData(value);
        errorMessage("adding the player: ", value);
        playerCreated = true;

    } else if (player != NULL && playerCreated) {
        // Update the position
        setVehiclePosition(0, player->getWorldPosition(), OSG::Vec3d(0,0,0));
    } else { // player == NULL

        // Remove area && vehicle
        Value value;
        value["removeViewareas"].append(0);
        value["removeVehicles"].append(0);

        value = client.sendData(value);
        errorMessage("removing the player", value);
        playerCreated = false;
    }*/
}

#include "addons/WorldGenerator/roads/VRRoadNetwork.h"

void OldTrafficSimulation::constructRoadSystem(VRRoadNetworkPtr net) {
    auto rs = sim.getRoadSystem();
    auto g = net->getGraph();
    auto nodes = g->getNodes();
    auto edges = g->getEdges();

    for (int i=0; i<nodes.size(); i++) {
        auto& node = nodes[i];
        Vec3d p = node.p.pos();
        rs->addNode(i, Vec2d(p[0], p[2]));
    }

    for (auto edgeVec : edges) {
        for (auto edge : edgeVec) {
            rs->createStreet(edge.ID, {edge.from, edge.to});
        }
    }
}










