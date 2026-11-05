#include "VRPyTriangulator.h"
#include "VRPyPolygon.h"
#include "VRPyGeometry.h"
#include "VRPyBaseT.h"

using namespace OSG;

simplePyType(Triangulator, New_ptr);

PyMethodDef VRPyTriangulator::methods[] = {
    {"add", PyWrapOpt2(Triangulator, add, "Add polygon, bool outer", "1", void, VRPolygon, bool ) },
    {"appendTo", PyWrapOpt2(Triangulator, appendTo, "Append to geometry, returns number of vertices added", "1", size_t, VRGeometryPtr, bool ) },
    {"compute", PyWrap2(Triangulator, compute, "Compute geometry", VRGeometryPtr ) },
    {NULL}  /* Sentinel */
};
