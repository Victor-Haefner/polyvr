#include "VRPathFinding.h"
#include "core/math/graph.h"

#include <OpenSG/OSGVector.h>
#include <map>

using namespace OSG;

VRPathFinding::Position::Position(int nID) : nID(nID) {}
VRPathFinding::Position::Position(int eID, float t) : eID(eID), t(t) {}


VRPathFinding::VRPathFinding() {}

float VRPathFinding::getDistance(int node1, int node2) {
    auto n1 = graph->getNode(node1);
    auto n2 = graph->getNode(node2);
    return (n2.p.pos() - n1.p.pos()).length(); // ? C++
}


vector<VRPathFinding::Position> VRPathFinding::reconstructBestPath(int current){
     vector<Position> bestRoute( { Position(current) } );
     auto node = graph->getNode(current);
     while( cameFrom.count(current) ) {
        current = cameFrom[current];
        bestRoute.push_back(current);
     }
     return bestRoute;
}


int VRPathFinding::getMinFromOpenSet() {
    int res = 0;
    float fMin = 1e9;
    for (auto n : openSet) {
        float f = fCost[n.first];
        if (f < fMin) {
            fMin = f;
            res = n.first;
        }
    }
    return res;
}

vector<VRPathFinding::Position> VRPathFinding::calcBestPath(int start, int goal) {
    vector<Position> route;
    while (!openSet.empty()) {
        int current = getMinFromOpenSet();
        if (current == goal) return reconstructBestPath(current);

        openSet.erase(current);
        closedSet[current] = 1;

        for (auto edge : graph->getEdges()[current]) {
            int neighbor = edge.to;
            if (closedSet.count(neighbor)) continue;
            gCost[current] = getDistance(start, current);
            float tentative_gCost = gCost[current] + getDistance(current, neighbor);

            if (!openSet.count(neighbor)) openSet[neighbor] = 1;
            else if (tentative_gCost >= gCost[neighbor]) continue;

            cameFrom[neighbor] = current;
            gCost[neighbor] = tentative_gCost;
            fCost[neighbor] = gCost[neighbor] + getDistance(neighbor, goal); //heuristic_cost_estimate(node, end)
        }
        cout << "Error openSet is empty" << endl;
    }
    return route;
}

