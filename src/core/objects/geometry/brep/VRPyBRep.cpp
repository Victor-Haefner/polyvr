#include "VRPyBRep.h"
#include "core/scripting/VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(BRepSurface, New_ptr);
simpleVRPyType(BRepEdge, New_ptr);
simpleVRPyType(BRepBound, New_ptr);

PyMethodDef VRPyBRepBound::methods[] = {
    {"addEdge", PyWrap( BRepBound, addEdge, "Add boundary edge", void, VRBRepEdgePtr ) },
    {"compute", PyWrap( BRepBound, compute, "Compute tessellation", void ) },
    {"build", PyWrap( BRepBound, build, "Build surface as geometry", VRGeometryPtr ) },
    {"getPoints", PyWrap( BRepBound, getPoints, "Get bound points from compute", vector<Vec3d> ) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyBRepSurface::methods[] = {
    {"setPose", PyWrap( BRepSurface, setPose, "Set surface orientation and position", void, PosePtr ) },
    {"setPlane", PyWrap( BRepSurface, setPlane, "Set plane surface", void ) },
    {"setCylinder", PyWrap( BRepSurface, setCylinder, "Set cylinder surface, setCylinder(radius)", void, double ) },
    {"addBound", PyWrap( BRepSurface, addBound, "Add bound", void, VRBRepBoundPtr ) },
    {"build", PyWrapOpt( BRepSurface, build, "Build surface as geometry, optional flag to only build 2D", "0", VRGeometryPtr, bool ) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyBRepEdge::methods[] = {
    {"setLine", PyWrap( BRepEdge, setLine, "Set line edge, setLine(begin, end)", void, Vec3d, Vec3d ) },
    {"setCircle", PyWrap( BRepEdge, setCircle, "Set circle edge, angles from -pi to pi setCircle(radius, center, angle1, angle2)", void, PosePtr, double, double, double ) },
    {"compute", PyWrap( BRepEdge, compute, "Compute tessellation", void ) },
    {"getPoints", PyWrap( BRepEdge, getPoints, "Get edge points from compute", vector<Vec3d> ) },
    {NULL}  /* Sentinel */
};
