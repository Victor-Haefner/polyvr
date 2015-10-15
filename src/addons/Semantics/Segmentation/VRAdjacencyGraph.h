#ifndef VRADJACENCYGRAPH_H_INCLUDED
#define VRADJACENCYGRAPH_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>
#include <vector>
#include <map>
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRAdjacencyGraph {
    public:
        struct edge { int v1, v2; };
        struct triangle { int v1, v2, v3; };

        map<int, map<int, vector<triangle> > > edge_triangle_loockup;
        vector<int> vertex_neighbor_params;
        vector<int> vertex_neighbors;
        vector<float> vertex_curvatures;

    private:
        VRGeometryWeakPtr geo;

    public:
        VRAdjacencyGraph();

        static shared_ptr<VRAdjacencyGraph> create();
        void clear();

        void setGeometry(VRGeometryPtr geo);

        void compNeighbors();
        void compTriLoockup();
        void compCurvatures(int range = 1);

        vector<int> getNeighbors(int i, int range = 1);
        vector<int> getBorderVertices();
        float getCurvature(int i);
};

OSG_END_NAMESPACE;

#endif // VRADJACENCYGRAPH_H_INCLUDED
