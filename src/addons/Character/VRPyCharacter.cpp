#include "VRPyCharacter.h"
#include "core/scripting/VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(Skeleton, New_ptr);
simpleVRPyType(Character, New_VRObjects_ptr);


PyMethodDef VRPySkeleton::methods[] = {
    {NULL}  /* Sentinel */
};


PyMethodDef VRPyCharacter::methods[] = {
    {"setSkeleton", (PyCFunction)VRPyCharacter::setSkeleton, METH_VARARGS, "Set the skeleton - setSkeleton( skeleton )" },
    {"setSkin", (PyCFunction)VRPyCharacter::setSkin, METH_VARARGS, "Set the skin geometry - setSkin( geometry | skinning )" },
    {"addBehavior", (PyCFunction)VRPyCharacter::addBehavior, METH_VARARGS, "Add a behavior pattern - addBehavior( behavior )" },
    {"triggerBehavior", (PyCFunction)VRPyCharacter::triggerBehavior, METH_VARARGS, "Trigger a certain behavior - triggerBehavior( str)" },
    {"simpleSetup", (PyCFunction)VRPyCharacter::simpleSetup, METH_NOARGS, "Simple character setup - simpleSetup()" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyCharacter::simpleSetup(VRPyCharacter* self) {
    if (!self->valid()) return NULL;
    self->objPtr->simpleSetup();
    Py_RETURN_TRUE;
}

PyObject* VRPyCharacter::setSkeleton(VRPyCharacter* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPySkeleton* g = 0;
    if (!PyArg_ParseTuple(args, "O", &g)) return NULL;
    self->objPtr->setSkeleton( g->objPtr );
    Py_RETURN_TRUE;
}

PyObject* VRPyCharacter::setSkin(VRPyCharacter* self, PyObject* args) {
    if (!self->valid()) return NULL;
    const char* a = 0;
    int p = 0;
    if (!PyArg_ParseTuple(args, "O|O", &a, &p)) return NULL;
    //self->objPtr->setSkin( a, p );
    Py_RETURN_TRUE;
}

PyObject* VRPyCharacter::addBehavior(VRPyCharacter* self, PyObject* args) {
    if (!self->valid()) return NULL;
    float r = 1;
    if (!PyArg_ParseTuple(args, "O", &r)) return NULL;
    //self->objPtr->addBehavior( r );
    Py_RETURN_TRUE;
}

PyObject* VRPyCharacter::triggerBehavior(VRPyCharacter* self, PyObject* args) {
    if (!self->valid()) return NULL;
    int N;
    float t = 0.1;
    if (!PyArg_ParseTuple(args, "s", &N, &t)) return NULL;
    //self->objPtr->triggerBehavior( N, t );
    Py_RETURN_TRUE;
}




