#include "Routing.h"

set<ID> getConnectedNodes(const RoadSystem *roadSystem, const ID startNode, const double maxDistance) {

    // This method uses a modified version of a breadth-first-search

    // A set of nodes that are near the start node
    set<ID> resultNodes;
    resultNodes.insert(startNode);

    // A "queue" with the nodes that still need to be visited
    map<ID, double> nodesToVisit;
    nodesToVisit.insert(make_pair(startNode, 0));

    // If the distance is 0 return an empty list
    if (maxDistance <= 0)
        return resultNodes;

    while (!nodesToVisit.empty()) {

        // Find the node with smallest distance
        ID nodeId;
        double minDistance = numeric_limits<double>::max();
        for (map<ID, double>::iterator iter = nodesToVisit.begin(); iter != nodesToVisit.end(); ++iter) {
            if (iter->second < minDistance) {
                minDistance = iter->second;
                nodeId = iter->first;
            }
        }
        // Remove the node from the queue
        nodesToVisit.erase(nodeId);

        // If we reached the nodes with a distance bigger than maxDistance, return the result set
        // since there will be no further nodes that are nearer to the startNode.
        if (minDistance > maxDistance)
            return resultNodes;

        // Add the node to the result set
        resultNodes.insert(nodeId);

        // Get a pointer to the node object
        Node *currentNode = roadSystem->getNode(nodeId);

        // Add the neighbor-nodes from all streets going through this node
        const vector<ID> streets = currentNode->getStreetIds();
        for (size_t s = 0; s < streets.size(); ++s) {
            // Iterate through the nodes of the street until our node is found
            int nodeI = -1;
            const vector<ID> *nodes = roadSystem->getStreet(streets[s])->getNodeIds();
            for (size_t i = 0; i < nodes->size(); ++i) {
                if ((*nodes)[i] == nodeId) {
                    nodeI = i;
                    break;
                }
            }
            if (nodeI < 0) {
                // WHAT???
                cerr << "ERROR! Node " << nodeId << " has street " << streets[s] << " registered as street but is no part of it.\n";
                // Next street
                continue;
            }

            // Add the neighbors of our node to the to-visit queue
            if (nodeI > 0) {
                double dist = minDistance + calcDistance(roadSystem->getNode(nodeId)->getPosition(), roadSystem->getNode((*nodes)[nodeI - 1])->getPosition());
                map<ID, double>::iterator iter = nodesToVisit.find((*nodes)[nodeI - 1]);
                // If it does not exist yet, add it. Otherwise reduce the distance if necessary
                if (iter == nodesToVisit.end())
                    nodesToVisit.insert(make_pair((*nodes)[nodeI - 1], dist));
                else if (iter->second > dist)
                    iter->second = dist;
            }
            if (nodeI < (int)(*nodes).size() - 1) {
                double dist = minDistance + calcDistance(roadSystem->getNode(nodeId)->getPosition(), roadSystem->getNode((*nodes)[nodeI + 1])->getPosition());
                map<ID, double>::iterator iter = nodesToVisit.find((*nodes)[nodeI + 1]);
                // If it does not exist yet, add it. Otherwise reduce the distance if necessary
                if (iter == nodesToVisit.end())
                    nodesToVisit.insert(make_pair((*nodes)[nodeI + 1], dist));
                else if (iter->second > dist)
                    iter->second = dist;
            }
        }
    }

    return resultNodes;
}

vector<ID> calculateRoute(const RoadSystem *roadSystem, const ID from, const ID start, const ID end) {

    /// Use an adapted A* algorithm to find the shortest way


    // The output-route
    vector<ID> route;

    if (start == end) {
        route.push_back(start);
        return route;
    }

    // The actual length to reach each node from the start node
    map<ID, double> g;
    // The predecessor for each node
    map<ID, ID> predecessors;
    // The nodes that still have to be checked and their estimated route-lengths
    map<ID, double> openlist;
    // The nodes that are completely checked
    set<ID> closedlist;

    openlist.insert(make_pair(start, 0));
    predecessors[start] = from;

    // The position of the end node
    Vec2f endPosition = roadSystem->getNode(end)->getPosition();

    do {
        ID currentNode;
        map<ID, double>::iterator minIter = openlist.begin();
        // Find the node with the smallest distance
        for (map<ID, double>::iterator iter = ++openlist.begin(); iter != openlist.end(); ++iter) {
            if (iter->second < minIter->second) {
                minIter = iter;
            }
        }

        currentNode = minIter->first;
        openlist.erase(minIter);
        if (currentNode == end) {
            break;
        }

        closedlist.insert(currentNode);

        // Add all successors

        const Node *currentNodePtr = roadSystem->getNode(currentNode);


        vector< pair<ID, ID> > nextNodes = currentNodePtr->getNextNodes(predecessors[currentNode]);

        for (size_t i = 0; i < nextNodes.size(); ++i) {

            ID neighbor = nextNodes[i].first;
            const Street *street = roadSystem->getStreet(nextNodes[i].second);
            if (closedlist.count(neighbor) == 0) {
                // Not checked yet, work on this node

                double streetDistance = street->getNodeDistance(currentNode, neighbor, false);
                double streetLength   = street->getLength();
                double streetCost     = street->getRoutingCost();

                double newG = g[neighbor] + (streetDistance / streetLength) * streetCost;

                // Search for the node in the openlist
                map<ID, double>::iterator iter = openlist.find(neighbor);
                if (iter == openlist.end() || newG < g[neighbor]) {
                    // Save new predecessor
                    predecessors[neighbor] = currentNode;
                    // Save new distance
                    g[neighbor] = newG;

                    // Update estimated route-distance through neighbor
                    double f = newG + calcDistance(roadSystem->getNode(neighbor)->getPosition(), endPosition);
                    if (iter != openlist.end()) {
                        openlist[neighbor] = f;
                    } else {
                        openlist.insert(make_pair(neighbor, f));
                    }
                }
            }
        }

    } while (!openlist.empty());


    // If the end-node has no predecessor, there is no path
    if (predecessors.count(end) == 0)
        return route;

    // Otherwise backtrack through the predecessor-list and assemble the route
    vector<ID> inverseRoute;
    ID node = end;
    // Go back until no predecessor can be found: start is reached
    while(predecessors.count(node) > 0 && node != start) {
        inverseRoute.push_back(node);
        node = predecessors[node];
    }
    // Invert the list
    route.push_back(start);
    for (size_t i = inverseRoute.size(); i > 0; --i) {
        route.push_back(inverseRoute[i - 1]);
    }

    return route;
}

ID getRandomDestinationNode(const RoadSystem *roadSystem, const ID from, const ID start, const double maxDistance ) {

    vector< pair<ID, ID> > neighborNodes = roadSystem->getNode(start)->getNextNodes(from);

    // No more neighbors? Bad luck.
    if (neighborNodes.empty())
        return start;

    // The node that has been traveled through before
    ID lastNode = from;
    // The node we are currently at
    ID currentNode = start;

    // Select a random neighbor-node to go to next. Start with the nodes in front
    // and increase the angle if none are found
    double allowedAngle = 45; // 45° in both directions
    double currentAngle = calcAngle(roadSystem->getNode(currentNode)->getPosition() - roadSystem->getNode(lastNode)->getPosition());

    for (; allowedAngle <= (180 + 30); allowedAngle += 30) {

        size_t rNode = rand() % neighborNodes.size();
        size_t neighborI = rNode;
        do {
            // Calculate the angle to the neighbor node. If it is near our current angle, accept the node
            double angle = calcAngle(roadSystem->getNode(neighborNodes[neighborI].first)->getPosition() - roadSystem->getNode(currentNode)->getPosition());
            double deltaAngle = angle - currentAngle;

            if (deltaAngle < allowedAngle || deltaAngle > 360 - allowedAngle) {

            // Exited the loop per break => Found a node
            lastNode    = currentNode;
            currentNode = neighborNodes[neighborI].first;
                allowedAngle = 360; // break angle-loop
                break; // node-loop
            }

            neighborI = (neighborI + 1) % neighborNodes.size();
        } while (neighborI != rNode);
    }

    if (currentNode == start) {
        // Nowhere to go...
        // Should not happen since there IS a possible neighbor node that should be used
        return start;
    }


    // === After this point, the "start" node means the node that has been found === //

    // Run through the road system until there is no further street that leads away from start or the distance is reached
    const Vec2f startPosition = roadSystem->getNode(start)->getPosition();
    allowedAngle = 45; // now constant
    double currentDistance = calcDistance(startPosition, roadSystem->getNode(currentNode)->getPosition());

    while (currentDistance < maxDistance) {

        // Get the neighbors of the current node
        neighborNodes = roadSystem->getNode(currentNode)->getNextNodes(lastNode);

        if (neighborNodes.empty()) {
            // Nowhere to go? Okay, work done.
            return currentNode;
        }

        Vec2f currentPosition = roadSystem->getNode(currentNode)->getPosition();

        // Search for a matching node
        size_t rNode = rand() % neighborNodes.size();
        size_t neighborI = rNode;
        double distance;
        bool found = false;
        do {

            // Calculate the angle to the neighbor node
            Vec2f neighborPosition = roadSystem->getNode(neighborNodes[neighborI].first)->getPosition();
            double deltaAngle = calcAngle(neighborPosition - currentPosition) - currentAngle;

            if (deltaAngle > allowedAngle && deltaAngle < 360 - allowedAngle) {
                // Angle does not match, look at next node

                neighborI = (neighborI + 1) % neighborNodes.size();
                continue;
            }

            distance = calcDistance(neighborPosition, startPosition);
            if (distance > currentDistance) {

                found = true;
                break;
            }

            neighborI = (neighborI + 1) % neighborNodes.size();
        } while (neighborI != rNode);

        if (!found) {
            // No further node found, return the current node
            return currentNode;
        }

        // Reset the last and current node
        currentDistance = distance;
        lastNode = currentNode;
        currentNode = neighborNodes[neighborI].first;
    }

    // Distance reached, return current node
    return currentNode;
}

double getNearestVehicleDistance(const RoadSystem *roadSystem, const Vec2f& position, const Street *street, const double maxDistance) {

    double minimalDistance = maxDistance;

    // Search vehicles on current street
    for (int streetDirection = -1; streetDirection < 2; streetDirection += 2) {
        for (unsigned int lane = 1; lane <= street->getLaneCount(streetDirection); ++lane) {

            const set<ID> *vehicles = street->getLaneVehicles(streetDirection * lane);

            for (set<ID>::const_iterator vehicleIter = vehicles->begin(); vehicleIter != vehicles->end(); ++vehicleIter) {

                Vehicle *otherVehicle = roadSystem->getVehicle(*vehicleIter);

                double distance = calcDistance(otherVehicle->getPosition(), position);

                if (distance < minimalDistance)
                    minimalDistance = distance;
            } // for(vehicles)
        } // for(lanes)
    } // for(directions)


    // Search vehicles on connected streets
    for (size_t index = 0; index < street->getNodeIds()->size(); ++index) {

        const Node *node = roadSystem->getNode(street->getNodeIds()->operator[](index));

        // Ignore if too far
        // Use the doubled distance since the street might loop back
        if (calcDistance(position, node->getPosition()) > maxDistance * 2)
            continue;

        // Get all streets going through this node
        const vector<ID>& streets = node->getStreetIds();

        for (size_t i = 0; i < streets.size(); ++i) {

            // Do not check current street
            if (streets[i] == street->getId())
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

                        double distance = calcDistance(otherVehicle->getPosition(), position);

                        if (distance < minimalDistance)
                            minimalDistance = distance;

                    } // for(vehicles)
                } // for(lanes)
            } // for(directions)
        } // for(streets)
    } // for(nodes)

    return minimalDistance;
}
