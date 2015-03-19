#ifndef ROUTING_H
#define ROUTING_H

#include "RoadSystem.h"

/**
 Calculates a list of all nodes that are connected to a given node by streets.
 In a normal city, that should return all nodes if no limits are set.
 The startNode will always be in the set.

 @note Unused at the moment

 @param roadSystem The RoadSystem to find nodes in.
 @param startNode The ID of the node to start searching from.
 @param maxDistance The maximal distance to travel over streets while searching for nodes.
 @return A set of node IDs that are connected to the given node.
 */
set<ID> getConnectedNodes(const RoadSystem *roadSystem, const ID startNode, const double maxDistance = numeric_limits<double>::max());

/**
 Calculates a route from Node \c start to Node \c end;
 If \c start is equal to \c end, this will return a vector only containing \c start.
 If there is no route, an empty vector will be returned.
 The \c start && \c end nodes are part of the returned route.
 @param roadSystem The RoadSystem to find a route in.
 @param from The id of the node the vehicle is coming from when arriving at \c start.
 @param start The id of the node to start routing at.
 @param end The id of the node to stop routing at.
 @return A vector containing the nodes visited while going from \c start to \c end.
 */
vector<ID> calculateRoute(const RoadSystem *roadSystem, const ID from, const ID start, const ID end);


/**
 Finds a random node that can be reached from the given start node.

 Follows a random way through the road map until either the \c maxDistance is reached || there is no further
 street to follow without decreasing the distance to the start. The latter case does not mean that there is
 no further way to go, it only means that the vehicle would need to turn to use it.

 The first node after \c start is chosen randomly while preferring streets that are in front of the vehicle.
 However, this can even lead the vehicle back on the same street if no other street is available. After that,
 the started direction is when possible continued.

 Note that this algorithm will happily lead you into a one-way dead-end.

 @param roadSystem The RoadSystem to find a node in.
 @param from The last node that has been visited by the vehicle.
 @param start The next node that will be visited by the vehicle. If the vehicle already has a route,
    this two parameters are the last entries in the existing route.
 @param maxDistance If a node is further away from \c start than this, it will be returned.
 @return The id of a node that can be reached from the current position. If \c start is returned,
    no further node can be reached from the given node. In that case, you are stuck.
 */
ID getRandomDestinationNode(const RoadSystem *roadSystem, const ID from, const ID start, const double maxDistance = 500);

/**
 Returns the next vehicle near the given position.
 This function assumes that there is no vehicle on the given position,
 otherwise the distance will most likely be 0.
 @param roadSystem The RoadSystem to work on.
 @param position The position to search nearby.
 @param street The street the position is at. Is used as basis to run through the street map.
 @param maxDistance The maximal distance to look into.
 @return The distance to the nearest vehicle. If no vehicle has been found, the return value equals \c maxDistance.
 */
double getNearestVehicleDistance(const RoadSystem *roadSystem, const Vec2f& position, const Street *street, const double maxDistance);


#endif // ROUTING_H
