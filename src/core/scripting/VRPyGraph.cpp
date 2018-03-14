#include "VRPyGraph.h"
#include "VRPyTransform.h"
#include "VRPyBaseT.h"
#include "VRPyPose.h"
#include "VRPyTypeCaster.h"

using namespace OSG;

newPyType( Graph , Graph , New_ptr );

template<> PyObject* VRPyTypeCaster::cast(const Graph::edge& e) {
    PyObject* epy = PyTuple_New(2);
    PyTuple_SetItem(epy, 0, PyInt_FromLong(e.from));
    PyTuple_SetItem(epy, 1, PyInt_FromLong(e.to));
    return epy;
}

template<> PyObject* VRPyTypeCaster::cast(const Graph::node& n) {
    return PyInt_FromLong(n.ID);
}

PyMethodDef VRPyGraph::methods[] = {
    {"getEdge", PyWrap2(Graph, getEdgeCopy, "Return the graph edge between n1 and n2", Graph::edge, int, int ) },
    {"getEdgeID", PyWrap2(Graph, getEdgeID, "Return the graph edge ID between n1 and n2", int, int, int ) },
    {"getEdges", PyWrap2(Graph, getEdgesCopy, "Return graph edges", vector<Graph::edge> ) },
    {"getInEdges", PyWrap2(Graph, getInEdges, "Return graph edges going in from node n", vector<Graph::edge>, int ) },
    {"getOutEdges", PyWrap2(Graph, getOutEdges, "Return graph edges going out from node n", vector<Graph::edge>, int ) },
    {"getNodePose", PyWrap2(Graph, getPosition, "Return graph node pose", PosePtr, int ) },
    {"addNode", PyWrap2(Graph, addNode, "Add a node at pose p, returns node ID", int, PosePtr ) },
    {"connect", PyWrapOpt2(Graph, connect, "Connect nodes n1 and n2, returns edge ID", "0", int, int, int, int ) },
    {"disconnect", PyWrap2(Graph, disconnect, "Disconnect nodes n1 and n2", void, int, int ) },
    {"getNodes", PyWrap2( Graph, getNodesCopy, "Get all node IDs", vector<Graph::node> ) },
    {NULL} /* Sentinel */
};





