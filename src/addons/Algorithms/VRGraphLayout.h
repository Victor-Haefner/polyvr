#ifndef VRGRAPHLAYOUT_H_INCLUDED
#define VRGRAPHLAYOUT_H_INCLUDED

#import "core/math/graph.h"
#import <OpenSG/OSGVector.h>
#import <map>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRGraphLayout {
    public:
        enum ALGORITHM {
            SPRINGS,
            OCCUPANCYMAP
        };

        enum FLAG {
            NONE,
            FIXED
        };

    private:
        graph<Vec3f> g;
        map<int, ALGORITHM> algorithms;
        map<int, FLAG> flags;
        Vec3f gravity;
        float radius = 1;

        void applySprings(float eps);
        void applyOccupancy(float eps);

        FLAG getFlag(int i);

    public:
        VRGraphLayout();

        void setGraph(graph<Vec3f>& g);
        graph<Vec3f>& getGraph();

        void setAlgorithm(ALGORITHM a, int position = 0);
        void clearAlgorithms();
        void compute(int N = 10, float eps = 0.1);

        void setGravity(Vec3f v);
        void setRadius(float r);
        void fixNode(int i);
};

OSG_END_NAMESPACE;

#endif // VRGRAPHLAYOUT_H_INCLUDED
