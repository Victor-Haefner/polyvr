#include "graph.h"

#include <algorithm>

using namespace OSG;

Graph::Graph() {}
Graph::~Graph() {}

Graph::edge& Graph::connect(int i, int j, CONNECTION c) {
    //if (i >= int(nodes.size()) || j >= int(nodes.size())) return edge;
    while (i >= int(edges.size())) edges.push_back( vector<edge>() );
    edges[i].push_back(edge(i,j,c));
    return *edges[i].rbegin();
}

void Graph::disconnect(int i, int j) {
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

bool Graph::connected(int i, int j) {
    if (i >= int(nodes.size()) || j >= int(nodes.size())) return false;
    if (i >= int(edges.size())) return false;
    auto& v = edges[i];
    for (uint k=0; k<v.size(); k++) {
        if (v[k].to == j) return true;
    }
    return false;
}

vector< vector< Graph::edge > >& Graph::getEdges() { return edges; }
vector< Graph::node >& Graph::getNodes() { return nodes; }
Graph::node& Graph::getNode(int i) { return nodes[i]; }
int Graph::size() { return nodes.size(); }

int Graph::getNEdges() {
    int N = 0;
    for (auto& n : edges) N += n.size();
    return N;
}

void Graph::setPosition(int i, Vec3f v) {
    auto& n = nodes[i];
    n.box.setCenter(v);
    update(i, true);
}

int Graph::addNode() { nodes.push_back(node()); return nodes.size()-1; }
void Graph::clear() { nodes.clear(); edges.clear(); }
void Graph::update(int i, bool changed) {}
void Graph::remNode(int i) { nodes.erase(nodes.begin() + i); }

Graph::edge::edge(int i, int j, CONNECTION c) : from(i), to(j), connection(c) {}

//vector<Graph::node>::iterator Graph::begin() { return nodes.begin(); }
//vector<Graph::node>::iterator Graph::end() { return nodes.end(); }
