#include "VRPyPathtool.h"
#include "VRPyMaterial.h"
#include "VRPyObject.h"
#include "VRPyGeometry.h"
#include "VRPyDevice.h"
#include "VRPyPath.h"
#include "VRPyGraph.h"
#include "VRPyBaseT.h"
#include "VRPyStroke.h"
#include "VRPyPose.h"
#include "VRPyTypeCaster.h"

using namespace OSG;

simpleVRPyType(Pathtool, New_ptr);

PyMethodDef VRPyPathtool::methods[] = {
    {"newPath", PyWrapOpt( Pathtool, newPath, "Add a new path", "10|0", PathPtr, VRDevicePtr, VRObjectPtr, int, bool ) },
    {"remPath", PyWrap( Pathtool, remPath, "Remove a path", void, PathPtr ) },
    {"extrude", PyWrap(Pathtool, extrude, "Extrude a path", VRGeometryPtr, VRDevicePtr, PathPtr) },
    {"addPath", PyWrapOpt( Pathtool, addPath, "Add a path and add resulting stroke to object", "0|0|0|1", void, PathPtr, VRObjectPtr, VRGeometryPtr, VRGeometryPtr, bool ) },
    {"select", PyWrap( Pathtool, select, "Select handle", void, VRGeometryPtr ) },
    {"selectPath", PyWrap( Pathtool, selectPath, "Select path", void, PathPtr ) },
    {"deselect", PyWrap( Pathtool, deselect, "Deselect anything previously selected - deselect()", void ) },
    {"setVisuals", PyWrapOpt( Pathtool, setVisuals, "Set the tool visibility, showStrokes, showHandles)", "0", void, bool, bool ) },
    {"setBezierVisuals", PyWrapOpt( Pathtool, setBezierVisuals, "Set the bezier visibility, controlPoints, hulls)", "0", void, bool, bool ) },
    {"getPaths", PyWrapOpt( Pathtool, getPaths, "Return all paths or paths connected to handle, handle", "0", vector<PathPtr>, VRGeometryPtr ) },
    {"getPath", PyWrap( Pathtool, getPath, "Return path between handles h1 and h2 - [path] getPath( handle h1, handle h2 )", PathPtr, VRGeometryPtr, VRGeometryPtr ) },
    {"getHandle", PyWrap( Pathtool, getHandle, "Return a handle by node ID", VRGeometryPtr, int ) },
    {"getHandles", PyWrap(Pathtool, getHandles, "Return a list of paths handles", vector<VRGeometryPtr>, PathPtr ) },
    {"getStroke", PyWrap( Pathtool, getStroke, "Return the stroke object", VRStrokePtr, PathPtr ) },
    {"update", PyWrap( Pathtool, update, "Update the tool", void ) },
    {"clear", PyWrapOpt( Pathtool, clear, "Clear a path or all paths nodes", "0", void, PathPtr ) },
    {"setHandleGeometry", PyWrap( Pathtool, setHandleGeometry, "Replace the default handle geometry - setHandleGeometry( geo )", void, VRGeometryPtr ) },
    {"getPathMaterial", PyWrap( Pathtool, getPathMaterial, "Get the material used for paths geometry", VRMaterialPtr ) },
    {"getArrowMaterial", PyWrap( Pathtool, getArrowMaterial, "Get the material used for arrow geometry", VRMaterialPtr ) },
    {"setGraph", PyWrapOpt( Pathtool, setGraph, "Setup from graph, flags: doClear, doHandles, doArrows", "1|0|0", void, GraphPtr, bool, bool, bool ) },
    {"addHandle", PyWrap( Pathtool, addHandle, "Add handle, graph nodeID, pose", VRGeometryPtr, int, PosePtr ) },
    {"addNode", PyWrap( Pathtool, addNode, "Add node", int, PosePtr ) },
    {"removeNode", PyWrap( Pathtool, removeNode, "Remove node by id", void, int ) },
    {"getNodeID", PyWrap(Pathtool, getNodeID, "Return node ID from handle", int, VRObjectPtr) },
    {"connect", PyWrapOpt( Pathtool, connect, "Connect two nodes by id, using optional normals, id1, id2, n1, n2, doHandles, addArrow)", "0,0,0|0,0,0|1|0", void, int, int, Vec3d, Vec3d, bool, bool ) },
    {"disconnect", PyWrap( Pathtool, disconnect, "Disconnect two nodes - disconnect( id1, id2 )", void, int, int ) },
    {"setProjectionGeometry", PyWrap( Pathtool, setProjectionGeometry, "Set an object to project handles onto", void, VRObjectPtr ) },
    {"setEdgeResolution", PyWrap( Pathtool, setEdgeResolution, "Set edge resolution, eID, res", void, int, int ) },
    {"setEdgeColor", PyWrap( Pathtool, setEdgeColor, "Set edge color, eID, color1, color2", void, int, Color3f, Color3f ) },
    {"setEdgeBulge", PyWrap( Pathtool, setEdgeBulge, "Set edge bulge, eID, bulge", void, int, Vec3d ) },
    {"setEdgeSmoothGraphNodes", PyWrap( Pathtool, setEdgeSmoothGraphNodes, "Set edge flag for computing smooth graph nodes", void, int, bool ) },
    {"setEdgeVisibility", PyWrap( Pathtool, setEdgeVisibility, "Set edge flag for computing smooth graph nodes", void, int, bool ) },
    {"setArrowSize", PyWrap( Pathtool, setArrowSize, "Set arrow sizes, call after the arrows are instantiated!", void, float ) },
    {NULL}  /* Sentinel */
};























