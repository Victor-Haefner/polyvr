#ifndef VRPATHFINDING_H_INCLUDED
#define VRPATHFINDING_H_INCLUDED


#include "VRAlgorithmsFwd.h"
#include "core/math/VRMathFwd.h"

#include "core/math/OSGMathFwd.h"
#include <OpenSG/OSGConfig.h>
#include <vector>
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

            string toString();
        };

    private:
        GraphPtr graph;
        vector<PathPtr> paths;

        map<Position, int> closedSet; // set of nodes already evaluated
        map<Position, int> openSet; // set of currently discovered nodes that are not evaluated yet
        map<Position, Position> cameFrom; // key = current step, value=the most efficient previous step
        map<Position, float> fCost; //key = node, value = estimated cost from the start to end


        Vec3d pos(Position& p);
        vector<Position> getNeighbors(Position& p, bool bidirectional);

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
        void setPaths(vector<PathPtr> p);
        vector<Position> computePath(Position start, Position goal, bool bidirectional = false);
};

OSG_END_NAMESPACE

#endif // VRPATHFINDING_H_INCLUDED
