#include "graph.h"
#include "core/utils/toString.h"

#include <algorithm>

using namespace OSG;

string toString(Graph::edge& e) {
    return toString(Vec3i(e.from, e.to, e.connection));
}

template<> int toValue(stringstream& ss, Graph::edge& e) {
    Vec3i tmp;
    auto b = toValue(ss, tmp);
    e.from = tmp[0];
    e.to = tmp[1];
    e.connection = Graph::CONNECTION(tmp[2]);
    return b;
}

string toString(Graph::node& n) {
    return toString(n.box) + " " + toString(n.p);
}

template<> int toValue(stringstream& ss, Graph::node& n) {
    bool b = toValue(ss, n.box);
    b = b?toValue(ss, n.p):b;
    return b;
}

Graph::position::position() {}
Graph::position::position(int n) { node = n; }
Graph::position::position(int e, float p) { edge = e; pos = p; }

#include "core/utils/VRStorage_template.h"


Graph::Graph() {
    storeMap("nodes", nodes);
    storeMap("edges", edges);
}

Graph::~Graph() {}

int Graph::connect(int i, int j, int c) {
    if (i < 0 || j < 0) return -1;
    static size_t eID = -1; eID++;
    edges[eID] = edge(i,j,CONNECTION(c),eID);
    if (!hasNode(i)) { cout << "Graph::connect, cannot connect unknown node " << i << endl; }
    if (!hasNode(j)) { cout << "Graph::connect, cannot connect unknown node " << j << endl; }
    getNode(i).outEdges.push_back(eID);
    getNode(j).inEdges.push_back(eID);
    return eID;
}

template<class T>
void erase(vector<T>& v, const T& t) {
    v.erase(remove(v.begin(), v.end(), t), v.end());
}

void Graph::disconnect(int i, int j) {
    int eID = getEdgeID(i,j);
    if (!hasEdge(eID)) return;
    edges.erase(eID);
    erase( getNode(i).outEdges, eID );
    erase( getNode(j).inEdges, eID );
}

vector< Graph::edge > Graph::getInEdges(int i) {
    vector<edge> res;
    for (int e : getNode(i).inEdges) res.push_back(edges[e]);
    return res;
}

vector< Graph::edge > Graph::getOutEdges(int i) {
    vector<edge> res;
    for (int e : getNode(i).outEdges) res.push_back(edges[e]);
    return res;
}

vector<Graph::edge> Graph::getConnectedEdges(node& n) {
    vector<edge> edges;
    for (int eID : n.inEdges) edges.push_back(getEdge(eID));
    for (int eID : n.outEdges) edges.push_back(getEdge(eID));
    return edges;
}

vector<Graph::edge> Graph::getPrevEdges(edge& e) {
    vector<edge> edges;
    auto& n = getNode(e.from);
    for (int eID : n.inEdges) edges.push_back(getEdge(eID));
    return edges;
}

vector<Graph::edge> Graph::getNextEdges(edge& e) {
    vector<edge> edges;
    auto& n = getNode(e.to);
    for (int eID : n.outEdges) edges.push_back(getEdge(eID));
    return edges;
}

bool Graph::connected(int i, int j) {
    if (!hasNode(i) || !hasNode(j)) return false;
    auto n = getNode(i);
    for (auto eID : n.outEdges) if (edges[eID].to == j) return true;
    return false;
}

int Graph::size() { return nodes.size(); }
bool Graph::hasNode(int i) { return nodes.count(i); }
bool Graph::hasEdge(int i) { return edges.count(i); }
map< int, Graph::edge >& Graph::getEdges() { return edges; }
map< int, Graph::node >& Graph::getNodes() { return nodes; }
Graph::node& Graph::getNode(int i) { return nodes[i]; }

Graph::edge& Graph::getEdge(int i) {
    if (!hasEdge(i)) return nullEdge;
    return edges[i];
}

Graph::edge Graph::getEdgeCopyByID(int i) { return getEdge(i); }

Graph::edge& Graph::getEdge(int n1, int n2) {
    if (!hasNode(n1)) return nullEdge;
    for (auto& eID : nodes[n1].outEdges) {
        if (!hasEdge(eID)) continue;
        auto& e = edges[eID];
        if (e.to == n2) return e;
    }
    return nullEdge;
}

Graph::edge Graph::getEdgeCopy(int n1, int n2) { return getEdge(n1, n2); }

vector< Graph::node > Graph::getNodesCopy() {
    vector<node> res;
    for (auto& n : nodes) res.push_back(n.second);
    return res;
}

vector<Graph::edge> Graph::getEdgesCopy() {
    vector<edge> res;
    for (auto& e : edges) res.push_back(e.second);
    return res;
}

vector< Graph::node > Graph::getPreviousNodes(int i) {
    vector<node> res;
    for (auto& e : getInEdges(i)) res.push_back( getNode(e.from) );
    return res;
}

vector< Graph::node > Graph::getNextNodes(int i) {
    vector<node> res;
    for (auto& e : getOutEdges(i)) res.push_back( getNode(e.to) );
    return res;
}

vector< Graph::node > Graph::getNeighbors(int i) {
    vector<node> res;
    for (auto& e : getInEdges(i)) res.push_back( getNode(e.from) );
    for (auto& e : getOutEdges(i)) res.push_back( getNode(e.to) );
    return res;
}

vector< int > Graph::getRelations(int e) { return edges[e].relations; }

void Graph::addRelation(int e1, int e2) {
    if (!hasRelation(e1,e2)) {
        edges[e1].relations.push_back(e2);
        edges[e2].relations.push_back(e1);
    }
}

bool Graph::hasRelation(int e1, int e2) {
    for (auto e : edges[e1].relations) { if (e == e2) return true; }
    return false;
}

float Graph::getEdgeLength(int e) {
    if (!hasEdge(e)) return 0.0;
    float res = ( getPosition(edges[e].to)->pos() - getPosition(edges[e].from)->pos() ).length();
    return res;
}

int Graph::getEdgeID(int n1, int n2) {
    auto edge = getEdge(n1, n2);
    return edge.ID;
}

int Graph::getNEdges() { return edges.size(); }

void Graph::setPosition(int i, PosePtr p) {
    if (!p || !hasNode(i)) return;
    nodes[i].p = *p;
    update(i, true);
}

PosePtr Graph::getPosition(int i) { auto p = Pose::create(); *p = nodes[i].p; return p; }

int Graph::addNode(PosePtr p, BoundingboxPtr b) {
    static size_t nID = -1; nID++;
    nodes[nID] = node();
    nodes[nID].ID = nID;
    nodes[nID].box = b ? *b : Boundingbox();
    if (p) setPosition(nID, p);
    return nID;
}
void Graph::clear() { nodes.clear(); edges.clear(); }
void Graph::update(int i, bool changed) {}

void Graph::remNode(int nID) {
    for (auto& n : getNeighbors(nID)) {
        if (connected(nID,n.ID)) disconnect(nID, n.ID);
        if (connected(n.ID,nID)) disconnect(n.ID, nID);
    }
    nodes.erase(nID);
    //cout << "Graph::remNode " << nID << " deleted" << endl;
}

Graph::edge::edge(int i, int j, CONNECTION c, int ID) : from(i), to(j), connection(c), ID(ID) {}

//vector<Graph::node>::iterator Graph::begin() { return nodes.begin(); }
//vector<Graph::node>::iterator Graph::end() { return nodes.end(); }
