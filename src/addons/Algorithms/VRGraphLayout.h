#ifndef VRGRAPHLAYOUT_H_INCLUDED
#define VRGRAPHLAYOUT_H_INCLUDED

#include "VRAlgorithmsFwd.h"
#include "core/math/graph.h"

#include <OpenSG/OSGVector.h>
#include <map>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRGraphLayout {
    public:
        enum ALGORITHM {
            SPRINGS,
            OCCUPANCYMAP
        };

        enum FLAG {
            NONE=1,
            FIXED=2,
            INACTIVE=4
        };

    private:
        GraphPtr graph;
        map<int, ALGORITHM> algorithms;
        map<int, int> flags;
        Vec3f gravity;
        float radius = 1;
        float speed = 1;

        void applySprings(float eps, float v);
        void applyOccupancy(float eps, float v);

        int getFlag(int i);
        void setFlag(int i, FLAG f);
        void remFlag(int i, FLAG f);
        bool isFlag(int i, FLAG f);

    public:
        VRGraphLayout();
        static VRGraphLayoutPtr create();

        void clear();
        void setGraph(GraphPtr g);
        GraphPtr getGraph();

        void setAlgorithm(ALGORITHM a, int position = 0);
        void setAlgorithm(string a, int position = 0);
        void clearAlgorithms();
        void compute(int N = 10, float eps = 0.1);

        void setGravity(Vec3f v);
        void setRadius(float r);
        void setSpeed(float s);
        void fixNode(int i);
        void setNodeState(int i, bool state);
};

OSG_END_NAMESPACE;

#endif // VRGRAPHLAYOUT_H_INCLUDED
