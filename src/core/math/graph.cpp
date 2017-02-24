#include "graph.h"
#include "graphT.h"

#include <algorithm>

using namespace OSG;

template class graph<graph_base::emptyNode*>;

graph_base::graph_base() {}
graph_base::~graph_base() {}

graph_base::edge& graph_base::connect(int i, int j, CONNECTION c) {
    //if (i >= int(nodes.size()) || j >= int(nodes.size())) return edge;
    while (i >= int(edges.size())) edges.push_back( vector<edge>() );
    edges[i].push_back(edge(i,j,c));
    return *edges[i].rbegin();
}

void graph_base::disconnect(int i, int j) {
    if (i >= int(nodes.size()) || j >= int(nodes.size())) return;
    if (i >= int(edges.size())) return;
    auto& v = edges[i];
    for (uint k=0; k<v.size(); k++) {
        if (v[k].to == j) {
            v.erase(v.begin()+k);
            break;
        }
    }
}

bool graph_base::connected(int i, int j) {
    if (i >= int(nodes.size()) || j >= int(nodes.size())) return false;
    if (i >= int(edges.size())) return false;
    auto& v = edges[i];
    for (uint k=0; k<v.size(); k++) {
        if (v[k].to == j) return true;
    }
    return false;
}

vector< vector< graph_base::edge > >& graph_base::getEdges() { return edges; }
vector< graph_base::node >& graph_base::getNodes() { return nodes; }
graph_base::node& graph_base::getNode(int i) { return nodes[i]; }
void graph_base::update(int i, bool changed) {}
int graph_base::size() { return nodes.size(); }
void graph_base::clear() { nodes.clear(); edges.clear(); }

int graph_base::getNEdges() {
    int N = 0;
    for (auto& n : edges) N += n.size();
    return N;
}

void graph_base::setPosition(int i, Vec3f v) {
    auto& n = nodes[i];
    n.box.setCenter(v);
    update(i, true);
}

int graph_base::addNode() { return 0; }
void graph_base::remNode(int i) {}

graph_base::edge::edge(int i, int j, CONNECTION c) : from(i), to(j), connection(c) {}

//vector<graph_base::node>::iterator graph_base::begin() { return nodes.begin(); }
//vector<graph_base::node>::iterator graph_base::end() { return nodes.end(); }
