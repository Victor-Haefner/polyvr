#include "VRPyBRep.h"
#include "core/scripting/VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(BRepSurface, New_ptr);
simpleVRPyType(BRepEdge, New_ptr);
simpleVRPyType(BRepBound, New_ptr);

PyMethodDef VRPyBRepSurface::methods[] = {
    {"addBound", PyWrap( BRepSurface, addBound, "Add bound", void, VRBRepBoundPtr ) },
    {"build", PyWrap( BRepSurface, build, "Build surface as geometry", VRGeometryPtr ) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyBRepEdge::methods[] = {
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyBRepBound::methods[] = {
    {"addEdge", PyWrap( BRepBound, addEdge, "Add boundary edge", void, VRBRepEdgePtr ) },
    {"compute", PyWrap( BRepBound, compute, "Compute tessellation", void ) },
    {"build", PyWrap( BRepBound, build, "Build surface as geometry", VRGeometryPtr ) },
    {NULL}  /* Sentinel */
};
