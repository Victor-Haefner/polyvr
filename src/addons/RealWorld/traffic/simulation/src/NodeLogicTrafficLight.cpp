#include "NodeLogicTrafficLight.h"

#include <boost/lexical_cast.hpp>

using boost::lexical_cast;

#include "RoadSystem.h"

const int NodeLogicTrafficLight::PHASE_DURATION_GREEN = 30*1000;
const int NodeLogicTrafficLight::PHASE_DURATION_AMBER = 3*1000;
const int NodeLogicTrafficLight::PHASE_DURATION_RED_AMBER = 2*1000;
const int NodeLogicTrafficLight::PHASE_DURATION_ALL_RED = 2*1000;


void NodeLogicTrafficLight::applyPhase() {

    if (phases.size() <= 0)
            return;

    const vector< pair<size_t, size_t> >& phase = phases[currentPhase % phases.size()].second;

    for (vector< pair<size_t, size_t> >::const_iterator iter = phase.begin(); iter != phase.end(); ++iter) {
        // Find the post with the given values
        switch(lightPosts[iter->first].laneStates[iter->second - 1]) {
            case GREEN:
                lightPosts[iter->first].laneStates[iter->second - 1] = AMBER;
                break;
            case AMBER:
                lightPosts[iter->first].laneStates[iter->second - 1] = RED;
                break;
            case RED:
                lightPosts[iter->first].laneStates[iter->second - 1] = RED_AMBER;
                break;
            case RED_AMBER:
                lightPosts[iter->first].laneStates[iter->second - 1] = GREEN;
                break;
            default:
                break;
        }
    }
}

size_t NodeLogicTrafficLight::getLightPost(const ID node, const ID facing) const {
    size_t position = 0;
    for (; position < lightPosts.size(); ++position)
        if (lightPosts[position].node == node && lightPosts[position].facing == facing)
            break;
    return position;
}

void NodeLogicTrafficLight::addNode(const Node *node) {

    // If node is already registered ignore it
    if (trafficLights.count(node->getId()))
        return;

    // Move center to the middle of the nodes
    center = (center * trafficLights.size() + node->getPosition()) / (trafficLights.size() + 1);

    // Adapt the radius
    double distance = calcDistance(center, node->getPosition());
    radius = (radius * trafficLights.size() + distance) / (trafficLights.size() + 1);

    trafficLights.insert(node->getId());

    // Check if any light post are at invalid positions now that the center has moved
    for (vector<LightPost>::iterator iter = lightPosts.begin(); iter != lightPosts.end(); ) {
        if (calcDistance(roadSystem->getNode(iter->node)->getPosition(), center) > calcDistance(roadSystem->getNode(iter->facing)->getPosition(), center)) {
            lightPosts.erase(iter++);
        } else {
            ++iter;
        }
    }

    // Add all streets of the new node
    const vector<ID>& streets = node->getStreetIds();
    for (unsigned int i = 0; i < streets.size(); ++i)
        createLightPost(node, roadSystem->getStreet(streets[i]));

    // Recalculate the light phases
    rebuildLightPhases();
}

bool NodeLogicTrafficLight::createLightPost(const Node* node, const Street* street) {

    size_t index = getLightPost(node->getId(), street->getId());
    if (index < lightPosts.size())
        return false;

    // If not found, initialize a new object

    double distNodeCrossroad = calcDistance(node->getPosition(), center);

    // Search the street inside the Connections
    for (multiset<Node::Connection>::iterator iter = node->connections.begin(); iter != node->connections.end(); ++iter) {
        if (iter->street == street->getId() && iter->flags & Node::Connection::INCOMING) {

            // If the next node is further away than the calling node, we have found our connection.
            // Not that we care about the connection as such, but through it we can get the next node in its
            // direction which we need to check if it is on the right side of the traffic-light node.
            // If it isn't, then we ignore this Connection since the post would be created at the middle of the crossing
            if (distNodeCrossroad < calcDistance(roadSystem->getNode(iter->node)->getPosition(), center)) {

                LightPost post;
                post.node = node->getId();
                post.street = iter->street;
                post.facing = iter->node;

                if (iter->flags & Node::Connection::FORWARD) {
                    post.direction = 1;
                    for (unsigned int laneI = 1; laneI <= street->getLaneCount(1); ++laneI)
                        post.laneStates.push_back(RED);
                } else {
                    post.direction = -1;
                    for (unsigned int laneI = 1; laneI <= street->getLaneCount(-1); ++laneI)
                        post.laneStates.push_back(RED);
                }

                lightPosts.push_back(post);
            }
        }
    }
    return true;
}

bool NodeLogicTrafficLight::removeLightPost(const Node* node, const Street* street) {

    size_t index = getLightPost(node->getId(), street->getId());
    if (index < lightPosts.size())
        return false;

    // If not the last element, overwrite it with the last one
    if (index < lightPosts.size() - 1)
        lightPosts[index] = lightPosts[lightPosts.size() - 1];

    // Drop the last element
    lightPosts.pop_back();

    return true;

}

void NodeLogicTrafficLight::rebuildLightPhases() {

    // The order in the set might lead to suboptimal combinations which in turn might lead to
    // one pair and two single light-posts

    // Find opposite traffic lights
    set< pair<size_t, size_t> > pairs;

    // A set to store the indices inside lightPosts and their angles
    // These angles can not be stored permanently since they change if another node is added and the center moves
    set< pair<int, size_t> > angles;

    if (lightPosts.size() == 2) {
        // If there are only two light posts, hard-set the angles to 0 and 180
        // This will result in creating a pair / one phase out of them later on
        angles.insert(make_pair(0, 0));
        angles.insert(make_pair(180, 1));
    } else {

        // Else calculate them based on the position of the middle
        for (size_t i = 0; i < lightPosts.size(); ++i) {
            angles.insert(make_pair(calcAngle(roadSystem->getNode(lightPosts[i].facing)->getPosition() - center), i));
        }
    }



    set< pair<int, size_t> > unmatched(angles);

    // Take one element out of the set and search for the street which is most opposite
    while (!unmatched.empty()) {
        // Get one element
        pair<size_t, int> post = *(unmatched.begin());
        unmatched.erase(unmatched.begin());
        // Search for the best match
        set< pair<int, size_t> >::iterator minIter;
        int minDelta = 900;
        for (set< pair<int, size_t> >::iterator setIter = unmatched.begin(); setIter != unmatched.end(); ++setIter) {
            int delta = (post.first - setIter->first + 360 + 180) % 360;

            if (delta < minDelta) {
                minDelta = delta;
                minIter = setIter;
            }
            if (360 - delta < minDelta) {
                minDelta = 360 - delta;
                minIter = setIter;
            }
        }
        if (minDelta < 45 || (minDelta > 360-45 && minDelta < 361)) {
            // Found a partner
            pairs.insert(make_pair(post.second, minIter->second));
            unmatched.erase(minIter);
        } else {
            // Sadly no partner...
            pairs.insert(make_pair(post.second, post.second));
        }
    }


    /**
     Basic idea of the phases:
     - One phases where turn-left-lanes can drive and their matching turn-right-counterparts
     - Multiple phases where each light-post-pair gets a phase (or to be exact: four phases) where all through-lanes of them are allowed to drive
     - In the middle of those one phases where turn-right-lanes can drive and their matching turn-left-counterparts
     */
    // Possible improvement: Add an GREEN_TURNING light-state where the e.g. left-turners of a left/through lane can drive together with the
    // vehicles on the left-only lane.

    // Reset the phases
    phases.clear();
    // This will be used as a counter for the start times inside this method, too.
    // Set the first start time to the length of the amber phase, so the last phase has some time, too.
    circleTime = PHASE_DURATION_AMBER;
    timeInPhase = 0;
    currentPhase = 0;

    // Reset all lights to red
    for (size_t lightI = 0; lightI < lightPosts.size(); ++lightI) {
        for (size_t laneI = 0; laneI < lightPosts[lightI].laneStates.size(); ++laneI)
            lightPosts[lightI].laneStates[laneI] = RED;
    }

    // Run through the connected street two times: In the first round, search for left-only lanes and
    // their matching right-only lanes on the next street. In the second round, it is done the other way round
    vector< pair<size_t, size_t> > leftOnlyPhase;
    vector< pair<size_t, size_t> > rightOnlyPhase;

    // Create phases for left-only and right-only lanes
    // Iterate over lightPosts
    bool left = true;
    for (set< pair<int, size_t> >::iterator setIter = angles.begin(); setIter != angles.end(); ++setIter) {
        Street *street = roadSystem->getStreet(lightPosts[setIter->second].street);
        // Iterate over lanes and search for *-only lanes
        for (unsigned int laneI = 1; laneI <= street->getLaneCount(lightPosts[setIter->second].direction); ++laneI) {

            Street::LANEFLAG flags = street->getLaneFlags(lightPosts[setIter->second].direction * laneI);

            if ((left && flags & Street::TURN_LEFT && !(flags & Street::TURN_THROUGH) && !(flags & Street::TURN_RIGHT))
                // This is not correct, it SHOULD be the out-commented line. But it does not work with it currently since for the vehicles waiting
                // at the light post all ways are THROUGH at the center crossing
                //|| (!left && flags & Street::TURN_RIGHT && !(flags & Street::TURN_THROUGH) && !(flags & Street::TURN_LEFT))) {
                || (!left && flags & Street::TURN_RIGHT)) {
                // Add it to the list of lights that should switch this time frame
                // Add it twice so it switches from RED to GREEN in one step
                leftOnlyPhase.push_back(make_pair(setIter->second, laneI));
                leftOnlyPhase.push_back(make_pair(setIter->second, laneI));
            }
        }
        left = !left;
    }
    left = false;
    for (set< pair<int, size_t> >::iterator setIter = angles.begin(); setIter != angles.end(); ++setIter) {
        Street *street = roadSystem->getStreet(lightPosts[setIter->second].street);
        // Iterate over lanes and search for *-only lanes
        for (unsigned int laneI = 1; laneI <= street->getLaneCount(lightPosts[setIter->second].direction); ++laneI) {

            Street::LANEFLAG flags = street->getLaneFlags(lightPosts[setIter->second].direction * laneI);

            if ((left && flags & Street::TURN_LEFT && !(flags & Street::TURN_THROUGH) && !(flags & Street::TURN_RIGHT))
                //|| (!left && flags & Street::TURN_RIGHT && !(flags & Street::TURN_THROUGH) && !(flags & Street::TURN_LEFT))) {
                || (!left && flags & Street::TURN_RIGHT)) {
                rightOnlyPhase.push_back(make_pair(setIter->second, laneI));
                rightOnlyPhase.push_back(make_pair(setIter->second, laneI));
            }
        }
        left = !left;
    }

    // If something has been created, add it
    if (!leftOnlyPhase.empty()) {
        phases.push_back(make_pair(PHASE_DURATION_GREEN, leftOnlyPhase));
        circleTime += PHASE_DURATION_GREEN;
        // The all-red phase might become longer if the crossing is big and the vehicles need more time to leave it
        phases.push_back(make_pair(PHASE_DURATION_ALL_RED * min(radius / CROSSROAD_RADIUS, 1.0), leftOnlyPhase));
        circleTime += PHASE_DURATION_ALL_RED * min(radius / CROSSROAD_RADIUS, 1.0);
    }

    // Take pairs from the list
    vector< pair<size_t, size_t> > phase;
    for (set< pair<size_t, size_t> >::iterator setIter = pairs.begin(); setIter != pairs.end(); ++setIter) {
        Street *streetFirst = roadSystem->getStreet(lightPosts[setIter->first].street);
        Street *streetSecond = roadSystem->getStreet(lightPosts[setIter->second].street);
        // Red-Amber
        for (unsigned int laneI = 1; laneI <= streetFirst->getLaneCount(lightPosts[setIter->first].direction); ++laneI)
            if (streetFirst->getLaneFlags(laneI * lightPosts[setIter->first].direction) & Street::TURN_THROUGH)
                phase.push_back(make_pair(setIter->first, laneI));
        // If the second is not the same as the first, switch it, too
        // If they are equal, than the post has no partner
        if (setIter->first != setIter->second)
            for (unsigned int laneI = 1; laneI <= streetSecond->getLaneCount(lightPosts[setIter->second].direction); ++laneI)
                if (streetSecond->getLaneFlags(laneI * lightPosts[setIter->second].direction) & Street::TURN_THROUGH)
                    phase.push_back(make_pair(setIter->second, laneI));
        phases.push_back(make_pair(PHASE_DURATION_RED_AMBER, phase));
        phase.clear();
        circleTime += PHASE_DURATION_RED_AMBER;

        // Green
        for (unsigned int laneI = 1; laneI <= streetFirst->getLaneCount(lightPosts[setIter->first].direction); ++laneI)
            if (streetFirst->getLaneFlags(laneI * lightPosts[setIter->first].direction) & Street::TURN_THROUGH)
                phase.push_back(make_pair(setIter->first, laneI));
        if (setIter->first != setIter->second)
            for (unsigned int laneI = 1; laneI <= streetSecond->getLaneCount(lightPosts[setIter->second].direction); ++laneI)
                if (streetSecond->getLaneFlags(laneI * lightPosts[setIter->second].direction) & Street::TURN_THROUGH)
                    phase.push_back(make_pair(setIter->second, laneI));
        phases.push_back(make_pair(PHASE_DURATION_GREEN, phase));
        phase.clear();
        circleTime += PHASE_DURATION_GREEN;

        // Amber
        for (unsigned int laneI = 1; laneI <= streetFirst->getLaneCount(lightPosts[setIter->first].direction); ++laneI)
            if (streetFirst->getLaneFlags(laneI * lightPosts[setIter->first].direction) & Street::TURN_THROUGH)
                phase.push_back(make_pair(setIter->first, laneI));
        if (setIter->first != setIter->second)
            for (unsigned int laneI = 1; laneI <= streetSecond->getLaneCount(lightPosts[setIter->second].direction); ++laneI)
                if (streetSecond->getLaneFlags(laneI * lightPosts[setIter->second].direction) & Street::TURN_THROUGH)
                    phase.push_back(make_pair(setIter->second, laneI));
        phases.push_back(make_pair(PHASE_DURATION_AMBER, phase));
        phase.clear();
        circleTime += PHASE_DURATION_AMBER;

        // All red
        for (unsigned int laneI = 1; laneI <= streetFirst->getLaneCount(lightPosts[setIter->first].direction); ++laneI)
            if (streetFirst->getLaneFlags(laneI * lightPosts[setIter->first].direction) & Street::TURN_THROUGH)
                phase.push_back(make_pair(setIter->first, laneI));
        if (setIter->first != setIter->second)
            for (unsigned int laneI = 1; laneI <= streetSecond->getLaneCount(lightPosts[setIter->second].direction); ++laneI)
                if (streetSecond->getLaneFlags(laneI * lightPosts[setIter->second].direction) & Street::TURN_THROUGH)
                    phase.push_back(make_pair(setIter->second, laneI));
        phases.push_back(make_pair(PHASE_DURATION_ALL_RED * min(radius / CROSSROAD_RADIUS, 1.0), phase));
        phase.clear();
        circleTime += PHASE_DURATION_ALL_RED * min(radius / CROSSROAD_RADIUS, 1.0);

        // If half of the pairs have been added, add the right-only round
        if (!rightOnlyPhase.empty() && phases.size() / 4 == pairs.size()) {
            phases.push_back(make_pair(PHASE_DURATION_GREEN, rightOnlyPhase));
            circleTime += PHASE_DURATION_GREEN;
            phases.push_back(make_pair(PHASE_DURATION_ALL_RED * min(radius / CROSSROAD_RADIUS, 1.0), rightOnlyPhase));
            circleTime += PHASE_DURATION_ALL_RED * min(radius / CROSSROAD_RADIUS, 1.0);
            rightOnlyPhase.clear();
        }
    }

    // Begin with the first phase
    applyPhase();
}

NodeLogicTrafficLight::NodeLogicTrafficLight(const RoadSystem *roadSystem, const Node *node, const double radius)
    : NodeLogic(TRAFFIC_LIGHT), roadSystem(roadSystem), center(node->getPosition()), radius(radius), trafficLights() {

    trafficLights.insert(node->getId());

    // Add all streets of the new node
    const vector<ID>& streets = node->getStreetIds();
    for (unsigned int i = 0; i < streets.size(); ++i)
        createLightPost(node, roadSystem->getStreet(streets[i]));

    // Recalculate the light phases
    rebuildLightPhases();
}

void NodeLogicTrafficLight::tick() {

    // Switch the lights
    timeInPhase += timer.getDelta().total_milliseconds();
    timeInPhase %= circleTime;

    while (timeInPhase > phases[currentPhase].first) {

        timeInPhase -= phases[currentPhase].first;
        currentPhase = (currentPhase + 1) % phases.size();
        applyPhase();
    }

}

int NodeLogicTrafficLight::canEnter(const Node* node, const ID streetId, const int lane, const ID, const int) const {

    int direction = (lane < 0) ? -1 : 1;

    // Search the matching light post
    for (size_t postI = 0; postI < lightPosts.size(); ++postI) {
        if (lightPosts[postI].node == node->getId() && lightPosts[postI].street == streetId && lightPosts[postI].direction == direction) {
            if (lane * direction <= (int)lightPosts[postI].laneStates.size()) {
                if (lightPosts[postI].laneStates[direction * lane - 1] == GREEN)
                    return 0;
                else
                    return (trafficLights.size() > 1 ? 1 : CROSSROAD_RADIUS);
            }
        }
    }

    // Not found, allow passing
    // Note that this is not necessarily a bug
    return 0;
}

NodeLogic* NodeLogicTrafficLight::makeNodeLogic(const RoadSystem *roadSystem, const ID nodeId) {

    // The given node is a traffic light.
    // Search for other traffic lights that are near. If one is found,
    // add this traffic light to the logic of the other one.
    // If none is found, create a new logic and return it.

    Node *nodePtr = roadSystem->getNode(nodeId);

    // Set maximal search distance based on street types and lane counts

    // The maximum type of the connected streets
    int maxType = 1;
    int minType = numeric_limits<int>::max();
    int maxLanes = 1;
    for (vector<ID>::const_iterator iter = nodePtr->getStreetIds().begin(); iter != nodePtr->getStreetIds().end(); ++iter) {
        Street *street = roadSystem->getStreet(*iter);
        int streetType = street->getType();
        if (streetType > maxType) {
            maxType = streetType;
        }
        if (streetType < minType) {
            minType = streetType;
        }
        int lanes = street->getLaneCount(1) + street->getLaneCount(-1);
        if (maxLanes < lanes)
            maxLanes = lanes;
    }

    double maxDistance = minType * ((double)maxLanes / 3);

    double minDistance = maxDistance;
    NodeLogicTrafficLight *minTrafficLight = NULL;

    // Search for other traffic lights
    for (set<NodeLogic*>::const_iterator logicIter = roadSystem->getNodeLogics()->begin(); logicIter != roadSystem->getNodeLogics()->end(); ++logicIter) {

        if ((*logicIter)->getType() == NodeLogic::TRAFFIC_LIGHT) {
            // It is a traffic light: Check for distance
            // If it is small enough, add this node to it
            double distance = calcDistance(nodePtr->getPosition(), (*logicIter)->getPosition());
            if (distance < minDistance + static_cast<NodeLogicTrafficLight*>(*logicIter)->radius) {
                minDistance = distance - static_cast<NodeLogicTrafficLight*>(*logicIter)->radius;
                minTrafficLight = static_cast<NodeLogicTrafficLight*>(*logicIter);
            }
        }
    }

    if (minTrafficLight != NULL) {
        // If a logic has been found, add this node to the found one
        minTrafficLight->addNode(nodePtr);
    } else {
        // If no crossroad has been found, create one
        minTrafficLight = new NodeLogicTrafficLight(roadSystem, nodePtr, maxDistance);
    }

    // Set the logic-pointer of the given node
    nodePtr->setNodeLogic(minTrafficLight);

    // Return the pointer to the crossroad, either found or created
    return minTrafficLight;
}

Vec2f NodeLogicTrafficLight::getPosition() const {
    return center;
}

void NodeLogicTrafficLight::addStreet(const Node* node, const Street* street) {

    if (createLightPost(node, street))
        rebuildLightPhases();

}

void NodeLogicTrafficLight::removeStreet(const Node* node, const Street* street) {

    if (removeLightPost(node, street))
        rebuildLightPhases();
}

string NodeLogicTrafficLight::toString(const bool extendedOutput) const {

    string str = string("NodeLogicTrafficLight")  + ((extendedOutput)?"\n  ":" [")
        + "pos=" + lexical_cast<string>(center[0]) + " / " + lexical_cast<string>(center[1]) + ((extendedOutput)?"\n  ":"; ")
        + "radius=" + lexical_cast<string>(radius) + ((extendedOutput)?"\n  ":"; ")
        + "#nodes=" + lexical_cast<string>(trafficLights.size()) + ((extendedOutput)?"\n    ":"; ");

    for (set<ID>::iterator iter = trafficLights.begin(); iter != trafficLights.end(); ++iter)
        str += lexical_cast<string>(*iter) + ((iter != --trafficLights.end())?"\n    ":"\n  ");

    str += "#lightPosts=" + lexical_cast<string>(lightPosts.size()) + ((extendedOutput)?"\n  ":"; ");
    for (size_t postI = 0; postI < lightPosts.size() && extendedOutput; ++postI) {
        str += "  " + lexical_cast<string>(postI) + ": To " + lexical_cast<string>(lightPosts[postI].facing) + ", state=";
        for (size_t laneI = 0; laneI < lightPosts[postI].laneStates.size() && extendedOutput; ++laneI)
            str += lexical_cast<string>(lightPosts[postI].laneStates[laneI]) + ";";
        str += "\n  ";
    }

    str += "#phases=" + lexical_cast<string>(phases.size()) + ((extendedOutput)?"\n  ":"; ");
    if (extendedOutput)
        for (size_t phaseI = 0; phaseI < phases.size(); ++phaseI) {
            str += ((phaseI == currentPhase)?" => ":"    ");
            if (phases[phaseI].first == PHASE_DURATION_ALL_RED * min(radius / CROSSROAD_RADIUS, 1.0))
                str += "aRed(";
            else if (phases[phaseI].first == PHASE_DURATION_AMBER)
                str += "Amber(";
            else if (phases[phaseI].first == PHASE_DURATION_GREEN)
                str += "Green(";
            else if (phases[phaseI].first == PHASE_DURATION_RED_AMBER)
                str += "RedAmber(";
            else
                str += "\?\?\?(";
            str += lexical_cast<string>(phases[phaseI].first / 1000) + "): ";
            for (vector< pair<size_t, size_t> >::const_iterator iter = phases[phaseI].second.begin(); iter != phases[phaseI].second.end(); ++iter) {
                str += lexical_cast<string>(iter->first) + "/" + lexical_cast<string>(iter->second) + ((iter != --phases[phaseI].second.end())?"; ":"\n  ");
            }
        }


    str += ((extendedOutput)?"":"]");

    return str;
}

const set<ID>& NodeLogicTrafficLight::getTrafficLights() const {
    return trafficLights;
}

double NodeLogicTrafficLight::getRadius() const {
    return radius;
}

vector<NodeLogicTrafficLight::LightPost> NodeLogicTrafficLight::getLightPostState(const ID node) const {

    vector<LightPost> result;

    // Search for the post
    for (size_t postI = 0; postI < lightPosts.size(); ++postI) {
        if (lightPosts[postI].node == node) {
            // Found one! Add it to the vector
            result.push_back(lightPosts[postI]);
        }
    }

    return result;
}
