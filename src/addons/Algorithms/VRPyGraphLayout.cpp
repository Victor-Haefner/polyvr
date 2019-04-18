#include "VRPyGraphLayout.h"
#include "core/scripting/VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(GraphLayout, New_ptr);

PyMethodDef VRPyGraphLayout::methods[] = {
    {"setGraph", PyWrap( GraphLayout, setGraph, "Set graph", void, GraphPtr ) },
    {"setAlgorithm", PyWrapOpt( GraphLayout, setAlgorithm, "Set pipeline algorithms - setAlgorithm( str algorithm, int position )\n\talgorithm: 'SPRINGS', 'OCCUPANCYMAP'", "0", void, string, int ) },
    {"setParameters", PyWrap( GraphLayout, setRadius, "Set parameters, radius", void, float ) },
    {"fixNode", PyWrap( GraphLayout, fixNode, "Fix a node, making it static", void, int ) },
    {"compute", PyWrapOpt( GraphLayout, compute, "Compute N steps, steps, threshold", "0.1", void, int, float ) },
    {NULL}  /* Sentinel */
};
