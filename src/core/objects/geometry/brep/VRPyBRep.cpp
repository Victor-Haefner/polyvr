#include "VRPyBRep.h"
#include "core/scripting/VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(BRepSurface, New_ptr);
simpleVRPyType(BRepEdge, New_ptr);
simpleVRPyType(BRepBound, New_ptr);

PyMethodDef VRPyBRepSurface::methods[] = {
    {"build", PyWrap( BRepSurface, build, "Build surface as geometry", VRGeometryPtr ) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyBRepEdge::methods[] = {
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyBRepBound::methods[] = {
    {"build", PyWrap( BRepSurface, build, "Build surface as geometry", VRGeometryPtr ) },
    {NULL}  /* Sentinel */
};
