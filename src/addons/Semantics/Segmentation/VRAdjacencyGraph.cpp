#include "VRAdjacencyGraph.h"
#include "core/objects/geometry/VRGeometry.h"

#include <OpenSG/OSGTriangleIterator.h>

using namespace OSG;
using namespace std;

VRAdjacencyGraph::VRAdjacencyGraph() {}

shared_ptr<VRAdjacencyGraph> VRAdjacencyGraph::create() { return shared_ptr<VRAdjacencyGraph>(new VRAdjacencyGraph()); }

void VRAdjacencyGraph::setGeometry(VRGeometry* geo) { this->geo = geo; }

void VRAdjacencyGraph::compute(bool do_neighbors, bool do_tri_loockup) {
    if (geo == 0) return;
    if (do_neighbors) compNeighbors();
    if (do_tri_loockup) compTriLoockup();
}

void VRAdjacencyGraph::compNeighbors() {
    vertex_neighbor_params.clear();
    vertex_neighbors.clear();
    if (geo == 0) return;
    map< int, map<int, int> > tmpdict;

    for (TriangleIterator it = TriangleIterator(geo->getMesh()); !it.isAtEnd(); ++it) {
        int i0 = it.getPositionIndex(0);
        int i1 = it.getPositionIndex(1);
        int i2 = it.getPositionIndex(2);

        auto reg = [&](int j0, int j1, int j2) {
            if ( tmpdict.count(j0) == 0 ) tmpdict[j0] = map<int, int>();
            if ( tmpdict[j0].count(j1) == 0 ) tmpdict[j0][j1] = 0;
            if ( tmpdict[j0].count(j2) == 0 ) tmpdict[j0][j2] = 0;
            tmpdict[j0][j1] += 1;
            tmpdict[j0][j2] += 1;
        };

        reg(i0,i1,i2);
        reg(i1,i0,i2);
        reg(i2,i0,i1);
    }

    int N = geo->getMesh()->getPositions()->size();
    vertex_neighbor_params.resize(2*N);
    for (auto i : tmpdict) vertex_neighbor_params[2*i.first+1] = i.second.size();

    int kN = 0;
    for (int i = 0; i < N; i++) {
        vertex_neighbor_params[2*i] = kN;
        kN += vertex_neighbor_params[2*i+1];
    }

    vertex_neighbors.resize(kN);
    for (auto i : tmpdict) {
        int o = vertex_neighbor_params[2*i.first];
        int k = 0;
        for (auto j : i.second) { vertex_neighbors[o+k] = j.first; k++; }
    }
}

void VRAdjacencyGraph::compTriLoockup() {
    edge_triangle_loockup.clear();
    if (geo == 0) return;

    for (TriangleIterator it = TriangleIterator(geo->getMesh()); !it.isAtEnd(); ++it) {
        triangle t;
        t.v1 = it.getPositionIndex(0);
        t.v2 = it.getPositionIndex(1);
        t.v3 = it.getPositionIndex(2);

        auto reg = [&](int j0, int j1) {
            if (edge_triangle_loockup.count(j0) == 0) edge_triangle_loockup[j0] = map<int, vector<triangle> >();
            if (edge_triangle_loockup[j0].count(j1) == 0) edge_triangle_loockup[j0][j1] = vector<triangle>();
            edge_triangle_loockup[j0][j1].push_back(t);
        };

        reg(t.v1,t.v2); reg(t.v2,t.v1);
        reg(t.v1,t.v3); reg(t.v3,t.v1);
        reg(t.v2,t.v3); reg(t.v3,t.v2);
    }
}

vector<int> VRAdjacencyGraph::getNeighbors(int i) {
    if (geo == 0) return vector<int>();
    if (2*i+1 >= vertex_neighbor_params.size()) return vector<int>();

    int o = vertex_neighbor_params[2*i];
    int nN = vertex_neighbor_params[2*i+1];
    vector<int> res(nN,0);
    for (int j=0; j<nN; j++) res[j] = vertex_neighbors[o+j];
    return res;
}


