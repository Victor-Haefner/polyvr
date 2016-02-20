#include "VRPyMeasure.h"
#include "VRPyGeometry.h"
#include "VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(Measure, New_VRObjects_ptr);

PyMethodDef VRPyMeasure::methods[] = {
    {"setPoint", (PyCFunction)VRPyMeasure::setPoint, METH_VARARGS, "Set one of the three points - setPoint( i, p )\n\twhere i is 0,1 or 2 and p the position" },
    {"rollPoints", (PyCFunction)VRPyMeasure::rollPoints, METH_VARARGS, "Roll through the points - rollPoints( p )" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyMeasure::setPoint(VRPyMeasure* self, PyObject* args) {
    if (!self->valid()) return NULL;
    int i; PyObject* p;
    if (! PyArg_ParseTuple(args, "iO", &i, &p)) return NULL;
    Vec3f v = parseVec3fList(p);
    self->objPtr->setPoint( i, v );
    Py_RETURN_TRUE;
}

PyObject* VRPyMeasure::rollPoints(VRPyMeasure* self, PyObject* args) {
    if (!self->valid()) return NULL;
    self->objPtr->rollPoints( parseVec3f(args) );
    Py_RETURN_TRUE;
}

