#ifndef VRPATHFINDING_H_INCLUDED
#define VRPATHFINDING_H_INCLUDED


#include "VRAlgorithmsFwd.h"
#include "core/math/VRMathFwd.h"

#include <OpenSG/OSGVector.h>
#include <map>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRPathFinding {
    public:
        struct Position {
            int nID = -1;
            int eID = -1;
            float t = 0;

            Position(int nID);
            Position(int eID, float t);
        };

    private:
        GraphPtr graph;

        map<int, int> closedSet; // set of nodes already evaluated
        map<int, int> openSet; // set of currently discovered nodes that are not evaluated yet
        map<int, int> cameFrom; // key = current step, value=the most efficient previous step

        map<int, float> gCost; //key = node, value = cost from the start to current node
        map<int, float> fCost; //key = node, value = estimated cost from the start to end
        float hCost; //?

        int getMinFromOpenSet();
        float getDistance(int node1, int node2); // return value?
        float hEstimation();
        vector<Position> reconstructBestPath(int current);


    public:
        VRPathFinding();
        ~VRPathFinding();
        vector<Position> calcBestPath(int start, int goal);

};

OSG_END_NAMESPACE

#endif // VRPATHFINDING_H_INCLUDED
