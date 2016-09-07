#ifndef VRGRAPHLAYOUT_H_INCLUDED
#define VRGRAPHLAYOUT_H_INCLUDED

#include "core/math/graph.h"
#include "core/utils/VRFwdDeclTemplate.h"

#include <OpenSG/OSGVector.h>
#include <map>

OSG_BEGIN_NAMESPACE;
using namespace std;

ptrFwd(VRGraphLayout);

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
        graph_basePtr graph;
        map<int, ALGORITHM> algorithms;
        map<int, FLAG> flags;
        Vec3f gravity;
        float radius = 1;

        void applySprings(float eps);
        void applyOccupancy(float eps);

        FLAG getFlag(int i);

    public:
        VRGraphLayout();
        static VRGraphLayoutPtr create();

        void setGraph(graph_basePtr g);
        graph_basePtr getGraph();

        void setAlgorithm(ALGORITHM a, int position = 0);
        void setAlgorithm(string a, int position = 0);
        void clearAlgorithms();
        void compute(int N = 10, float eps = 0.1);

        void setGravity(Vec3f v);
        void setRadius(float r);
        void fixNode(int i);
};

OSG_END_NAMESPACE;

#endif // VRGRAPHLAYOUT_H_INCLUDED
