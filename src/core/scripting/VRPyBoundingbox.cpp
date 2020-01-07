#include "VRPyBoundingbox.h"
#include "VRPyBaseT.h"
#include "VRPyBaseFactory.h"

namespace OSG { typedef Boundingbox VRBoundingbox; }

using namespace OSG;

newPyType(Boundingbox, Boundingbox, New_ptr);

template<> bool toValue(PyObject* o, Boundingbox& v) { if (!VRPyBoundingbox::check(o)) return 0; v = *((VRPyBoundingbox*)o)->objPtr; return 1; }

PyMethodDef VRPyBoundingbox::methods[] = {
    {"min", PyWrap(Boundingbox, py_min, "Get the minimum vector", Vec3d) },
    {"max", PyWrap(Boundingbox, py_max, "Get the maximum vector", Vec3d) },
    {"update", PyWrap(Boundingbox, py_update, "Update the bounding box", void, Vec3d) },
    {"center", PyWrap(Boundingbox, py_center, "Get the center", Vec3d) },
    {"size", PyWrap(Boundingbox, py_size, "Get the size", Vec3d) },
    {"radius", PyWrap(Boundingbox, py_radius, "Get the size", float) },
    {"volume", PyWrap(Boundingbox, py_volume, "Get the size", float) },
    {"setCenter", PyWrap(Boundingbox, py_setCenter, "Get the size", void, Vec3d) },
    {"move", PyWrap(Boundingbox, py_move, "Get the size", void, Vec3d) },
    {"scale", PyWrap(Boundingbox, scale, "Get the size", void, float) },
    {"inflate", PyWrap(Boundingbox, inflate, "Get the size", void, float) },
    {"isInside", PyWrap(Boundingbox, py_isInside, "Get the size", bool, Vec3d) },
    {"intersectedBy", PyWrap(Boundingbox, intersectedBy, "Get the size", bool, Line) },
    {"clamp", PyWrap(Boundingbox, py_clamp, "Get the size", void, Vec3d) },
    {"getRandomPoint", PyWrap(Boundingbox, getRandomPoint, "Get the size", Vec3d) },
    {"intersect", PyWrap(Boundingbox, intersect, "Check if intersects other boundingbox", bool, BoundingboxPtr) },
    {"asGeometry", PyWrap(Boundingbox, asGeometry, "Return boundingbox as cube geometry", VRGeometryPtr) },
    {NULL}  /* Sentinel */
};
