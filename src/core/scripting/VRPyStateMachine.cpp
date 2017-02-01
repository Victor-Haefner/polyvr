#include "VRPyStateMachine.h"
#include "VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(StateMachine, New_ptr);

PyMethodDef VRPyStateMachine::methods[] = {
    {"addState", (PyCFunction)VRPyStateMachine::addState, METH_VARARGS, "Add a state - addState( str )" },
    {"setState", (PyCFunction)VRPyStateMachine::setState, METH_VARARGS, "Set the state - setState( str )" },
    {"getState", (PyCFunction)VRPyStateMachine::getState, METH_VARARGS, "Get the current state - str getState()" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyStateMachine::addState(VRPyStateMachine* self, PyObject *args) {
    if (!self->valid()) return NULL;
    self->objPtr->addState( parseString(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyStateMachine::setState(VRPyStateMachine* self, PyObject *args) {
    if (!self->valid()) return NULL;
    self->objPtr->setState( parseString(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyStateMachine::getState(VRPyStateMachine* self, PyObject *args) {
    if (!self->valid()) return NULL;
    return PyString_FromString( self->objPtr->getState().c_str() );
}

