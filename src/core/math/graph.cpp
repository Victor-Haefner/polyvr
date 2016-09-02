#include "graph.h"
#include "graphT.h"

using namespace OSG;

graph_base::graph_base() {}
graph_base::~graph_base() {}

graph_basePtr graph_base::create() { return graph_basePtr(new graph_base); }

void graph_base::connect(int i, int j, CONNECTION c) {
    while (i >= edges.size()) edges.push_back( vector<edge>() );
    edges[i].push_back(edge(i,j,c));
}

vector< vector< graph_base::edge > >& graph_base::getEdges() { return edges; }

graph_base::edge::edge(int i, int j, CONNECTION c) : from(i), to(j), connection(c) {}
