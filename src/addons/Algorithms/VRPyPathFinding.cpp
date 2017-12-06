#include "VRPyPathFinding.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyGraph.h"
#include "core/scripting/VRPyPath.h"

using namespace OSG;

simpleVRPyType(PathFinding, New_ptr);

PyMethodDef VRPyPathFinding::methods[] = {
    {"setGraph", (PyCFunction)VRPyPathFinding::setGraph, METH_VARARGS, "Set graph - setGraph( graph )" },
    {"setPaths", (PyCFunction)VRPyPathFinding::setPaths, METH_VARARGS, "Set paths - setPaths( [path] )" },
    {"computePath", (PyCFunction)VRPyPathFinding::computePath, METH_VARARGS, "Compute the best path from start to end - computePath( start, end )\n\tstart/end can either be a node ID or an edge ID followed by a float edge position." },
    {NULL}  /* Sentinel */
};

PyObject* VRPyPathFinding::setGraph(VRPyPathFinding* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyGraph* g = 0;
    if (!PyArg_ParseTuple(args, "O", &g)) return NULL;
    self->objPtr->setGraph( g->objPtr );
    Py_RETURN_TRUE;
}

PyObject* VRPyPathFinding::setPaths(VRPyPathFinding* self, PyObject* args) {
    if (!self->valid()) return NULL;
    vector<PyObject*> pypaths = parseList(args);
    vector<PathPtr> paths;
    for (auto p : pypaths) paths.push_back( ((VRPyPath*)p)->objPtr );
    self->objPtr->setPaths( paths );
    Py_RETURN_TRUE;
}

PyObject* VRPyPathFinding::computePath(VRPyPathFinding* self, PyObject* args) {
    if (!self->valid()) return NULL;
    int i, j;
    float t1 = -1;
    float t2 = -1;
    if (PyArg_ParseTuple(args, "ii", &i, &j));
    else if (PyArg_ParseTuple(args, "ifi", &i, &t1, &j)); // TODO: this dows not work for strange reasons!
    else if (PyArg_ParseTuple(args, "iif", &i, &j, &t2));
    else if (PyArg_ParseTuple(args, "ifif", &i, &t1, &j, &t2));
    else return NULL;

    VRPathFinding::Position p1(i);
    VRPathFinding::Position p2(j);
    if (t1 >= 0) p1 = VRPathFinding::Position(i,t1);
    if (t2 >= 0) p1 = VRPathFinding::Position(j,t2);
    auto route = self->objPtr->computePath( p1, p2 );

    PyObject* res = PyList_New(0);
    for (auto p : route) PyList_Append(res, PyInt_FromLong(p.nID));
    return res;
}

