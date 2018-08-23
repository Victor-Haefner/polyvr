#include "VRPyBoundingbox.h"
#include "VRPyBaseT.h"
#include "VRPyBaseFactory.h"

namespace OSG { typedef Boundingbox VRBoundingbox; }

using namespace OSG;

newPyType(Boundingbox, Boundingbox, New_ptr);

template<> bool toValue(PyObject* o, Boundingbox& v) { if (!VRPyBoundingbox::check(o)) return 0; v = *((VRPyBoundingbox*)o)->objPtr; return 1; }

PyMethodDef VRPyBoundingbox::methods[] = {
    {"min", PyCastWrap(Boundingbox, min, "Get the minimum vector", Vec3d) },
    {"max", PyCastWrap(Boundingbox, max, "Get the maximum vector", Vec3d) },
    {"update", PyCastWrap(Boundingbox, update, "Update the bounding box", void, Vec3d) },
    {"center", PyCastWrap(Boundingbox, center, "Get the center", Vec3d) },
    {"size", PyCastWrap(Boundingbox, size, "Get the size", Vec3d) },
    {"radius", PyCastWrap(Boundingbox, radius, "Get the size", float) },
    {"volume", PyCastWrap(Boundingbox, volume, "Get the size", float) },
    {"setCenter", PyCastWrap(Boundingbox, setCenter, "Get the size", void, Vec3d) },
    {"move", PyCastWrap(Boundingbox, move, "Get the size", void, Vec3d) },
    {"scale", PyCastWrap(Boundingbox, scale, "Get the size", void, float) },
    {"inflate", PyCastWrap(Boundingbox, inflate, "Get the size", void, float) },
    {"isInside", PyCastWrap(Boundingbox, isInside, "Get the size", bool, Vec3d) },
    {"intersectedBy", PyCastWrap(Boundingbox, intersectedBy, "Get the size", bool, Line) },
    {"clamp", PyCastWrap(Boundingbox, clamp, "Get the size", void, Vec3d) },
    {"getRandomPoint", PyCastWrap(Boundingbox, getRandomPoint, "Get the size", Vec3d) },
    {NULL}  /* Sentinel */
};
