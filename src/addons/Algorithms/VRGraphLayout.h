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
            SPRINGS
        };

        enum FLAG {
            NONE,
            FIXED
        };

    private:
        graph<Vec3f> g;
        ALGORITHM algorithm = SPRINGS;
        map<int, FLAG> flags;
        Vec3f gravity;
        float radius = 1;

        void applySprings(int N, float eps);

        FLAG getFlag(int i);

    public:
        VRGraphLayout();

        void setGraph(graph<Vec3f>& g);
        graph<Vec3f>& getGraph();

        void setAlgorithm(ALGORITHM a);
        void compute(int N = 10, float eps = 0.1);

        void setGravity(Vec3f v);
        void setRadius(float r);
        void fixNode(int i);
};

OSG_END_NAMESPACE;

#endif // VRGRAPHLAYOUT_H_INCLUDED
