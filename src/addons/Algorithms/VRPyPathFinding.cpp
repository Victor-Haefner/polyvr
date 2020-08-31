#include "VRPyPathFinding.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyGraph.h"
#include "core/scripting/VRPyPath.h"

using namespace OSG;

simpleVRPyType(PathFinding, New_ptr);

static string computePathDoc =
"Compute the best path from start to end\n"
"\tstart/end can either be a node ID or an edge ID followed by a float edge position.";

PyMethodDef VRPyPathFinding::methods[] = {
    {"setGraph", PyWrap(PathFinding, setGraph, "Set pathfinding graph", void, GraphPtr ) },
    {"setPaths", PyWrap(PathFinding, setPaths, "Set pathfinding paths", void, vector<PathPtr> ) },
    {"computePath", (PyCFunction)VRPyPathFinding::computePath, METH_VARARGS, computePathDoc.c_str() },
    //{"computePath", PyWrapOpt(PathFinding, computePath, computePathDoc, "0", vector<Position>, Position },
    {NULL}  /* Sentinel */
};

PyObject* VRPyPathFinding::computePath(VRPyPathFinding* self, PyObject* args) {
    if (!self->valid()) return NULL;
    int i, j;
    float t1 = -1;
    float t2 = -1;
    int bd = 0;
    if (!PyArg_ParseTuple(args, "ii|ffi", &i, &j, &t1, &t2, &bd)) return NULL;

    VRPathFinding::Position p1(i);
    VRPathFinding::Position p2(j);
    if (t1 >= 0) p1 = VRPathFinding::Position(i,t1);
    if (t2 >= 0) p2 = VRPathFinding::Position(j,t2);
    auto route = self->objPtr->computePath( p1, p2, bd );

    PyObject* res = PyList_New(0);
    for (auto p : route) PyList_Append(res, PyInt_FromLong(p.nID));
    return res;
}

