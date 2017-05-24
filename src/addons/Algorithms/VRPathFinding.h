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

            Position(int nID = -1);
            Position(int eID, float t);

            bool operator==(const Position& p);
            bool operator<(const Position& p) const;
        };

    private:
        GraphPtr graph;
        vector<pathPtr> paths;

        map<Position, int> closedSet; // set of nodes already evaluated
        map<Position, int> openSet; // set of currently discovered nodes that are not evaluated yet
        map<Position, Position> cameFrom; // key = current step, value=the most efficient previous step

        map<Position, float> gCost; //key = node, value = cost from the start to current node
        map<Position, float> fCost; //key = node, value = estimated cost from the start to end
        float hCost; //?


        Vec3f pos(Position& p);
        vector<Position> getNeighbors(Position& p);

        bool valid(Position& p);
        Position getMinFromOpenSet();
        float getDistance(Position node1, Position node2); // return value?
        float hEstimation();
        vector<Position> reconstructBestPath(Position current);


    public:
        VRPathFinding();
        ~VRPathFinding();
        static VRPathFindingPtr create();

        void setGraph(GraphPtr g);
        void setPaths(vector<pathPtr> p);
        vector<Position> computePath(Position start, Position goal);
};

OSG_END_NAMESPACE

#endif // VRPATHFINDING_H_INCLUDED
