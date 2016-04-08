#include "VRPyMeasure.h"
#include "VRPyGeometry.h"
#include "VRPyPose.h"
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
    int i; VRPyPose* p = 0;
    if (! PyArg_ParseTuple(args, "iO", &i, &p)) return NULL;
    if (!p || !p->objPtr) return NULL;
    self->objPtr->setPoint( i, *p->objPtr );
    Py_RETURN_TRUE;
}

PyObject* VRPyMeasure::rollPoints(VRPyMeasure* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyPose* p = 0;
    if (! PyArg_ParseTuple(args, "O", &p)) return NULL;
    if (!p || !p->objPtr) return NULL;
    self->objPtr->rollPoints( *p->objPtr );
    Py_RETURN_TRUE;
}

