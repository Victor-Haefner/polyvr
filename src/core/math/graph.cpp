#include "graph.h"

struct graph::edge {
    int n1, n2;

    edge(int i, int j) : n1(i), n2(j) {}
};

struct graph::node {
    node() {}
};

graph::graph() {}
graph::~graph() {}

int graph::addNode() { nodes.push_back(node()); return nodes.size()-1; }
int graph::connect(int i, int j) { edges.push_back(edge(i,j)); return edges.size()-1; }
