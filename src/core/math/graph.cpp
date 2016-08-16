#include "graph.h"

template<class T> graph<T>::edge::edge(int i, int j, CONNECTION c) : from(i), to(j), connection(c) {}

template<class T> graph<T>::graph() {}
template<class T> graph<T>::~graph() {}

template<class T> int graph<T>::addNode() { nodes.push_back(T()); return nodes.size()-1; }
template<class T> int graph<T>::addNode(T t) { nodes.push_back(t); return nodes.size()-1; }

template<class T> void graph<T>::connect(int i, int j, CONNECTION c) {
    while (i >= edges.size()) edges.push_back( vector<edge>() );
    edges[i].push_back(edge(i,j,c));
}

template<class T> vector<T>& graph<T>::getNodes() { return nodes; }
template<class T> vector< vector< typename graph<T>::edge > >& graph<T>::getEdges() { return edges; }
template<class T> T& graph<T>::getNode(int i) { return nodes[i]; }

#include <OpenSG/OSGVector.h>

template class graph<OSG::Vec3f>;
