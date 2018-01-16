#include "core/scene/VRSceneLoader.h"
#include "core/scene/VRScene.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/utils/toString.h"
#include "TrafficSimulation.h"
#include "RoadSystem.h"
#include "addons/WorldGenerator/GIS/OSMMap.h"

#include <boost/bind.hpp>
#include <boost/thread/locks.hpp>

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
    ;
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
    // Update the position of the player to be send to the server
    /*if (player != NULL) {
        // Move the vehicle
        Value vehicle;
        vehicle["id"] = 0;
        Value pos;

        Vec3d worldPosition = player->getFrom();
        pos[0u] = worldPosition[0];
        pos[1]  = worldPosition[1];
        pos[2]  = worldPosition[2];
        vehicle["pos"] = pos;

        // Pack as the requested array entry
        dataToSend["moveVehicles"][0] = vehicle;
    }

    if (!receivedData.isNull()) {
        //cout << "Received data\n";

        // New data received per network. Update the positions of vehicles && the states of traffic lights

        // A set which contains the IDs of the currently existing vehicles.
        // If data for one of those is received, the entry in the set will be removed.
        // All vehicles that are in the set after the loop finished are no longer in the
        // area && can be deleted.
        set<uint> vehicleIDs;
        for (auto iter : vehicles) vehicleIDs.insert(iter.first);

        if (!receivedData["vehicles"].isNull() && receivedData["vehicles"].isArray()) {
            // Update the vehicles

            //cout << "Received vehicles " << receivedData["vehicles"].size();

            // A set for possible collisions
            // Add them all to the set && resolve them later on to avoid doubled checks
            set< pair<unsigned int, unsigned int> > collisions;

            // Sleeps for 10ms each tick, but the send deltas are for one second
            // Use only a part of them to avoid moving too fast
            static const float partDelta = 10 / 1000;

            for (auto vehicleIter : receivedData["vehicles"]) {
                // Check if the values have valid types
                if (!vehicleIter["id"].isConvertibleTo(uintValue)
                 || !vehicleIter["pos"].isConvertibleTo(arrayValue)
                 || !vehicleIter["pos"][0].isConvertibleTo(realValue)
                 || !vehicleIter["pos"][1].isConvertibleTo(realValue)
                 || !vehicleIter["pos"][2].isConvertibleTo(realValue)
                 || !vehicleIter["dPos"].isConvertibleTo(arrayValue)
                 || !vehicleIter["dPos"][0].isConvertibleTo(realValue)
                 || !vehicleIter["dPos"][1].isConvertibleTo(realValue)
                 || !vehicleIter["dPos"][2].isConvertibleTo(realValue)
                 || !vehicleIter["angle"].isConvertibleTo(arrayValue)
                 || !vehicleIter["angle"][0].isConvertibleTo(realValue)
                 || !vehicleIter["angle"][1].isConvertibleTo(realValue)
                 || !vehicleIter["angle"][2].isConvertibleTo(realValue)
                 || !vehicleIter["dAngle"].isConvertibleTo(arrayValue)
                 || !vehicleIter["dAngle"][0].isConvertibleTo(realValue)
                 || !vehicleIter["dAngle"][1].isConvertibleTo(realValue)
                 || !vehicleIter["dAngle"][2].isConvertibleTo(realValue)) {
                    cout << "OldTrafficSimulation: Warning: Received invalid vehicle data.\n";
                    continue;
                }

                uint ID = vehicleIter["id"].asUInt();

                if (vehicles.count(ID) == 0) { // The vehicle is new, create it
                    if (!vehicleIter["vehicle"].isConvertibleTo(uintValue)
                     || !vehicleIter["driver"].isConvertibleTo(uintValue)) {
                        cout << "OldTrafficSimulation: Warning: Received invalid vehicle data.\n";
                        continue;
                    }

                    uint vID = vehicleIter["vehicle"].asUInt();
                    if (meshes.count(vID) == 0) {
                        // If it is bigger than 500 it is our user-controlled vehicle
                        if (vID < 500) cout << "OldTrafficSimulation: Warning: Received unknown vehicle type " << vID << ".\n";
                        continue;
                    }

                    Vehicle v;

                    v.id = ID;
                    v.vehicleTypeId = vID;
                    v.driverTypeId = vehicleIter["driver"].asUInt();

                    if (meshes.count(v.vehicleTypeId) == 0) v.vehicleTypeId = 404;
                    v.model = static_pointer_cast<VRTransform>( meshes[v.vehicleTypeId]->duplicate(true) );
                    v.model->setPersistency(0);

                    // Add it to the map
                    vehicles.insert(make_pair(v.id, v));

                } else vehicleIDs.erase(ID); // Already (and still) exists, remove its ID from the vehicle-id-set

                // Now the vehicle exists, update its position && state

                Vehicle& v = vehicles[ID];

                //v.pos = toVec3d(vehicleIter["pos"]);
                v.pos = Vec3d(vehicleIter["pos"][0].asFloat(), vehicleIter["pos"][1].asFloat(), vehicleIter["pos"][2].asFloat());
                v.deltaPos = Vec3d(vehicleIter["dPos"][0].asFloat(), vehicleIter["dPos"][1].asFloat(), vehicleIter["dPos"][2].asFloat());
                v.deltaPos *= partDelta;
                v.orientation = Vec3d(vehicleIter["angle"][0].asFloat(), vehicleIter["angle"][1].asFloat(), vehicleIter["angle"][2].asFloat());
                v.deltaOrientation = Vec3d(vehicleIter["dAngle"][0].asFloat(), vehicleIter["dAngle"][1].asFloat(), vehicleIter["dAngle"][2].asFloat());
                v.deltaOrientation *= partDelta;

                if (!vehicleIter["state"].isNull() && vehicleIter["state"].isArray()) {
                    v.state = Vehicle::NONE;

                    for (auto state : vehicleIter["state"]) {
                        if (!state.isConvertibleTo(stringValue)) continue;

                        if(state.asString() == "rightIndicator") {
                            v.state |= Vehicle::RIGHT_INDICATOR;
                        } else if(state.asString() == "leftIndicator") {
                            v.state |= Vehicle::LEFT_INDICATOR;
                        } else if(state.asString() == "accelerating") {
                            v.state |= Vehicle::ACCELERATING;
                        } else if(state.asString() == "braking") {
                            v.state |= Vehicle::BRAKING;
                        } else if(state.asString() == "waiting") {
                            v.state |= Vehicle::WAITING;
                        } else if(state.asString() == "blocked") {
                            v.state |= Vehicle::BLOCKED;
                        } else if(state.asString() == "collision") {
                            v.state |= Vehicle::COLLIDED;
                        }
                    }
                }

                if (!vehicleIter["colliding"].isNull() && vehicleIter["colliding"].isArray()) {
                    for (auto collisionIter : vehicleIter["colliding"]) {
                        if (!collisionIter.isConvertibleTo(uintValue)) continue;

                        uint other = collisionIter.asUInt();
                        if (other < v.id) collisions.insert(make_pair(other, v.id));
                        else collisions.insert(make_pair(v.id, other));
                    }
                }
            } // End vehicle iteration

            // Okay, all vehicles are updated now

            // Resolve collisions
            if (collisionHandler != NULL) {
                for (auto c : collisions) {
                    if (collisionHandler(vehicles[c.first], vehicles[c.second])) {
                        dataToSend["collision"].append(c.first);
                        dataToSend["collision"].append(c.second);
                    }
                }
            }

        } // End vehicle updates

        // Remove vehicles which are no longer on the map
        for (auto v : vehicleIDs) {
            vehicles[v].model->destroy();
            vehicles.erase(v);
        }


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
    //cout << "Update " << vehicles.size() << " vehicles\n";
    for (auto v : vehicles) {
        Vec3d p = v.second.pos;
        p[1] = 0;//TODO: get right street height
        v.second.model->setFrom(p);
        v.second.model->setDir(v.second.pos - v.second.orientation);
        v.second.pos += v.second.deltaPos;
        v.second.orientation += v.second.deltaOrientation;
    }

    sim.tick();
}

void OldTrafficSimulation::setCollisionHandler(bool (*handler) (Vehicle& a, Vehicle& b)) {
    collisionHandler = handler;
}

void OldTrafficSimulation::setVehiclePosition(const unsigned int id, const OSG::Vec3d& pos, const OSG::Vec3d& orientation) {
    ;
}

void OldTrafficSimulation::setSimulationSpeed(const double speed) {
    ;
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










