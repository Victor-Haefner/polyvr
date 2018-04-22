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
    {NULL}  /* Sentinel */
};


