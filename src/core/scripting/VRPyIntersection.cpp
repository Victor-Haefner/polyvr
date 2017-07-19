#include "VRPyIntersection.h"
#include "VRPyBaseT.h"
#include "VRPyTypeCaster.h"

using namespace OSG;

simpleVRPyType(Intersection, 0);

PyMethodDef VRPyIntersection::methods[] = {
    {"getIntersected", (PyCFunction)VRPyIntersection::getIntersected, METH_NOARGS, "Get intersected object - obj getIntersected()" },
    {"getIntersection", (PyCFunction)VRPyIntersection::getIntersection, METH_NOARGS, "Get intersection point - [x,y,z] getIntersection()" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyIntersection::getIntersected(VRPyIntersection* self) {
    if (!self->valid()) return NULL;
    return VRPyTypeCaster::cast( self->objPtr->object.lock() );
}

PyObject* VRPyIntersection::getIntersection(VRPyIntersection* self) {
    if (!self->valid()) return NULL;
    return toPyTuple( Vec3d(self->objPtr->point) );
}
