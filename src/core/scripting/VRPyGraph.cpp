#include "VRPyGraph.h"
#include "VRPyBaseT.h"
#include "VRPyTypeCaster.h"
#include "VRPyTransform.h"
#include "VRPyPose.h"

using namespace OSG;

simplePyType( Graph , New_ptr );

template<> PyObject* VRPyTypeCaster::cast(const Graph::edge& e) {
    PyObject* epy = PyTuple_New(2);
    PyTuple_SetItem(epy, 0, PyLong_FromLong(e.from));
    PyTuple_SetItem(epy, 1, PyLong_FromLong(e.to));
    return epy;
}

template<> PyObject* VRPyTypeCaster::cast(const Graph::node& n) {
    return PyLong_FromLong(n.ID);
}

PyMethodDef VRPyGraph::methods[] = {
    {"getEdge", PyWrap2(Graph, getEdgeCopy, "Return the graph edge between n1 and n2", Graph::edge, int, int ) },
    {"getEdgeID", PyWrap2(Graph, getEdgeID, "Return the graph edge ID between n1 and n2", int, int, int ) },
    {"getEdgeByID", PyWrap2(Graph, getEdgeCopyByID, "Return the graph edge by ID", Graph::edge, int ) },
    {"getEdges", PyWrap2(Graph, getEdgesCopy, "Return graph edges", vector<Graph::edge> ) },
    {"getNEdges", PyWrap2(Graph, getNEdges, "Return number of edges", int ) },
    {"getInEdges", PyWrap2(Graph, getInEdges, "Return graph edges going in from node n", vector<Graph::edge>, int ) },
    {"getOutEdges", PyWrap2(Graph, getOutEdges, "Return graph edges going out from node n", vector<Graph::edge>, int ) },
    {"getNodePose", PyWrap2(Graph, getPosition, "Return graph node pose", PosePtr, int ) },
    {"setNodePose", PyWrap2(Graph, setPosition, "Set graph node pose", void, int, PosePtr ) },
    {"addNode", PyWrapOpt2(Graph, addNode, "Add a node at pose p with optional boundingbox, returns node ID", "0", int, PosePtr, BoundingboxPtr ) },
    {"connect", PyWrapOpt2(Graph, connect, "Connect nodes n1 and n2, returns edge ID", "0", int, int, int, int ) },
    {"connected", PyWrap2(Graph, connected, "Check if n1 and n2 are connected (directed)", bool, int, int ) },
    {"disconnect", PyWrap2(Graph, disconnect, "Disconnect nodes n1 and n2", void, int, int ) },
    {"getNodes", PyWrap2( Graph, getNodesCopy, "Get all node IDs", vector<Graph::node> ) },
    {"getRootNodes", PyWrap2( Graph, getRootNodes, "Get all nodes without incoming edges", vector<Graph::node> ) },
    {"hasNode", PyWrap2( Graph, hasNode, "Check if graph has node ID", bool, int ) },
    {"hasEdge", PyWrap2( Graph, hasEdge, "Check if graph has edge ID", bool, int ) },
    {"remNode", PyWrap2( Graph, remNode, "Remove node", void, int ) },
    {"clear", PyWrap2( Graph, clear, "Clear", void ) },
    {"getNodeWeight", PyWrap2( Graph, getNodeWeight, "Get node weight", float, int ) },
    {"getEdgeWeight", PyWrap2( Graph, getEdgeWeight, "Get edge weight", float, int ) },
    {"setNodeWeight", PyWrap2( Graph, setNodeWeight, "Set node weight", void, int, float ) },
    {"setEdgeWeight", PyWrap2( Graph, setEdgeWeight, "Set edge weight", void, int, float ) },
    {NULL} /* Sentinel */
};





