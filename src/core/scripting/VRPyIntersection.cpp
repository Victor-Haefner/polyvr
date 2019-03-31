#include "VRPyIntersection.h"
#include "VRPyBaseT.h"
#include "VRPyTypeCaster.h"
#include "core/utils/toString.h"

using namespace OSG;

simpleVRPyType(Intersection, 0);

template<> PyObject* VRPyTypeCaster::cast(const VRIntersection& i) { return VRPyIntersection::fromObject(i); }

template<> bool toValue(PyObject* o, VRIntersection& v) { if (!VRPyIntersection::check(o)) return 0; v = *((VRPyIntersection*)o)->objPtr; return 1; }
template<> int toValue(stringstream& ss, VRIntersection& v) { return 1; }

PyMethodDef VRPyIntersection::methods[] = {
    {"getIntersected", PyWrap(Intersection, getIntersected, "Get intersected object", VRObjectPtr ) },
    {"getIntersection", PyWrap(Intersection, getIntersection, "Get intersection point", Pnt3d ) },
    {NULL}  /* Sentinel */
};
