#include "VRPyTriangulator.h"
#include "VRPyPolygon.h"
#include "VRPyGeometry.h"
#include "VRPyBaseT.h"

using namespace OSG;

simplePyType(Triangulator, New);

PyMethodDef VRPyTriangulator::methods[] = {
    {"add", (PyCFunction)VRPyTriangulator::add, METH_VARARGS, "Add polygon - add( polygon )" },
    {"compute", (PyCFunction)VRPyTriangulator::compute, METH_NOARGS, "Compute geometry - geo compute()" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyTriangulator::add(VRPyTriangulator* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyPolygon* p;
    if (! PyArg_ParseTuple(args, "O", &p)) return NULL;
    self->obj->add( *p->objPtr );
    Py_RETURN_TRUE;
}

PyObject* VRPyTriangulator::compute(VRPyTriangulator* self) {
    if (!self->valid()) return NULL;
    return VRPyGeometry::fromSharedPtr( self->obj->compute() );
}
