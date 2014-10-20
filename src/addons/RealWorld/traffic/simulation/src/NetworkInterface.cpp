
#define JSON_HAS_INT64
#include "NetworkInterface.h"

#include <iostream>

#ifdef FILE_WRITING
#include <fstream>
#endif // FILE_WRITING

#include "TrafficSimulator.h"

#include "NodeLogicTrafficLight.h"

using namespace std;

/**
 Checks whether a value is a non-negative integer.
 Since the normal code seems to be broken, use this method.
 @param val The value to check.
 @return \c True if the value contains a non-negativ integer.
 */
inline bool isUInt(const Value& val) {
    if (val.isUInt() || (val.isInt() && val.asInt64() >= 0))
        return true;
    else {
        StyledWriter w;
        cout << "Can not convert value to unsigned int:\n" << w.write(val);
        cout << "Value is of type " << val.type() << ".\n";
        return false;
    }
}

/**
 Returns the value as an integer.
 If the value is an unsigned int, the asUInt() function is used.
 @note No check is done whether the value contains an integer.
 @param val The value to transform.
 @return \c True if the value contains a non-negative integer.
 */
inline unsigned int asUInt(const Value& val) {
    if (val.isUInt())
        return val.asUInt64();
    else
        return val.asInt64();
}

const Value NetworkInterface::handlePostRequest(const Value& input) {

#ifdef FILE_WRITING
    // Print all received data to a file
    ofstream file("postLog.json", ios_base::app);
    if (file.is_open()) {
        FastWriter w;
        file << w.write(input);
        file.close();
    }
#endif // FILE_WRITING

    // Acquire a lock
    boost::unique_lock<boost::mutex> lock(mutex);
    // Increase the number of threads waiting
    waitingCount++;
    // Wait/Sleep until there is a system that can be modified
    while (simulator == 0)
        condition.wait(lock);

    // Do the modifications
    Value result;

    // Set server state
    if (input["serverState"].isString()) {
        if (input["serverState"].asString() == "running")
            simulator->setState(TrafficSimulator::RUN);
        else if (input["serverState"].asString() == "paused")
            simulator->setState(TrafficSimulator::PAUSE);
        else if (input["serverState"].asString() == "restart")
            simulator->setState(TrafficSimulator::RESTART);
        else if (input["serverState"].asString() == "shutdown")
            simulator->setState(TrafficSimulator::STOP);
        else {
            result["error"] = "INVALID_VALUE";
            result["error_message"] = "The requested server state is not known.";

            // Decrease the waiting-count
            waitingCount--;
            // Tell other threads that they might work now
            condition.notify_all();
            return result;
        }
    }

    // Set simulation speed
    if (input["simulationSpeed"].isDouble()) {
        double scale = input["simulationSpeed"].asDouble();
        if (scale > 0) {
            timer.setTimeScale(scale);
        } else {
            result["error"] = "INVALID_VALUE";
            result["error_message"] = "The requested simulation speed is invalid.";

            // Decrease the waiting-count
            waitingCount--;
            // Tell other threads that they might work now
            condition.notify_all();
            return result;
        }
    }

    RoadSystem *roadSystem = simulator->getRoadSystem();

    // Add nodes
    if (!input["addNodes"].isNull() && input["addNodes"].isArray()) {
        for (Value::iterator iter = input["addNodes"].begin(); iter != input["addNodes"].end(); iter++) {
            if (!(*iter).isObject())
                continue;

            // Get the id
            if (!isUInt((*iter)["id"])) {
                result["error"] = "INVALID_ID";
                result["error_message"] = "A node needs to have a positive id.";
                waitingCount--;
                condition.notify_all();
                return result;
            }
            ID id = asUInt((*iter)["id"]);

            // If the node already exists, ignore it
            if (roadSystem->hasNode(id))
                continue;

            // Get the position
            Vec2f pos;
            if (!(*iter)["pos"].isNull() && (*iter)["pos"].isArray() && (*iter)["pos"][0].isDouble() && (*iter)["pos"][1].isDouble())
                pos = Vec2f((*iter)["pos"][0].asDouble(), (*iter)["pos"][1].asDouble());
            else {
                result["error"] = "INVALID_POS";
                result["error_message"] = "No position given or in wrong format.";
                waitingCount--;
                condition.notify_all();
                return result;
            }

            // Parse the features
            Node::FEATURE features = Node::NONE;
            if (!(*iter)["features"].isNull() && (*iter)["features"].isArray()) {
                for (Value::iterator iter2 = (*iter)["features"].begin(); iter2 != (*iter)["features"].end(); iter2++) {
                    if ((*iter2).isString() && (*iter2).asString() == "traffic_signals") {
                        features |= Node::TRAFFICLIGHTS;
                    }
                }
            }

            // Try to add the node
            if (!roadSystem->addNode(id, pos, features)) {
                result["error"] = "INVALID_NODE";
                result["error_message"] = "The node could not be added to the road system.";
                waitingCount--;
                condition.notify_all();
                return result;
            }
        }
    }

    // Set lane width
    if (input["laneWidth"].isDouble())
        roadSystem->setLaneWidth(input["laneWidth"].asDouble());

    // Set default speed
    if (input["defaultSpeed"].isDouble())
        roadSystem->setDefaultSpeed(input["defaultSpeed"].asDouble());

    // Set default type
    if (input["defaultType"].isString()) {
        if (input["defaultType"].asString() == "motorway")
            roadSystem->setDefaultType(Street::MOTORWAY);
        else if (input["defaultType"].asString() == "trunk")
            roadSystem->setDefaultType(Street::TRUNK);
        else if (input["defaultType"].asString() == "primary")
            roadSystem->setDefaultType(Street::PRIMARY);
        else if (input["defaultType"].asString() == "motorway_link")
            roadSystem->setDefaultType(Street::MOTORWAY);
        else if (input["defaultType"].asString() == "secondary")
            roadSystem->setDefaultType(Street::SECONDARY);
        else if (input["defaultType"].asString() == "trunk_link")
            roadSystem->setDefaultType(Street::TRUNK);
        else if (input["defaultType"].asString() == "tertiary")
            roadSystem->setDefaultType(Street::TERTIARY);
        else if (input["defaultType"].asString() == "primary_link")
            roadSystem->setDefaultType(Street::PRIMARY);
        else if (input["defaultType"].asString() == "unclassified")
            roadSystem->setDefaultType(Street::UNCLASSIFIED);
        else if (input["defaultType"].asString() == "secondary_link")
            roadSystem->setDefaultType(Street::SECONDARY);
        else if (input["defaultType"].asString() == "tertiary_link")
            roadSystem->setDefaultType(Street::TERTIARY);
        else if (input["defaultType"].asString() == "residential")
            roadSystem->setDefaultType(Street::RESIDENTIAL);
        else if (input["defaultType"].asString() == "service")
            roadSystem->setDefaultType(Street::SERVICE);
        else if (input["defaultType"].asString() == "track")
            roadSystem->setDefaultType(Street::TRACK);
        else if (input["defaultType"].asString() == "living_street")
            roadSystem->setDefaultType(Street::LIVING_STREET);
        else {
            // Either it should really be this type or the type is not known.
            // OSM offers more types than the one used from the simulator
            // so this is not really an error
            roadSystem->setDefaultType(Street::ROAD);
        }


    }


    // Remove streets
    // Remove before adding to free up IDs
    if (!input["removeStreets"].isNull() && input["removeStreets"].isArray()) {
        for (Value::iterator iter = input["removeStreets"].begin(); iter != input["removeStreets"].end(); iter++) {
            if (!isUInt(*iter))
                continue;
            roadSystem->removeStreet(asUInt(*iter));
        }
    }

    // Add streets
    if (!input["addStreets"].isNull() && input["addStreets"].isArray()) {
        for (Value::iterator iter = input["addStreets"].begin(); iter != input["addStreets"].end(); iter++) {

            if (!(*iter).isObject())
                continue;

            // Get the id
            if (!isUInt((*iter)["id"])) {
                result["error"] = "INVALID_ID";
                result["error_message"] = "A street needs to have a positive id.";
                waitingCount--;
                condition.notify_all();
                return result;
            }
            ID id = asUInt((*iter)["id"]);

            // If the street is already known to the system, do not add it again
            if (roadSystem->hasStreet(id))
                continue;

            // Get the nodes
            vector<ID> nodes;
            if (!(*iter)["nodes"].isNull() && (*iter)["nodes"].isArray()) {
                for (Value::iterator nodesIter = (*iter)["nodes"].begin(); nodesIter != (*iter)["nodes"].end(); nodesIter++) {
                    if (isUInt(*nodesIter)) {
                        nodes.push_back(asUInt(*nodesIter));
                    }
                }
            } else {
                result["error"] = "NO_NODES";
                result["error_message"] = "No nodes for street given.";
                waitingCount--;
                condition.notify_all();
                return result;
            }

            if (nodes.size() < 2) {
                result["error"] = "NOT_ENOUGH_NODES";
                result["error_message"] = "A street needs at least two nodes.";
                waitingCount--;
                condition.notify_all();
                return result;
            }

            // Get the lane counts
            unsigned int lanesBackward = 1;
            unsigned int lanesForward = 1;
            if (!(*iter)["lanes"].isNull() && (*iter)["lanes"].isArray() && !((*iter)["lanes"]).isNull()) {
                if (isUInt((*iter)["lanes"][0]) && isUInt((*iter)["lanes"][1])) {
                    lanesForward = asUInt((*iter)["lanes"][0]);
                    lanesBackward = asUInt((*iter)["lanes"][1]);
                } else {
                    result["error"] = "INVALID_LANES";
                    result["error_message"] = "Lane counts are not valid for a street.";
                    waitingCount--;
                    condition.notify_all();
                    return result;
                }
            }

            vector<Street::LANEFLAG> laneFlags;
            for (unsigned int i = 0; i < lanesBackward + lanesForward; ++i)
                laneFlags.push_back(0);

            // Parse the possible lane changes
            if (!(*iter)["change"].isNull() && (*iter)["change"].isArray() && (*iter)["change"].size() > 0) {
                if ((*iter)["change"].size() < lanesBackward + lanesForward) {
                    result["error"] = "INVALID_LANES";
                    result["error_message"] = "Too small number of change possibilities.";
                    waitingCount--;
                    condition.notify_all();
                    return result;
                }

                unsigned  int lane = 0;
                for (Value::iterator iter2 = (*iter)["change"].begin();
                     iter2 != (*iter)["change"].end(), lane < lanesBackward + lanesForward;
                     iter2++, lane++) {
                    if (!(*iter2).isString())
                        continue;
                    if((*iter2).asString() == "right") {
                        laneFlags[lane] = laneFlags[lane] | Street::CHANGE_RIGHT;
                    } else if((*iter2).asString() == "left") {
                        laneFlags[lane] = laneFlags[lane] | Street::CHANGE_LEFT;
                    } else if((*iter2).asString() == "both") {
                        laneFlags[lane] = laneFlags[lane] | Street::CHANGE_RIGHT | Street::CHANGE_LEFT;
                    }
                }
            } else {
                // If no changing information is given, create some

                // If there is only one lane in every direction, allow free changing
                if (lanesBackward == 1 && lanesForward == 1) {
                    laneFlags[0] |= Street::CHANGE_LEFT;
                    laneFlags[1] |= Street::CHANGE_LEFT;
                } else {
                    // If there is more than one lane in a direction, only allow changing between lanes in that direction
                    if (lanesBackward >= 2) {
                        laneFlags[0] |= Street::CHANGE_LEFT;
                        laneFlags[lanesBackward - 1] |= Street::CHANGE_RIGHT;
                        for (unsigned int i = 1; i < lanesBackward; ++i)
                            laneFlags[i] |= Street::CHANGE_LEFT | Street::CHANGE_RIGHT;
                    }
                    if (lanesForward >= 2) {
                        laneFlags[lanesBackward] |= Street::CHANGE_RIGHT;
                        laneFlags[lanesBackward + lanesForward - 1] |= Street::CHANGE_LEFT;
                        for (unsigned int i = lanesBackward + 1; i < lanesBackward + lanesForward - 1; ++i)
                            laneFlags[i] |= Street::CHANGE_LEFT | Street::CHANGE_RIGHT;
                    }
                }
            }

            // Parse the possible turns
            if (!(*iter)["turn"].isNull() && (*iter)["turn"].isArray() && (*iter)["turn"].size() > 0) {
                if ((*iter)["turn"].size() < lanesBackward + lanesForward) {
                    result["error"] = "INVALID_LANES";
                    result["error_message"] = "Too small number of turn possibilities.";
                    waitingCount--;
                    condition.notify_all();
                    return result;
                }

                unsigned int lane = 0;
                for (Value::iterator iter2 = (*iter)["turn"].begin();
                     iter2 != (*iter)["turn"].end(), lane < lanesBackward + lanesForward;
                     iter2++, lane++) {
                    if (!(*iter2).isString())
                        continue;

                    if((*iter2).asString() == "right") {
                        laneFlags[lane] = laneFlags[lane] | Street::TURN_RIGHT;
                    } else if((*iter2).asString() == "left") {
                        laneFlags[lane] = laneFlags[lane] | Street::TURN_LEFT;
                    } else if((*iter2).asString() == "through") {
                        laneFlags[lane] = laneFlags[lane] | Street::TURN_THROUGH;
                    } else if((*iter2).asString() == "not_left") {
                        laneFlags[lane] = laneFlags[lane] | Street::TURN_RIGHT | Street::TURN_THROUGH;
                    } else if((*iter2).asString() == "not_through") {
                        laneFlags[lane] = laneFlags[lane] | Street::TURN_RIGHT | Street::TURN_LEFT;
                    } else if((*iter2).asString() == "not_right") {
                        laneFlags[lane] = laneFlags[lane] | Street::TURN_LEFT | Street::TURN_THROUGH;
                    } else if((*iter2).asString() == "all") {
                        laneFlags[lane] = laneFlags[lane] | Street::TURN_LEFT | Street::TURN_RIGHT| Street::TURN_THROUGH;
                    }
                }
            } else {
                // If no turning information is given, create some

                // The outer lanes will turn in their direction,
                // all but the left lane will drive through

                // Backward
                if (lanesBackward > 0) {
                    laneFlags[0] |= Street::TURN_RIGHT;
                    laneFlags[lanesBackward - 1] |= Street::TURN_LEFT;
                    for (unsigned int i = 0; i < lanesBackward - (lanesBackward > 1?1:0); ++i)
                        laneFlags[i] |= Street::TURN_THROUGH;
                }
                // Forward
                if (lanesForward > 0) {
                    laneFlags[lanesBackward] |= Street::TURN_LEFT;
                    // If it is a one-way street, allow through on all lanes (is the case for most big streets)
                    if (lanesBackward == 0)
                        laneFlags[lanesBackward] |= Street::TURN_THROUGH;
                    laneFlags[lanesBackward + lanesForward - 1] |= Street::TURN_RIGHT;
                    for (unsigned int i = (lanesForward > 1?1:0) + lanesBackward; i < lanesForward + lanesBackward; ++i)
                        laneFlags[i] |= Street::TURN_THROUGH;
                }
            }

            // Set speed
            double speed = -1;

            if ((*iter)["speed"].isDouble())
                speed = (*iter)["speed"].asDouble();

            // Set type
            Street::TYPE type = Street::DEFAULT;

            if ((*iter)["type"].isString()) {
                if ((*iter)["type"].asString() == "motorway")
                    type = Street::MOTORWAY;
                else if ((*iter)["type"].asString() == "trunk")
                    type = Street::TRUNK;
                else if ((*iter)["type"].asString() == "primary")
                    type = Street::PRIMARY;
                else if ((*iter)["type"].asString() == "motorway_link")
                    type = Street::MOTORWAY;
                else if ((*iter)["type"].asString() == "secondary")
                    type = Street::SECONDARY;
                else if ((*iter)["type"].asString() == "trunk_link")
                    type = Street::TRUNK;
                else if ((*iter)["type"].asString() == "tertiary")
                    type = Street::TERTIARY;
                else if ((*iter)["type"].asString() == "primary_link")
                    type = Street::PRIMARY;
                else if ((*iter)["type"].asString() == "unclassified")
                    type = Street::UNCLASSIFIED;
                else if ((*iter)["type"].asString() == "secondary_link")
                    type = Street::SECONDARY;
                else if ((*iter)["type"].asString() == "tertiary_link")
                    type = Street::TERTIARY;
                else if ((*iter)["type"].asString() == "residential")
                    type = Street::RESIDENTIAL;
                else if ((*iter)["type"].asString() == "service")
                    type = Street::SERVICE;
                else if ((*iter)["type"].asString() == "track")
                    type = Street::TRACK;
                else if ((*iter)["type"].asString() == "living_street")
                    type = Street::LIVING_STREET;
            }

            // Try to add the node
            Street *street = roadSystem->createStreet(id, nodes);
            if (street == NULL) {
                result["error"] = "INVALID_STREET";
                result["error_message"] = "The street has already be added to the road system.";
                waitingCount--;
                condition.notify_all();
                return result;
            } else {
                street->setLaneCount(-1, lanesBackward);
                street->setLaneCount(1, lanesForward);
                for (unsigned int i = 0; i < lanesBackward; ++i)
                    street->setLaneFlags(-1 * (int)(lanesBackward - i), laneFlags[i]);
                for (unsigned int i = 1; i <= lanesForward; ++i)
                    street->setLaneFlags(i, laneFlags[lanesBackward + i - 1]);
                street->setType(type);
                street->setMaxSpeed(speed);
                if (!roadSystem->addStreet(street)) {
                    delete street;
                    result["error"] = "INVALID_STREET";
                    result["error_message"] = "The street could not be added to the road system.";
                    waitingCount--;
                    condition.notify_all();
                    return result;
                }
            }
        }
    }

    // Set traffic density
    if (input["trafficDensity"].isDouble())
        roadSystem->setTrafficDensity(input["trafficDensity"].asDouble());


    // Remove vehicle types
    if (!input["removeVehicleTypes"].isNull() && input["removeVehicleTypes"].isArray()) {
        for (Value::iterator iter = input["removeVehicleTypes"].begin(); iter != input["removeVehicleTypes"].end(); iter++) {
            if (!isUInt(*iter))
                continue;
            roadSystem->removeVehicleType(asUInt(*iter));
        }
    }

    // Add vehicle types
    if (!input["addVehicleTypes"].isNull() && input["addVehicleTypes"].isArray()) {
        for (Value::iterator iter = input["addVehicleTypes"].begin(); iter != input["addVehicleTypes"].end(); iter++) {
            if (!(*iter).isObject())
                continue;

            // Get the id
            if (!isUInt((*iter)["id"])) {
                result["error"] = "INVALID_VEHICLE_TYPE";
                result["error_message"] = "A vehicle type needs to have a positive id.";
                waitingCount--;
                condition.notify_all();
                return result;
            }
            ID id = asUInt((*iter)["id"]);

            double probability = 1;
            if ((*iter)["probability"].isDouble()) {
                probability = (*iter)["probability"].asDouble();
            }

            double radius = VEHICLE_LENGTH;
            if ((*iter)["radius"].isDouble()) {
                radius = (*iter)["radius"].asDouble();
            } else {
                result["error"] = "INVALID_VEHICLE_TYPE";
                result["error_message"] = "The radius is no double.";
                waitingCount--;
                condition.notify_all();
                return result;
            }

            double maxSpeed = 130;
            if ((*iter)["maxSpeed"].isDouble()) {
                maxSpeed = (*iter)["maxSpeed"].asDouble();
            } else {
                result["error"] = "INVALID_VEHICLE_TYPE";
                result["error_message"] = "The maxSpeed is no double.";
                waitingCount--;
                condition.notify_all();
                return result;
            }

            double maxAcc = 10;
            if ((*iter)["maxAcc"].isDouble()) {
                maxAcc = (*iter)["maxAcc"].asDouble();
                if (maxAcc < 0)
                    maxAcc = -1 * maxAcc;
            } else {
                result["error"] = "INVALID_VEHICLE_TYPE";
                result["error_message"] = "The maxAcc is no double.";
                waitingCount--;
                condition.notify_all();
                return result;
            }

            double maxRot = 20;
            if ((*iter)["maxRot"].isDouble()) {
                maxRot = (*iter)["maxRot"].asDouble();
            } else {
                result["error"] = "INVALID_VEHICLE_TYPE";
                result["error_message"] = "The maxRot is no double.";
                waitingCount--;
                condition.notify_all();
                return result;
            }

            // Try to add the vehicle type
            if (!roadSystem->addVehicleType(id, probability, radius, maxSpeed, maxAcc, maxRot)) {
                result["error"] = "INVALID_VEHICLE_TYPE";
                result["error_message"] = "The vehicle type could not be added to the road system.";
                waitingCount--;
                condition.notify_all();
                return result;
            }
        }
    }



    // Remove driver types
    if (!input["removeDriverTypes"].isNull() && input["removeDriverTypes"].isArray()) {
        for (Value::iterator iter = input["removeDriverTypes"].begin(); iter != input["removeDriverTypes"].end(); iter++) {
            if (!isUInt(*iter))
                continue;
            roadSystem->removeDriverType(asUInt(*iter));
        }
    }

    // Add driver types
    if (!input["addDriverTypes"].isNull() && input["addDriverTypes"].isArray()) {
        for (Value::iterator iter = input["addDriverTypes"].begin(); iter != input["addDriverTypes"].end(); iter++) {
            if (!(*iter).isObject())
                continue;

            // Get the id
            if (!isUInt((*iter)["id"])) {
                result["error"] = "INVALID_DRIVER_TYPE";
                result["error_message"] = "A driver type needs to have a positive id.";
                waitingCount--;
                condition.notify_all();
                return result;
            }
            ID id = asUInt((*iter)["id"]);

            double probability = 1;
            if ((*iter)["probability"].isDouble()) {
                probability = (*iter)["probability"].asDouble();
            }

            double lawlessness = 0.5;
            if ((*iter)["lawlessness"].isDouble()) {
                lawlessness = (*iter)["lawlessness"].asDouble();
                if (lawlessness < 0)
                    lawlessness = 0;
                else if (lawlessness > 1)
                    lawlessness = 1;
            } else {
                result["error"] = "INVALID_DRIVER_TYPE";
                result["error_message"] = "The lawlessness is no double.";
                waitingCount--;
                condition.notify_all();
                return result;
            }

            double cautiousness = 1;
            if ((*iter)["cautiousness"].isDouble()) {
                cautiousness = (*iter)["cautiousness"].asDouble();
                if (cautiousness < 0)
                    cautiousness = 0;
                else if (cautiousness > 1)
                    cautiousness = 1;
            } else {
                result["error"] = "INVALID_DRIVER_TYPE";
                result["error_message"] = "The cautiousness is no double.";
                waitingCount--;
                condition.notify_all();
                return result;
            }

            // Try to add the driver type
            if (!roadSystem->addDriverType(id, probability, lawlessness, cautiousness)) {
                result["error"] = "INVALID_DRIVER_TYPE";
                result["error_message"] = "The driver type could not be added to the road system.";
                waitingCount--;
                condition.notify_all();
                return result;
            }
        }
    }


    // Remove some vehicles
    if (!input["removeVehicles"].isNull() && input["removeVehicles"].isArray()) {
        for (Value::iterator iter = input["removeVehicles"].begin(); iter != input["removeVehicles"].end(); iter++) {
            if (!isUInt(*iter))
                continue;
            roadSystem->removeVehicle(asUInt(*iter));
        }
    }

    // Add client controlled vehicles
    if (!input["addVehicles"].isNull() && input["addVehicles"].isArray()) {

        for (Value::iterator iter = input["addVehicles"].begin(); iter != input["addVehicles"].end(); iter++) {
            if (!(*iter).isObject())
                continue;

            // Get the id
            if (!isUInt((*iter)["id"])) {
                result["error"] = "INVALID_ID";
                result["error_message"] = "A vehicle needs to have a positive id.";
                waitingCount--;
                condition.notify_all();
                return result;
            }
            ID id = asUInt((*iter)["id"]);

            double radius = 0;
            if ((*iter)["radius"].isConvertibleTo(realValue)) {
                radius = (*iter)["radius"].asDouble();
                if (radius < 0)
                    radius = 0;
            } else {
                result["error"] = "INVALID_VEHICLE";
                result["error_message"] = "The radius is no double.";
                waitingCount--;
                condition.notify_all();
                return result;
            }

            // Get the position
            Vec3f pos;
            if (!(*iter)["pos"].isNull() && (*iter)["pos"].isArray() && (*iter)["pos"][0].isDouble() && (*iter)["pos"][1].isDouble())
                pos = Vec3f((*iter)["pos"][0].asDouble(), 0, (*iter)["pos"][2].asDouble());
            else {
                result["error"] = "INVALID_VEHICLE";
                result["error_message"] = "Position given is in wrong format.";
                waitingCount--;
                condition.notify_all();
                return result;
            }

            // Try to add it
            if (!roadSystem->addVehicle(id, pos, radius)) {
                result["error"] = "INVALID_VEHICLE";
                result["error_message"] = "The vehicle could not be added to the road system.";
                waitingCount--;
                condition.notify_all();
                return result;
            } else {
                // Mark the vehicle as free
                roadSystem->getFreeVehicles()->insert(id);
            }
        }
    }

    // Move client controlled vehicles
    if (!input["moveVehicles"].isNull() && input["moveVehicles"].isArray()) {
        for (Value::iterator iter = input["moveVehicles"].begin(); iter != input["moveVehicles"].end(); iter++) {
            if (!(*iter).isObject())
                continue;

            // Get the id
            if (!isUInt((*iter)["id"])) {
                result["error"] = "INVALID_ID";
                result["error_message"] = "A vehicle needs to have a positive id.";
                waitingCount--;
                condition.notify_all();
                return result;
            }
            ID id = asUInt((*iter)["id"]);

            // Get the vehicle
            Vehicle *v = roadSystem->getVehicle(id);
            if (v == NULL) {
                result["error"] = "INVALID_VEHICLE";
                result["error_message"] = "The vehicle could not be found.";
                waitingCount--;
                condition.notify_all();
                return result;
            }

            // Get the position
            if (!(*iter)["pos"].isNull() && (*iter)["pos"].isArray()) {
                if ((*iter)["pos"][0].isDouble() && (*iter)["pos"][1].isDouble()) {
                    Vec3f pos;
                    pos = Vec3f((*iter)["pos"][0].asDouble(), 0, (*iter)["pos"][2].asDouble());
                    v->setPosition(pos);
                } else {
                    result["error"] = "INVALID_VEHICLE";
                    result["error_message"] = "Position given is in wrong format.";
                    waitingCount--;
                    condition.notify_all();
                    return result;
                }
            }

            // Get the angle
            if (!(*iter)["angle"].isNull() && (*iter)["angle"].isArray()) {
                if ((*iter)["angle"][0].isDouble() && (*iter)["angle"][1].isDouble() && (*iter)["angle"][2].isDouble() && (*iter)["angle"][3].isDouble()) {
                    Quaternion angle;
                    angle = Quaternion((*iter)["angle"][0].asDouble(), (*iter)["angle"][1].asDouble(), (*iter)["angle"][2].asDouble(), (*iter)["angle"][3].asDouble());
                    v->setOrientation(angle);
                } else {
                    result["error"] = "INVALID_VEHICLE";
                    result["error_message"] = "Angle given is in wrong format.";
                    waitingCount--;
                    condition.notify_all();
                    return result;
                }
            }
        }
    }

    // Release client controlled vehicles
    if (!input["releaseVehicles"].isNull() && input["releaseVehicles"].isArray()) {
        for (Value::iterator iter = input["releaseVehicles"].begin(); iter != input["releaseVehicles"].end(); iter++) {
            if (!(*iter).isObject())
                continue;

            // Get the id
            if (!isUInt((*iter)["id"])) {
                result["error"] = "INVALID_ID";
                result["error_message"] = "A vehicle needs to have a positive id.";
                waitingCount--;
                condition.notify_all();
                return result;
            }
            ID id = asUInt((*iter)["id"]);

            // Try to find it
            Vehicle *v = roadSystem->getVehicle(id);
            if (v == NULL) {
                result["error"] = "INVALID_VEHICLE";
                result["error_message"] = "The vehicle could not be found.";
                waitingCount--;
                condition.notify_all();
                return result;
            }

            // Set the controller to simulation controlled
            v->setController(0);

            // Mark the vehicle as no longer free
            roadSystem->getFreeVehicles()->erase(v->getId());

            // Find the nearest node and set it as destination
            Node *minNode = NULL;
            double minDistance = numeric_limits<double>::max();
            for (map<ID, Node*>::const_iterator nodeIter = roadSystem->getNodes()->begin(); nodeIter != roadSystem->getNodes()->end(); ++nodeIter) {
                double dist = calcDistance(v->getPosition(), nodeIter->second->getPosition());
                if (dist < minDistance) {
                    minDistance = dist;
                    minNode = nodeIter->second;
                }
            }

            // Select a random street at it
            Street *street = roadSystem->getStreet(minNode->getStreetIds()[rand() % minNode->getStreetIds().size()]);

            // Find out on which side of the street we are
            // "Evil" hack: Get the lane-offset at the node and look which one is nearer to our position
            int lane = street->getLaneCount(1);
            Vec2f destPosition = street->getRelativeNodePosition(minNode->getId(), lane);
            if (calcDistance(destPosition, v->getPosition()) > minDistance) {
                // Wrong side
                lane = street->getLaneCount(-1);
                destPosition = street->getRelativeNodePosition(minNode->getId(), lane);
            }

            v->setCurrentDestination(destPosition);
            v->setStreet(street->getId(), lane);
            ID minNodeI = street->getNodeIndex(minNode->getId());
            ID prevNode;
            if (lane < 0) {
                if (minNodeI > 0)
                    prevNode = (*(street->getNodeIds()))[minNodeI - 1];
                else
                    prevNode = (*(street->getNodeIds()))[minNodeI + 1];
            } else /* lane > 0 */ {
                if (minNodeI < street->getNodeIds()->size() - 1)
                    prevNode = (*(street->getNodeIds()))[minNodeI + 1];
                else
                    prevNode = (*(street->getNodeIds()))[minNodeI - 1];
            }

            v->getRoute()->push_back(prevNode);
            v->getRoute()->push_back(minNode->getId());

            // Get and set the driver
            if (isUInt((*iter)["driverType"])) {
                v->setDriverType(asUInt((*iter)["driverType"]));
            }

            // Get and set the vehicle type
            if (isUInt((*iter)["vehicleType"])) {
                v->setVehicleType(asUInt((*iter)["vehicleType"]));
            } else {
                roadSystem->removeDriverType(500 + id);
                v->setVehicleType(404);
            }
        }
    }

    // Handle crashed vehicles
    if (!input["collision"].isNull() && input["collision"].isArray()) {
        for (Value::iterator iter = input["collision"].begin(); iter != input["collision"].end(); iter++) {
            if (!isUInt(*iter))
                continue;
            Vehicle *v = roadSystem->getVehicle(asUInt(*iter));
            if (v != NULL) {
                if (v->getController() >= 0)
                    v->setController(180 + (rand() % 360));
            }
        }
    }


    // Remove viewarea
    if (!input["removeViewareas"].isNull() && input["removeViewareas"].isArray()) {
        for (Value::iterator iter = input["removeViewareas"].begin(); iter != input["removeViewareas"].end(); iter++) {
            if (!isUInt(*iter))
                continue;
            roadSystem->removeViewarea(asUInt(*iter));
        }
    }

    // Add viewarea
    if (!input["addViewareas"].isNull() && input["addViewareas"].isArray()) {
        for (Value::iterator iter = input["addViewareas"].begin(); iter != input["addViewareas"].end(); iter++) {
            if (!(*iter).isObject())
                continue;

            // Get the id
            if (!isUInt((*iter)["id"])) {
                result["error"] = "INVALID_ID";
                result["error_message"] = "A viewarea needs to have a positive id.";
                waitingCount--;
                condition.notify_all();
                return result;
            }
            ID id = asUInt((*iter)["id"]);

            double size = 0;
            if ((*iter)["size"].isDouble() || (*iter)["size"].isInt()) {
                size = (*iter)["size"].asDouble();

                if (size < 0)
                    size = 0;
            } else {
                result["error"] = "INVALID_VIEWAREA";
                result["error_message"] = "The size is no double.";
                waitingCount--;
                condition.notify_all();
                return result;
            }

            // Get the position
            // Either a static position
            if (!(*iter)["pos"].isNull() && (*iter)["pos"].isArray()) {

                Vec2f pos;

                if (((*iter)["pos"][0].isDouble() || (*iter)["pos"][0].isInt()) && ((*iter)["pos"][1].isDouble() || (*iter)["pos"][1].isInt()))
                    pos = Vec2f((*iter)["pos"][0].asDouble(), (*iter)["pos"][1].asDouble());
                else {
                    result["error"] = "INVALID_VIEWAREA";
                    result["error_message"] = "Position given is in wrong format.";
                    waitingCount--;
                    condition.notify_all();
                    return result;
                }

                // Try to add it
                if (!roadSystem->addViewarea(id, pos, size)) {
                    result["error"] = "INVALID_VIEWAREA";
                    result["error_message"] = "The viewarea could not be added to the road system.";
                    waitingCount--;
                    condition.notify_all();
                    return result;
                }
            } else if (isUInt((*iter)["vehicleId"])) {

                ID vehicleId = asUInt((*iter)["vehicleId"]);

                // Try to add it
                if (!roadSystem->addViewarea(id, vehicleId, size)) {
                    result["error"] = "INVALID_VIEWAREA";
                    result["error_message"] = "The viewarea could not be added to the road system.";
                    waitingCount--;
                    condition.notify_all();
                    return result;
                }
            } else {
                    result["error"] = "INVALID_VIEWAREA";
                    result["error_message"] = "The viewarea needs to have a position or a vehicle id.";
                    waitingCount--;
                    condition.notify_all();
                    return result;
            }

        }
    }

    // Move a viewarea
    if (!input["moveViewareas"].isNull() && input["moveViewareas"].isArray()) {
        for (Value::iterator iter = input["moveViewareas"].begin(); iter != input["moveViewareas"].end(); iter++) {
            if (!(*iter).isObject())
                continue;

            // Get the id
            if (!isUInt((*iter)["id"])) {
                result["error"] = "INVALID_ID";
                result["error_message"] = "A viewarea needs to have a positive id.";
                waitingCount--;
                condition.notify_all();
                return result;
            }
            ID id = asUInt((*iter)["id"]);

            // Get the position
            Vec2f pos;

            if (!(*iter)["pos"].isNull() && (*iter)["pos"].isArray() && (*iter)["pos"][0].isDouble() && (*iter)["pos"][1].isDouble())
                pos = Vec2f((*iter)["pos"][0].asDouble(), (*iter)["pos"][1].asDouble());
            else {
                result["error"] = "INVALID_VIEWAREA";
                result["error_message"] = "Position given is in wrong format.";
                waitingCount--;
                condition.notify_all();
                return result;
            }

            // Try to move it
            if (!roadSystem->moveViewarea(id, pos)) {
                result["error"] = "INVALID_VIEWAREA";
                result["error_message"] = "The viewarea could not be move.";
                waitingCount--;
                condition.notify_all();
                return result;
            }
        }
    }


    // Decrease the waiting-count
    waitingCount--;
    // Tell other threads that they might work now
    condition.notify_all();

    // Return the result to the network-client
    return result;

}

IJsonServerListener::SIMULATORSTATE NetworkInterface::getSimulatorState() {
    // Acquire a lock
    boost::unique_lock<boost::mutex> lock(mutex);
    // Increase the number of threads waiting
    waitingCount++;
    // Wait/Sleep until there is a system that can be modified
    while (simulator == 0)
        condition.wait(lock);

    // Fetch the state
    TrafficSimulator::STATE simState = simulator->getState();
    IJsonServerListener::SIMULATORSTATE netState;

    switch(simState) {
        case TrafficSimulator::RUN:
            netState = IJsonServerListener::RUNNING;
            break;
        case TrafficSimulator::PAUSE:
            netState = IJsonServerListener::PAUSED;
            break;
        case TrafficSimulator::STOP:
            netState = IJsonServerListener::SHUTDOWN;
            break;
        case TrafficSimulator::RESTART:
            netState = IJsonServerListener::RESTART;
            break;
    };

    // Decrease the waiting-count
    waitingCount--;
    // Tell other threads that they might work now
    condition.notify_all();

    // Return the result to the network-client
    return netState;
}

const Value NetworkInterface::getViewareaData(const unsigned int id) {
    // Acquire a lock
    boost::unique_lock<boost::mutex> lock(mutex);
    // Increase the number of threads waiting
    waitingCount++;
    // Wait/Sleep until there is a system that can be modified
    while (simulator == 0)
        condition.wait(lock);

    Value result;

    // Get the data
    RoadSystem *roadSystem = simulator->getRoadSystem();
    const RoadSystem::ViewArea *viewArea = roadSystem->getViewarea(id);
    if (viewArea == NULL) {
        result["error"] = "INVALID_ID";
        result["error_message"] = "The viewarea with the given id could not be found.";
        waitingCount--;
        condition.notify_all();
        return result;
    }

    result["id"] = id;

    // Iterate over all vehicles and add all vehicles to the output
    // that are within viewArea->radius of viewArea->position
    const map<ID, Vehicle*> *vehicles = roadSystem->getVehicles();
    for (map<ID, Vehicle*>::const_iterator iter = vehicles->begin(); iter != vehicles->end(); ++iter) {

        if (calcDistance(viewArea->position, iter->second->getPosition()) > viewArea->radius)
            continue;

        Value v;
        v["id"] = iter->second->getId();
        v["vehicle"] = iter->second->getVehicleType();
        v["driver"] = iter->second->getDriverType();
        v["pos"][0] = iter->second->getPosition()[0];
        v["pos"][1] = iter->second->getPosition()[1];
        v["pos"][2] = iter->second->getPosition()[2];
        v["angle"][0] = iter->second->getFuturePosition()[0];
        v["angle"][1] = iter->second->getFuturePosition()[1];
        v["angle"][2] = iter->second->getFuturePosition()[2];

        v["dPos"][0] = iter->second->getFuturePosition()[0] - iter->second->getPosition()[0];
        v["dPos"][1] = iter->second->getFuturePosition()[1] - iter->second->getPosition()[1];
        v["dPos"][2] = iter->second->getFuturePosition()[2] - iter->second->getPosition()[2];

        v["dAngle"][0] = iter->second->getFuturePosition()[0] - iter->second->getPosition()[0];
        v["dAngle"][1] = iter->second->getFuturePosition()[1] - iter->second->getPosition()[1];
        v["dAngle"][2] = iter->second->getFuturePosition()[2] - iter->second->getPosition()[2];

        Vehicle::STATE state = iter->second->getState();
        if (state & Vehicle::TURN_LEFT)
            v["state"].append("leftIndicator");
        if (state & Vehicle::TURN_RIGHT)
            v["state"].append("leftIndicator");
        if (state & Vehicle::BRAKING)
            v["state"].append("braking");
        if (state & Vehicle::COLLIDING)
            v["state"].append("collision");
        if (state & Vehicle::CRASHED)
            v["state"].append("crashed");

        for (set< pair<ID, ID> >::iterator iter2 = roadSystem->getCollisions()->begin(); iter2 != roadSystem->getCollisions()->end(); ++iter2) {
            if (iter2->first == iter->second->getId())
                v["colliding"].append(iter2->second);
            else if (iter2->second == iter->second->getId())
                v["colliding"].append(iter2->first);
        }

        // Append vehicle to list
        result["vehicles"].append(v);
    }

    // Add the traffic lights

    const set<NodeLogic*>* logics = roadSystem->getNodeLogics();
    for (set<NodeLogic*>::const_iterator iter = logics->begin(); iter != logics->end(); ++iter) {

        if ((*iter)->getType() != NodeLogic::TRAFFIC_LIGHT)
            continue;

        NodeLogicTrafficLight *trafficLight = static_cast<NodeLogicTrafficLight*>(*iter);

        if (calcDistance(viewArea->position, trafficLight->getPosition()) > viewArea->radius)
            continue;

        // Add the traffic light
        const set<ID>& lightNodes = trafficLight->getTrafficLights();

        for (set<ID>::const_iterator nodeIter = lightNodes.begin(); nodeIter != lightNodes.end(); ++nodeIter) {
            const vector<NodeLogicTrafficLight::LightPost>& posts = trafficLight->getLightPostState(*nodeIter);

            for (vector<NodeLogicTrafficLight::LightPost>::const_iterator post = posts.begin(); post != posts.end(); ++post) {

                Value postValue;
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

                if (!error)
                    result["trafficlights"].append(postValue);
            }

        }

    }

    // All collisions transferred for now, clear the list
    roadSystem->clearCollisions();


    // Decrease the waiting-count
    waitingCount--;
    // Tell other threads that they might work now
    condition.notify_all();

    // Return the result to the network-client
    return result;
}

NetworkInterface::NetworkInterface()
    : server(), mutex(), condition(), waitingCount(0), simulator(0) {
    server.setListener(this);
}

void NetworkInterface::start() {
    if (!isRunning())
        server.start(5550);
}

void NetworkInterface::stop() {
    server.stop();
}

bool NetworkInterface::isRunning() {
    return server.isRunning();
}

void NetworkInterface::applyChanges(TrafficSimulator *trafficSimulator) {
    // Acquire the lock for the threads
    boost::unique_lock<boost::mutex> lock(mutex);
    // Set the system that should be modified
    simulator = trafficSimulator;
    // While there are waiting threads...
    while (waitingCount > 0) {
        // Notify them
        condition.notify_all();
        // And wait some time
        condition.wait(lock);
    }
    // Set the system to NULL to avoid further modifications
    simulator = NULL;
    // Auto-release the lock
}
