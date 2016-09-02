#ifndef GRAPHT_H_INCLUDED
#define GRAPHT_H_INCLUDED

#include "graph.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

template<class T> graph<T>::graph() {}
template<class T> graph<T>::~graph() {}

template<class T> shared_ptr< graph<T> > create() { return shared_ptr< graph<T> >(new graph<T>()); }

template<class T> int graph<T>::addNode() { nodes.push_back(node()); elements.push_back(T()); return nodes.size()-1; }
template<class T> int graph<T>::addNode(T t) { nodes.push_back(node()); elements.push_back(t); return nodes.size()-1; }

template<class T> vector<T>& graph<T>::getElements() { return elements; }
template<class T> T& graph<T>::getElement(int i) { return elements[i]; }

template<class T> void graph<T>::update(int i) { elements[i].update( nodes[i] ); }

OSG_END_NAMESPACE;

#endif // GRAPHT_H_INCLUDED
