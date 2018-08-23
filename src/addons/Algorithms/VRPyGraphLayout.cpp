#include "VRPyGraphLayout.h"
#include "core/scripting/VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(GraphLayout, New_ptr);

PyMethodDef VRPyGraphLayout::methods[] = {
    {"setGraph", PyWrap( GraphLayout, setGraph, "Set graph - setGraph(graph)", void, GraphPtr ) },
    {"setAlgorithm", PyWrapOpt( GraphLayout, setAlgorithm, "Set pipeline algorithms - setAlgorithm( str algorithm, int position )\n\talgorithm: 'SPRINGS', 'OCCUPANCYMAP'", "0", void, string, int ) },
    {"setParameters", PyWrap( GraphLayout, setRadius, "Set parameters - setParameters( flt radius )", void, float ) },
    {"fixNode", PyWrap( GraphLayout, fixNode, "Fix a node, making it static - fixNode( int n )", void, int ) },
    {"compute", PyWrapOpt( GraphLayout, compute, "Compute N steps - compute( int steps, float threshold )", "0.1", void, int, float ) },
    {NULL}  /* Sentinel */
};
