#include "graph.h"
#include "graphT.h"

using namespace OSG;

graph_base::graph_base() {}
graph_base::~graph_base() {}

void graph_base::connect(int i, int j, CONNECTION c) {
    while (i >= edges.size()) edges.push_back( vector<edge>() );
    edges[i].push_back(edge(i,j,c));
}

vector< vector< graph_base::edge > >& graph_base::getEdges() { return edges; }
vector< graph_base::node >& graph_base::getNodes() { return nodes; }
graph_base::node& graph_base::getNode(int i) { return nodes[i]; }
void graph_base::setPosition(int i, Vec3f v) { nodes[i].pos = v; update(i); }
void graph_base::update(int i) {}

graph_base::edge::edge(int i, int j, CONNECTION c) : from(i), to(j), connection(c) {}

//vector<graph_base::node>::iterator graph_base::begin() { return nodes.begin(); }
//vector<graph_base::node>::iterator graph_base::end() { return nodes.end(); }
