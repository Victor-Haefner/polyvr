#ifndef GRAPHT_H_INCLUDED
#define GRAPHT_H_INCLUDED

#include "graph.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

template<class T> graph<T>::graph() {}
template<class T> graph<T>::~graph() {}

template<class T> int graph<T>::addNode() { nodes.push_back(T()); return nodes.size()-1; }
template<class T> int graph<T>::addNode(T t) { nodes.push_back(t); return nodes.size()-1; }

template<class T> vector<T>& graph<T>::getNodes() { return nodes; }
template<class T> T& graph<T>::getNode(int i) { return nodes[i]; }

#include <OpenSG/OSGVector.h>

template class graph<OSG::Vec3f>;

OSG_END_NAMESPACE;

#endif // GRAPHT_H_INCLUDED
