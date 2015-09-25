#ifndef VRADJACENCYGRAPH_H_INCLUDED
#define VRADJACENCYGRAPH_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>
#include <vector>
#include <map>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRObject;
class VRGeometry;

class VRAdjacencyGraph {
    public:
        struct edge {
            int v1, v2;
        };

        struct triangle {
            int v1, v2, v3;
        };

        map<int, map<int, vector<triangle> > > edge_triangle_loockup;
        vector<int> vertex_neighbor_params;
        vector<int> vertex_neighbors;
        vector<float> vertex_curvatures;

    private:
        VRGeometry* geo = 0;

    public:
        VRAdjacencyGraph();

        static shared_ptr<VRAdjacencyGraph> create();

        void setGeometry(VRGeometry* geo);

        void compNeighbors();
        void compTriLoockup();
        void compCurvatures();

        vector<int> getNeighbors(int i);
        vector<int> getBorderVertices();
        float getCurvature(int i);
};

OSG_END_NAMESPACE;

#endif // VRADJACENCYGRAPH_H_INCLUDED
