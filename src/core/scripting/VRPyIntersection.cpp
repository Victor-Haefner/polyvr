#include "VRPyIntersection.h"
#include "VRPyBaseT.h"
#include "VRPyTypeCaster.h"

using namespace OSG;

simpleVRPyType(Intersection, 0);

template<> PyObject* VRPyTypeCaster::cast(const VRIntersection& i) { return VRPyIntersection::fromObject(i); }

PyMethodDef VRPyIntersection::methods[] = {
    {"getIntersected", PyWrap(Intersection, getIntersected, "Get intersected object", VRObjectPtr ) },
    {"getIntersection", PyWrap(Intersection, getIntersection, "Get intersection point", Pnt3d ) },
    {NULL}  /* Sentinel */
};
