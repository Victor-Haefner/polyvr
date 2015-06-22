#include <boost/lexical_cast.hpp>

#include "core/scene/VRSceneLoader.h"
#include "core/scene/VRSceneManager.h"
#include "core/scene/VRScene.h"
#include "core/objects/material/VRMaterial.h"
#include "core/utils/toString.h"
#include "../Config.h"
#include "TrafficSimulation.h"


using namespace realworld;

/**
 * Splits a string at the given character.
 * @param str The string to split.
 * @param at The character to split at.
 * @return A vector containing the parts that are separated by \c at.
 */

vector<string> split(const string& str, char at = '|') {

    vector<string> ret;
    istringstream buf(str);

    string token;
    while (getline(buf, token, at))
        ret.push_back(token);
    return ret;
}

Value TrafficSimulation::convertNode(OSMNode *node) {
        Value value;

        value["id"] = boost::lexical_cast<Json::Value::UInt64>(node->id.c_str());
        Value pos;
        Vec2f vecPos = mapCoordinator->realToWorld(Vec2f(node->lat, node->lon));
        pos[0u] = vecPos[0];
        pos[1] = vecPos[1];
        value["pos"] = pos;

        if (node->tags.count("highway") > 0 && node->tags["highway"] == "traffic_signals") {
            value["features"].append("traffic_signals");
        }

        return value;
}

Value TrafficSimulation::convertStreet(OSMWay *street) {

    Value value;
    value["id"] = boost::lexical_cast<Json::Value::UInt64>(street->id.c_str());

    // Add node ids
    Value nodes;
    unsigned int iNodes = 0;
    for (vector<string>::const_iterator iter2 = street->nodeRefs.begin(); iter2 != street->nodeRefs.end(); iter2++) {
        nodes[iNodes++] = boost::lexical_cast<unsigned int>(iter2->c_str());
    }
    value["nodes"] = nodes;

    // Get number of lanes
    unsigned int nLanes = 2;
    unsigned int nLanesFor = 1;
    unsigned int nLanesBack = 1;
    if (street->tags.count("lanes") > 0) {
        nLanes = boost::lexical_cast<unsigned int>(street->tags["lanes"].c_str());

        if (street->tags.count("lanes:forward") > 0) {

            nLanesFor = boost::lexical_cast<unsigned int>(street->tags["lanes:forward"].c_str());
            nLanesBack = nLanes - nLanesFor;
        } else if (street->tags.count("lanes:backward") > 0) {

            nLanesBack = boost::lexical_cast<unsigned int>(street->tags["lanes:backward"].c_str());
            nLanesFor = nLanes - nLanesBack;
        } else if (street->tags.count("oneway") > 0 && street->tags["oneway"] == "yes") {

            nLanesFor = nLanes;
            nLanesBack = 0;
        } else {

            nLanesFor = nLanes / 2 + (nLanes % 2);
            nLanesBack = nLanes / 2;
        }
    } else if (street->tags.count("oneway") > 0 && street->tags["oneway"] == "yes") {

        // If no lane-counts are given but it is a oneway, create only one lane into the forward direction
        nLanesFor = 1;
        nLanesBack = 0;
        nLanes = 1;
    }
    // If the counts differ from the default, send them
    if (nLanes != 2 || nLanesFor != 1 || nLanesBack != 1) {
        Value lanes;
        lanes[0] = nLanesFor;
        lanes[1] = nLanesBack;
        value["lanes"] = lanes;
    }


    // Find out which lanes are accessible with a vehicle
    vector<string> accessForwardStrings = split(street->tags["access:lanes:forward"]);
    if (accessForwardStrings.empty())
        accessForwardStrings = split(street->tags["access:lanes"]);
    vector<string> accessBackwardStrings = split(street->tags["access:lanes:backward"]);
    vector<bool> accessForward, accessBackward;
    for (unsigned int i = 0; i < accessForwardStrings.size(); ++i) {
        // Modify here to allow other access-levels for vehicles, too
        if (accessForwardStrings[i].compare("yes") == 0)
            accessForward.push_back(true);
        else
            accessForward.push_back(false);
    }
    // Fill up if the access-array is too small. Should not happen but who knows...
    for (unsigned int i = accessForward.size(); i < nLanesFor; ++i)
        accessForward.push_back(true);

    for (unsigned int i = 0; i < accessBackwardStrings.size(); ++i) {
        // Modify here to allow other access-levels for vehicles, too
        if (accessBackwardStrings[i].compare("yes") == 0)
            accessBackward.push_back(true);
        else
            accessBackward.push_back(false);
    }
    for (unsigned int i = accessBackward.size(); i < nLanesBack; ++i)
        accessBackward.push_back(true);


    // Get lane changing permissions
    if (street->tags.count("change:lanes:forward") > 0 && street->tags.count("change:lanes:backward") > 0) {

        vector<string> forward = split(street->tags["change:lanes:forward"]);
        vector<string> backward = split(street->tags["change:lanes:backward"]);

        // Assuming lanes are [ 2, 1 ]
        // Input is:  backward=not_left|not_right|no*, forward=no
        //  Order is In-to-out:    a        b      c            d
        // Output is: [ left right none ]
        //  Order is      b    a     d
        //  Left-to-right when looking into the direction of the way,
        //  the permissions for each line are in their respective driving direction.
        // no* is ignored since there only are 2 lanes in this direction,
        // the "no" entry is for a bicycle lane.


        if (backward.size() >= 0 || forward.size() >= 0) {

            Value changing;

            // Backwards
            int inputI = backward.size() - 1;
            unsigned int outputI = 0;

            for (; inputI >= 0 && outputI < nLanesBack;) {
                if (accessBackward[inputI]) {
                    if (backward[inputI].compare("yes") == 0)
                        changing[outputI] = "both";
                    else if (backward[inputI].compare("no") == 0)
                        changing[outputI] = "none";
                    else if (backward[inputI].compare("not_left") == 0 || backward[inputI].compare("only_right") == 0)
                        changing[outputI] = "right";
                    else if (backward[inputI].compare("not_right") == 0 || backward[inputI].compare("only_left") == 0)
                        changing[outputI] = "left";
                    else
                        changing[outputI] = "none";
                    ++outputI;
                }
                --inputI;
            }
            while (outputI < nLanesBack)
                changing[outputI++] = "both";

            // Forward
            inputI = 0;
            outputI = nLanesBack;

            for (; inputI < (int)forward.size() && outputI < nLanes;) {
                if (accessForward[inputI]) {
                    if (forward[inputI].compare("yes") == 0)
                        changing[outputI] = "both";
                    else if (forward[inputI].compare("no") == 0)
                        changing[outputI] = "none";
                    else if (forward[inputI].compare("not_left") == 0 || forward[inputI].compare("only_right") == 0)
                        changing[outputI] = "right";
                    else if (forward[inputI].compare("not_right") == 0 || forward[inputI].compare("only_left") == 0)
                        changing[outputI] = "left";
                    else
                        changing[outputI] = "none";
                    ++outputI;
                }
                ++inputI;
            }
            while (outputI < nLanes)
                changing[outputI++] = "both";

            value["change"] = changing;
        }
    }

    // Get turning permissions
    // Nearly the same as for lane changes
    if ((street->tags.count("turn:lanes") > 0 || street->tags.count("turn:lanes:forward") > 0) && street->tags.count("turn:lanes:backward") > 0) {

        vector<string> forward;
        if (street->tags.count("turn:lanes") > 0)
            forward = split(street->tags["turn:lanes"]);
        else if (street->tags.count("turn:lanes:forward") > 0)
            forward = split(street->tags["turn:lanes:forward"]);
        vector<string> backward = split(street->tags["turn:lanes:backward"]);

        if (backward.size() >= 0 || forward.size() >= 0) {

            Value turns;

            // Backwards
            int inputI = backward.size() - 1;
            unsigned int outputI = 0;

            for (; inputI >= 0 && outputI < nLanesBack;) {
                char directions = 0;

                if (accessBackward[inputI]) {
                    if (backward[inputI].find("right") != string::npos)
                        directions |= 1;
                    if (backward[inputI].find("through") != string::npos)
                        directions |= 2;
                    if (backward[inputI].find("left") != string::npos)
                        directions |= 4;

                    if (directions == 1)
                        turns[outputI] = "right";
                    else if (directions == 2)
                        turns[outputI] = "through";
                    else if (directions == 3)
                        turns[outputI] = "not_left";
                    else if (directions == 4)
                        turns[outputI] = "left";
                    else if (directions == 5)
                        turns[outputI] = "not_through";
                    else if (directions == 6)
                        turns[outputI] = "not_right";
                    else //if (directions == 7)
                        turns[outputI] = "all";
                    ++outputI;
                }
                --inputI;
            }
            while (outputI < nLanesBack)
                turns[outputI++] = "all";

            // Forward
            inputI = 0;
            outputI = nLanesBack;
            for (; inputI < (int)forward.size() && outputI < nLanes;) {

                if (accessForward[inputI]) {
                    char directions = 0;

                    if (forward[inputI].find("right") != string::npos)
                        directions |= 1;
                    if (forward[inputI].find("through") != string::npos)
                        directions |= 2;
                    if (forward[inputI].find("left") != string::npos)
                        directions |= 4;

                    if (directions == 1)
                        turns[outputI] = "right";
                    else if (directions == 2)
                        turns[outputI] = "through";
                    else if (directions == 3)
                        turns[outputI] = "not_left";
                    else if (directions == 4)
                        turns[outputI] = "left";
                    else if (directions == 5)
                        turns[outputI] = "not_through";
                    else if (directions == 6)
                        turns[outputI] = "not_right";
                    else //if (directions == 7)
                        turns[outputI] = "all";
                    ++outputI;
                }
                ++inputI;
            }
            while (outputI < nLanes)
                turns[outputI++] = "all";

            value["turn"] = turns;
        }
    }

    if (street->tags.count("highway") > 0 && street->tags["highway"].compare("") != 0) {
        value["type"] = street->tags["highway"];
    }

    if (street->tags.count("maxspeed") > 0) {
        value["speed"] = boost::lexical_cast<double>(street->tags["maxspeed"].c_str());
    }

    return value;
}

void TrafficSimulation::communicationThread(VRThread* t) {

    lock_guard<mutex> guardThread(communicationThreadMutex);

    // Running-check is not needed since the thread will only exists while in running state

    // Send data over network
    networkDataMutex.lock();
    if (!dataToSend.isNull()) {
        Value tmp = dataToSend;
        networkDataMutex.unlock();
        Value result = client.sendData(dataToSend);
        errorMessage("sending updates to server", result);
    } else {
        networkDataMutex.unlock();
    }

    // Get view area data
    Value tmp = client.retrieveViewareaData(0);
    networkDataMutex.lock();
    receivedData = tmp;
    networkDataMutex.unlock();
    if (errorMessage("retrieving viewarea data", tmp))
        return;
}

bool TrafficSimulation::errorMessage(const string& action, const Value& value) {

    if (value["error"].isNull()) {
        noConnection = false;
        return false;
    } else {
        if (value["error"].asString() == "COULD_NOT_SEND") {
            if (!noConnection) {
                cerr << "TrafficSimulation: Error while " << action << ": COULD_NOT_SEND\n > " << value["error_message"].asString() << "\n";
                noConnection = true;
            }
        } else {
            cerr << "TrafficSimulation: Error while " << action << ": " << value["error"].asString() << "\n > " << value["error_message"].asString() << "\n";
            noConnection = false;
        }
        return true;
    }
}


TrafficSimulation::TrafficSimulation(MapCoordinator *mapCoordinator, const string& host)
    : driverVehicleRadius(5.0), loadedMaps(), collisionHandler(NULL), client(host), mapCoordinator(mapCoordinator), viewDistance(200), player(NULL), playerCreated(false),
      communicationThreadId(-1), networkDataMutex(), receivedData(), dataToSend(), meshes(), vehicles(), lightBulbs(), noConnection(false), communicationThreadMutex() {

    // Add a dummy model for unknown vehicle types
    VRGeometry* geo = new VRGeometry("vehicle_type_unknown");
    geo->setPersistency(0);
    geo->setPrimitive("Box", "1 1 2 1 1 1");
    meshes[404] = geo;


    a_red = new VRMaterial("a_red");
    a_orange = new VRMaterial("a_orange");
    a_green = new VRMaterial("a_green");
    a_red->setDiffuse(Vec3f(1,0,0));
    a_orange->setDiffuse(Vec3f(1,0.8,0.1));
    a_green->setDiffuse(Vec3f(0,1,0.1));
    a_red->setLit(false);
    a_orange->setLit(false);
    a_green->setLit(false);

    //VRFunction<int>* fkt = new VRFunction<int>( "traffic update fkt", boost::bind(&TrafficSimulation::updateScene, this) );
    //VRSceneManager::getCurrent()->addUpdateFkt(fkt);
}

TrafficSimulation::~TrafficSimulation() {

    // Stop the server
    Value value;
    value["serverState"] = "shutdown";
    value = client.sendData(value);
    errorMessage("shutting down the server", value);

    player = NULL;
    playerCreated = false;

    // Stop the thread
    if (communicationThreadId > 0) {
        VRSceneManager::get()->stopThread(communicationThreadId);
        communicationThreadId = -1;
        communicationThreadMutex.lock();
        communicationThreadMutex.unlock();
    }
}

void TrafficSimulation::setServer(const string& host) {
    client.setServer(host);

    // Retransmit the already loaded maps
    set<const OSMMap*> tmp;
    tmp.swap(loadedMaps);

    for (auto m : tmp) {
        addMap(m);
        //VRFunction<VRThread*>* func = new VRFunction<VRThread*>("trafficAddMap", boost::bind(&realworld::TrafficSimulation::addMap, this, *m));
        //VRSceneManager::get()->initThread(func, "trafficAddMap", false);
    }
}

void TrafficSimulation::addMap(const OSMMap* map) {

    // Check if the map is loaded. If it already is, abort
    if (loadedMaps.count(map) > 0)
        return;

    // Insert map into set
    loadedMaps.insert(map);

    // Iterate over the streets of the map && add them to an array to send to the server
    Value arrayStreets;
    unsigned int iStreets = 0;
    for (vector<OSMWay*>::const_iterator iter = map->osmWays.begin(); iter != map->osmWays.end(); iter++) {

        // Only handle streets && no other ways (e.g. building || park boundaries)
        if ((*iter)->tags.count("highway") && !((*iter)->tags["highway"].empty())) {

            // A vector with the allowed road types
            static set<string> allowedValues;
            if (allowedValues.empty()) {
                // Do this initialisation only once
                allowedValues.insert("motorway");
                allowedValues.insert("trunk");
                allowedValues.insert("primary");
                allowedValues.insert("secondary");
                allowedValues.insert("tertiary ");
                allowedValues.insert("unclassified");
                allowedValues.insert("residential");
                allowedValues.insert("service");
                allowedValues.insert("motorway_link");
                allowedValues.insert("trunk_link");
                allowedValues.insert("primary_link");
                allowedValues.insert("secondary_link");
                allowedValues.insert("tertiary_link");
                allowedValues.insert("living_street");
                allowedValues.insert("track");
                allowedValues.insert("road");
            }

            // Check if it is a street for cars
            if (allowedValues.count((*iter)->tags["highway"]) == 0)
                continue;

            arrayStreets[iStreets++] = convertStreet(*iter);
        }
    }

    // Iterate over the nodes of the map && add them to an array to send to the server
    Value arrayNodes;
    // Iterate over nodes
    for (vector<OSMNode*>::const_iterator iter = map->osmNodes.begin(); iter != map->osmNodes.end(); iter++) {
        uint64_t id = boost::lexical_cast<uint64_t>((*iter)->id.c_str());
        bool found = false;
        // Search in the streets for a reference to this node. If non is found, ignore the node
        for (Value::iterator streetIter = arrayStreets.begin(); streetIter != arrayStreets.end() && !found; streetIter++) {
            for (Value::iterator nodeIter = (*streetIter)["nodes"].begin(); nodeIter != (*streetIter)["nodes"].end() && !found; nodeIter++) {

                if ((*nodeIter).asUInt64() == id) {
                    found = true;
                    break;
                }
            }
        }
        if (found)
            arrayNodes.append(convertNode(*iter));
    }

    Value value;
    value["addNodes"] = arrayNodes;
    value["addStreets"] = arrayStreets;
    value["laneWidth"] = Config::get()->STREET_WIDTH;

    value = client.sendData(value);
    errorMessage("adding map", value);
}

void TrafficSimulation::removeMap(const OSMMap* map) {

    // Check if the map is loaded. If not, abort
    if (loadedMaps.count(map) == 0)
        return;

    // Remove map from set
    loadedMaps.erase(map);

    // Iterate over the streets of the map && add them to an array to send to the server
    Value array;
    unsigned int i = 0;
    for (vector<OSMWay*>::const_iterator iter = map->osmWays.begin(); iter != map->osmWays.end(); iter++) {
        array[i++] = (*iter)->id;
    }

    Value value;
    value["removeStreets"] = array;

    value = client.sendData(value);
    errorMessage("removing streets", value);
}

void TrafficSimulation::setDrawingDistance(const double distance) {

    //VRFunction<OSG::VRThread*>* func = new VRFunction<OSG::VRThread*>("trafficSetDrawingDistance", boost::bind(&realworld::TrafficSimulation::setDrawingDistance, self->obj, b));
    //OSG::VRSceneManager::get()->initThread(func, "trafficSetDrawingDistance", false);

    if (playerCreated) {

        Value value;

        // Remove the old viewarea
        value["removeViewareas"].append(0);

        // Create a new viewarea
        Value area;
        area["id"] = 0;
        area["vehicleId"] = 0;
        area["size"] = distance;

        // Pack as the requested array entry
        value["addViewareas"].append(area);

        value = client.sendData(value);
        errorMessage("recreating a viewarea", value);
        viewDistance = distance;
    } else {
        viewDistance = distance;
    }
}

void TrafficSimulation::setTrafficDensity(const double density) {

    //OSG::VRFunction<VRThread*>* func = new OSG::VRFunction<OSG::VRThread*>("trafficSetTrafficDensity", boost::bind(&realworld::TrafficSimulation::setTrafficDensity, self->obj, b));
    //OSG::VRSceneManager::get()->initThread(func, "trafficSetTrafficDensity", false);

    Value value;
    value["trafficDensity"] = density;
    value = client.sendData(value);
    errorMessage("setting the traffic density", value);
}

void TrafficSimulation::addVehicleType(const unsigned int id, const double probability, const double collisionRadius, const double maxSpeed, const double maxAcceleration, const double maxRoration, VRGeometry *geometry) {


    //void addVehicleType(const unsigned int id, const double probability, const double collisionRadius, const double maxSpeed, const double maxAcceleration, const double maxRoration);
    //OSG::VRFunction<VRThread*>* func = new OSG::VRFunction<VRThread*>("trafficAddVehicleType", boost::bind(&realworld::TrafficSimulation::addVehicleType, self->obj, id, prob, radius, speed, acc, rot, (VRGeometry*)geo->obj));
    //OSG::VRSceneManager::get()->initThread(func, "trafficAddVehicleType", false);


    if (geometry == NULL) {
        cerr << "Given geometry is invalid!\n";
        return;
    }

    // Store the mesh
    meshes[id] = geometry;

    // Send the data about the type to the server
    Value type, value;
    type["id"]          = id;
    type["probability"] = probability;
    type["radius"]      = collisionRadius;
    type["maxSpeed"]    = maxSpeed;
    type["maxAcc"]      = maxAcceleration;
    type["maxRot"]      = maxRoration;

    // Pack as the requested array entry
    value["addVehicleTypes"].append(type);
    value = client.sendData(value);
    errorMessage("adding a vehicle type", value);
}

void TrafficSimulation::addDriverType(const unsigned int id, const double probability, const double lawlessness, const double cautiousness) {

    // void addDriverType(const unsigned int id, const double probability, const double lawlessness, const double cautiousness);
    //VRFunction<VRThread*>* func = new VRFunction<VRThread*>("trafficAddDriverType", boost::bind(&realworld::TrafficSimulation::addDriverType, self->obj, id, prob,  law, caut));
    //VRSceneManager::get()->initThread(func, "trafficAddDriverType", false);

    Value value, type;
    type["id"]           = id;
    type["probability"]  = probability;
    type["lawlessness"]  = lawlessness;
    type["cautiousness"] = cautiousness;

    // Pack as the requested array entry
    value["addDriverTypes"].append(type);
    value = client.sendData(value);
    errorMessage("adding a driver type", value);
}

void TrafficSimulation::start() {
    //VRFunction<VRThread*>* func = new VRFunction<VRThread*>("trafficStart", boost::bind(&realworld::TrafficSimulation::start, self->obj));
    //VRSceneManager::get()->initThread(func, "trafficStart", false);

    if (communicationThreadId < 0) {
        VRFunction<VRThread*>* func = new VRFunction<VRThread*>("trafficCommunicationThread", boost::bind(&TrafficSimulation::communicationThread, this, _1));
        communicationThreadId = VRSceneManager::get()->initThread(func, "trafficCommunicationThread", true);
    }


    Value value;
    value["serverState"] = "running";
    value = client.sendData(value);
    errorMessage("starting the simulation", value);
}

void TrafficSimulation::pause() {

    //VRFunction<VRThread*>* func = new VRFunction<VRThread*>("trafficPause", boost::bind(&realworld::TrafficSimulation::pause, self->obj));
    //VRSceneManager::get()->initThread(func, "trafficPause", false);

    if (communicationThreadId > 0) {
        VRSceneManager::get()->stopThread(communicationThreadId);
        communicationThreadId = -1;
    }

    Value value;
    value["serverState"] = "paused";
    value = client.sendData(value);
    errorMessage("stopping the simulation", value);
}

bool TrafficSimulation::isRunning() {
    return (client.getSimulatorState() == client.RUNNING);
}

void TrafficSimulation::tick() {
    //VRFunction<VRThread*>* func = new VRFunction<VRThread*>("traffic_update", boost::bind(&TrafficSimulation::update, this, _1));
    //VRSceneManager::get()->initThread(func, "traffic_update", false);
    update();
}

Vec3f toVec3f(Json::Value val) { // TODO
    Vec3f v;
    if (!val.isConvertibleTo(arrayValue)) return v;
    //if (!val[0].isConvertibleTo(realValue) || !val[1].isConvertibleTo(realValue) || !val[2].isConvertibleTo(realValue)) return v;
    v[0] = toFloat(val[0].toStyledString());
    v[1] = toFloat(val[0].toStyledString());
    v[2] = toFloat(val[0].toStyledString());
    //Json::Value::toStyledString()
    return v;
}

void TrafficSimulation::update() {

    networkDataMutex.lock();

    // Update the position of the player to be send to the server
    if (player != NULL) {

        // Move the vehicle
        Value vehicle;
        vehicle["id"] = 0;

        Value pos;

        Vec3f worldPosition = player->getFrom();
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
                    cout << "TrafficSimulation: Warning: Received invalid vehicle data.\n";
                    continue;
                }

                uint ID = vehicleIter["id"].asUInt();

                if (vehicles.count(ID) == 0) { // The vehicle is new, create it
                    if (!vehicleIter["vehicle"].isConvertibleTo(uintValue)
                     || !vehicleIter["driver"].isConvertibleTo(uintValue)) {
                        cout << "TrafficSimulation: Warning: Received invalid vehicle data.\n";
                        continue;
                    }

                    uint vID = vehicleIter["vehicle"].asUInt();
                    if (meshes.count(vID) == 0) {
                        // If it is bigger than 500 it is our user-controlled vehicle
                        if (vID < 500) cout << "TrafficSimulation: Warning: Received unknown vehicle type " << vID << ".\n";
                        continue;
                    }

                    Vehicle v;

                    v.id = ID;
                    v.vehicleTypeId = vID;
                    v.driverTypeId = vehicleIter["driver"].asUInt();

                    if (meshes.count(v.vehicleTypeId) == 0) v.vehicleTypeId = 404;
                    v.geometry = (VRGeometry*)meshes[v.vehicleTypeId]->duplicate(true);
                    v.geometry->setPersistency(0);

                    // Add it to the map
                    vehicles.insert(make_pair(v.id, v));

                } else vehicleIDs.erase(ID); // Already (and still) exists, remove its ID from the vehicle-id-set

                // Now the vehicle exists, update its position && state

                Vehicle& v = vehicles[ID];

                //v.pos = toVec3f(vehicleIter["pos"]);
                v.pos = Vec3f(vehicleIter["pos"][0].asFloat(), vehicleIter["pos"][1].asFloat(), vehicleIter["pos"][2].asFloat());
                v.deltaPos = Vec3f(vehicleIter["dPos"][0].asFloat(), vehicleIter["dPos"][1].asFloat(), vehicleIter["dPos"][2].asFloat());
                v.deltaPos *= partDelta;
                v.orientation = Vec3f(vehicleIter["angle"][0].asFloat(), vehicleIter["angle"][1].asFloat(), vehicleIter["angle"][2].asFloat());
                v.deltaOrientation = Vec3f(vehicleIter["dAngle"][0].asFloat(), vehicleIter["dAngle"][1].asFloat(), vehicleIter["dAngle"][2].asFloat());
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
            vehicles[v].geometry->destroy();
            vehicles.erase(v);
        }


        // Get traffic light updates
        if (!receivedData["trafficlights"].isNull() && receivedData["trafficlights"].isArray()) {

            // The light bulbs in the array will be moved around arbitrary whichever light posts are given
            // If there are not enough bulbs in the array, more are added
            // If there are too many bulbs, they are deleted
            size_t bulbIndex = 0;
            static const double postHeight = 2;
            static const double bulbSize   = 1; // Note: If you change this value from 2, change the value further down in new VRGeometry(), too.

            for (auto lightpost : receivedData["trafficlights"]) {
                if (!lightpost.isObject()) continue;

                if (!lightpost["at"].isConvertibleTo(uintValue)
                 || !lightpost["to"].isConvertibleTo(uintValue)
                 ||  lightpost["state"].isNull()
                 || !lightpost["state"].isArray()) {
                    cout << "TrafficSimulation: Warning: Received invalid light post data.\n";
                    continue;
                }

                // Calculate the vector of the street

                // Get the node positions
                Vec2f atPos, toPos;
                string atId = lexical_cast<string>(lightpost["at"].asUInt());
                string toId = lexical_cast<string>(lightpost["to"].asUInt());
                bool foundAt = false, foundTo = false;
                for (auto mapIter : loadedMaps) {
                    for (auto nodeIter : mapIter->osmNodes) {

                        if (!foundAt && nodeIter->id == atId) {
                            atPos = mapCoordinator->realToWorld(Vec2f(nodeIter->lat, nodeIter->lon));
                            foundAt = true;
                        }

                        if (!foundTo && nodeIter->id == toId) {
                            toPos = mapCoordinator->realToWorld(Vec2f(nodeIter->lat, nodeIter->lon));
                            foundTo = true;
                        }

                        if (foundAt && foundTo)
                            break;
                    }
                    if (foundAt && foundTo)
                        break;
                }

                Vec2f streetOffset = toPos - atPos;
                const float prevLength = streetOffset.length();
                streetOffset.normalize();
                streetOffset *= min(prevLength / 2, Config::get()->STREET_WIDTH);

                Vec2f normal(-streetOffset[1], streetOffset[0]);
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
                        if (VRSceneManager::getCurrent() == NULL) break;

                        // Create a new light
                        VRGeometry* geo = new VRGeometry("ampel");
                        geo->setPersistency(0);
                        geo->setPrimitive("Sphere", "0.5 2"); // The first value has to be half of bulbSize
                        geo->setMaterial(a_red);

                        VRSceneManager::getCurrent()->add(geo);
                        lightBulbs.push_back(geo);
                    }

                    // color switch
                    VRGeometry* bulb = lightBulbs[bulbIndex++];
                    Vec3f p = Vec3f(streetOffset[0] + lane * normal[0], postHeight, streetOffset[1] + lane * normal[1]);
                    string lcol = light.asString();
                    if (lcol == "red") {
                        bulb->setWorldPosition(p+Vec3f(0,3 * bulbSize,0));
                        bulb->setMaterial(a_red);

                    } else if (lcol == "redamber") {
                        bulb->setWorldPosition(p+Vec3f(0,3 * bulbSize,0));
                        bulb->setMaterial(a_red);

                        bulb = lightBulbs[bulbIndex++];
                        bulb->setWorldPosition(p+Vec3f(0,2 * bulbSize,0));
                        bulb->setMaterial(a_orange);
                    } else if (lcol == "amber") {
                        bulb->setWorldPosition(p+Vec3f(0,2 * bulbSize,0));
                        bulb->setMaterial(a_orange);
                    } else if (lcol == "green") {
                        bulb->setWorldPosition(p+Vec3f(0,bulbSize,0));
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
    }

    networkDataMutex.unlock();

        // Advance the vehicles a bit
    //cout << "Update " << vehicles.size() << " vehicles\n";
    for (auto v : vehicles) {
        Vec3f p = v.second.pos;
        p[1] = -1.2;//TODO: get right street height
        v.second.geometry->setFrom(p);
        v.second.geometry->setDir(v.second.pos - v.second.orientation);
        v.second.pos += v.second.deltaPos;
        v.second.orientation += v.second.deltaOrientation;
    }

}

void TrafficSimulation::setCollisionHandler(bool (*handler) (Vehicle& a, Vehicle& b)) {
    collisionHandler = handler;
}

void TrafficSimulation::setVehiclePosition(const unsigned int id, const OSG::Vec3f& pos, const OSG::Vec3f& orientation) {


    //VRFunction<VRThread*>* func = new VRFunction<VRThread*>("trafficSetVehiclePosition", boost::bind(&realworld::TrafficSimulation::setVehiclePosition, self->obj, id, pos, rot));
    //VRSceneManager::get()->initThread(func, "trafficSetVehiclePosition", false);

    // Move the vehicle
    Value vehicle;
    vehicle["id"] = id;

    Value valPos;
    valPos[0u] = pos[0];
    valPos[1] = pos[1];
    valPos[2] = pos[2];
    vehicle["pos"] = valPos;

    Value valRot;
    valRot[0u] = orientation[0];
    valRot[1] = orientation[1];
    valRot[2] = orientation[2];
    vehicle["angle"] = valRot;

    // Pack as the requested array entry
    Value value;
    value["moveVehicles"].append(vehicle);
    value = client.sendData(value);
    errorMessage("moving a vehicle", value);
}

void TrafficSimulation::setSimulationSpeed(const double speed) {
    //VRFunction<VRThread*>* func = new VRFunction<VRThread*>("trafficSetSimulationSpeed", boost::bind(&realworld::TrafficSimulation::setSimulationSpeed, self->obj, speed));
    //VRSceneManager::get()->initThread(func, "trafficSetSimulationSpeed", false);

    Value value;
    value["simulationSpeed"] = speed;
    value = client.sendData(value);
    errorMessage("setting the simulation speed", value);
}

void TrafficSimulation::setPlayerTransform(VRTransform *transform) {
    //VRFunction<VRThread*>* func = new VRFunction<VRThread*>("trafficSetPlayerTransform", boost::bind(&realworld::TrafficSimulation::setPlayerTransform, self->obj, _child->obj));
    //VRSceneManager::get()->initThread(func, "trafficSetPlayerTransform", false);

    player = transform;

    if (player != NULL && !playerCreated) {

        // Create a vehicle && a viewarea around it
        Value value;

        // Create the vehicle
        Value vehicle;
        vehicle["id"] = 0;
        Value pos;
        Vec3f worldPosition = player->getWorldPosition();
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
        setVehiclePosition(0, player->getWorldPosition(), OSG::Vec3f(0,0,0));
    } else /* player == NULL */ {

        // Remove area && vehicle
        Value value;
        value["removeViewareas"].append(0);
        value["removeVehicles"].append(0);

        value = client.sendData(value);
        errorMessage("removing the player", value);
        playerCreated = false;
    }
}
