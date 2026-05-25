#include "VRPyPolygon.h"
#include "VRPyMath.h"
#include "VRPyBaseT.h"

using namespace OSG;

template<> PyObject* VRPyTypeCaster::cast(const VRPolygon& p) { return VRPyPolygon::fromObject(p); }
template<> PyObject* VRPyTypeCaster::cast(const Frustum& p) { return VRPyFrustum::fromObject(p); }
template<> bool toValue(PyObject* o, VRPolygon& v) { if (!VRPyPolygon::check(o)) return 0; v = *((VRPyPolygon*)o)->objPtr; return 1; }
template<> bool toValue(PyObject* o, Frustum& v) { if (!VRPyFrustum::check(o)) return 0; v = *((VRPyFrustum*)o)->objPtr; return 1; }

simpleVRPyType(Polygon, New_ptr);
simplePyType(Frustum, New_ptr);

PyMethodDef VRPyFrustum::methods[] = {
    {"isInside", PyWrap2(Frustum, isInside, "Check if point in frustum", bool, Vec3d) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyPolygon::methods[] = {
    {"addPoint", PyWrap(Polygon, addPoint, "Add a point - addPoint([x,y])", void, Vec2d ) },
    {"getPoint", PyWrap(Polygon, getPoint, "Get a point - [x,y] getPoint( int i )", Vec2d, int ) },
    {"getPoints", PyWrap(Polygon, getPoints, "Get the list of points - [[x,y]] getPoints()", vector<Vec2d>& ) },
    {"getPoints3", PyWrap(Polygon, getPoints3, "Get the list of points - [[x,y,z]] getPoints3()", vector<Vec3d>& ) },
    {"getConvexHull", PyWrap(Polygon, getConvexHull, "Get the convex hull - VRPolygon getConvexHull()", VRPolygonPtr ) },
    {"close", PyWrap(Polygon, close, "Close the VRPolygon - close()", void ) },
    {"size", PyWrap(Polygon, size, "Get the number of points - int size()", int ) },
    {"set", PyWrap(Polygon, set, "Set the VRPolygon from a list of points - set( [[x,y]] )", void, vector<Vec2d> ) },
    {"clear", PyWrap(Polygon, clear, "Clear all points - clear()", void ) },
    {"getRandomPoints", PyWrapOpt(Polygon, getRandomPoints, "Clear all points - getRandomPoints( | float density, float padding, float spread)", "10|0|0.5", vector<Vec3d>, double, double, double ) },
    {"isInside", PyWrap(Polygon, isInside, "Check if point is inside polygon", bool, Vec2d) },
    {"gridSplit", PyWrap(Polygon, gridSplit, "Split the polygon using a virtual grid layout", vector< VRPolygonPtr >, double) },
    {"reverseOrder", PyWrap(Polygon, reverseOrder, "Reverse the order of the points", void) },
    {"translate", PyWrap(Polygon, translate, "Translate all points", void, Vec3d) },
    {"removeDoubles", PyWrapOpt(Polygon, removeDoubles, "Remove doubles", "0.001", void, float) },
    {"shrink", PyWrap(Polygon, shrink, "Shrink polygon, move edges inwards", VRPolygonPtr, double) },
    {"grow", PyWrap(Polygon, grow, "Grow polygon, move edges outwards", VRPolygonPtr, double) },
    {"getBoundingBox", PyWrap(Polygon, getBoundingBox, "Get a boundingbox of the polygon", Boundingbox) },
    {"reorder", PyWrap(Polygon, reorder, "Reorder the points to set the rotation direction around the bb center, 'CW' or 'CCW'", void, string) },
    {NULL}  /* Sentinel */
};


