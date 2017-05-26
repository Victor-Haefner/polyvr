#include "VRPyBoundingbox.h"
#include "VRPyBaseT.h"

using namespace OSG;

newPyType(boundingbox, Boundingbox, New_ptr);

PyMethodDef VRPyBoundingbox::methods[] = {
    {"min", (PyCFunction)VRPyBoundingbox::min, METH_NOARGS, "Get the minimum vector - [x,y,z] min()" },
    {"max", (PyCFunction)VRPyBoundingbox::max, METH_NOARGS, "Get the maximum vector - [x,y,z] max()" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyBoundingbox::min(VRPyBoundingbox* self) {
    if (!self->valid()) return NULL;
    return toPyTuple( self->objPtr->min() );
}

PyObject* VRPyBoundingbox::max(VRPyBoundingbox* self) {
    if (!self->valid()) return NULL;
    return toPyTuple( self->objPtr->max() );
}


