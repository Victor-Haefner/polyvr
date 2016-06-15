#include "VRPyJointTool.h"
#include "VRPyTransform.h"
#include "VRPyPose.h"
#include "VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(JointTool, New_VRObjects_ptr);

PyMethodDef VRPyJointTool::methods[] = {
    {"append", (PyCFunction)VRPyJointTool::append, METH_VARARGS, "Set one of both intersections - append( obj, pose )" },
    {"clear", (PyCFunction)VRPyJointTool::clear, METH_NOARGS, "Clear everything - clear()" },
    {"select", (PyCFunction)VRPyJointTool::select, METH_VARARGS, "Set selection mode - select(bool b)" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyJointTool::append(VRPyJointTool* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyTransform* o; VRPyPose* p;
    if (! PyArg_ParseTuple(args, "OO", &o, &p)) return NULL;
    int i = self->objPtr->append( o->objPtr, p->objPtr );
    return PyInt_FromLong(i);
}

PyObject* VRPyJointTool::select(VRPyJointTool* self, PyObject* args) {
    if (!self->valid()) return NULL;
    self->objPtr->select( parseBool(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyJointTool::clear(VRPyJointTool* self) {
    if (!self->valid()) return NULL;
    self->objPtr->clear();
    Py_RETURN_TRUE;
}

