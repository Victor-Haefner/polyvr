#include "VRPyStateMachine.h"
#include "VRPyBaseT.h"
#include "core/math/VRStateMachine.cpp"

using namespace OSG;

newPyType(VRStateMachinePy, StateMachine, New_named_ptr);
newPyType(VRStateMachinePy::State, State, 0);

PyMethodDef VRPyState::methods[] = {
    {"getName", (PyCFunction)VRPyState::getName, METH_NOARGS, "Return the state name - str getName()" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyState::getName(VRPyState* self) {
    if (!self->valid()) return NULL;
    return PyString_FromString(self->objPtr->getName().c_str());
}

PyMethodDef VRPyStateMachine::methods[] = {
    {"addState", (PyCFunction)VRPyStateMachine::addState, METH_VARARGS, "Add a state - state addState( str, transition cb )" },
    {"getState", (PyCFunction)VRPyStateMachine::getState, METH_VARARGS, "Get the state s - state getState( str s )" },
    {"setCurrentState", (PyCFunction)VRPyStateMachine::setCurrentState, METH_VARARGS, "Set the current state s - state setCurrentState( str s )" },
    {"getCurrentState", (PyCFunction)VRPyStateMachine::getCurrentState, METH_NOARGS, "Get the current state - state getCurrentState()" },
    {"process", (PyCFunction)VRPyStateMachine::process, METH_VARARGS, "Process parameters - state process( { str, str } params )" },
    {NULL}  /* Sentinel */
};

string cbWrapper(PyObject* pyFkt, const PyObject* params) {
    if (pyFkt == 0) return "";
    PyGILState_STATE gstate = PyGILState_Ensure();
    if (PyErr_Occurred() != NULL) PyErr_Print();

    /*PyObject* dict = PyDict_New();
    for (auto p : params) {
        PyDict_SetItem(dict, PyString_FromString(p.first.c_str()), PyString_FromString(p.second.c_str()));
    }*/

    PyObject* args = PyTuple_New(1);
    PyTuple_SetItem(args, 0, (PyObject*)params);
    PyObject* res = PyObject_CallObject(pyFkt, args);
    Py_XDECREF(args);

    if (PyErr_Occurred() != NULL) PyErr_Print();
    PyGILState_Release(gstate);

    if (!res) return "";
    if (!PyString_Check(res)) return "";
    return PyString_AsString(res);
}

PyObject* VRPyStateMachine::addState(VRPyStateMachine* self, PyObject *args) {
    if (!self->valid()) return NULL;
    const char* s = 0;
    PyObject* c;
    if (! PyArg_ParseTuple(args, "sO", &s, &c)) return NULL;

    Py_IncRef(c);
    VRStateMachinePy::VRTransitionCbPtr fkt = VRStateMachinePy::VRTransitionCb::create( "pyStateTransition", bind(cbWrapper, c, placeholders::_1) );

    auto state = self->objPtr->addState( s, fkt );
    return VRPyState::fromSharedPtr( state );
}

PyObject* VRPyStateMachine::getState(VRPyStateMachine* self, PyObject *args) {
    if (!self->valid()) return NULL;
    auto state = self->objPtr->getState( parseString(args) );
    return VRPyState::fromSharedPtr( state );
}

PyObject* VRPyStateMachine::setCurrentState(VRPyStateMachine* self, PyObject *args) {
    if (!self->valid()) return NULL;
    auto state = self->objPtr->setCurrentState( parseString(args) );
    return VRPyState::fromSharedPtr( state );
}

PyObject* VRPyStateMachine::getCurrentState(VRPyStateMachine* self) {
    if (!self->valid()) return NULL;
    auto state = self->objPtr->getCurrentState();
    return VRPyState::fromSharedPtr( state );
}

PyObject* VRPyStateMachine::process(VRPyStateMachine* self, PyObject *args) {
    if (!self->valid()) return NULL;
    PyObject* p;
    if (! PyArg_ParseTuple(args, "O", &p)) return NULL;



    /*map<string, string> params;
    auto plist = PyDict_Items(p);
    for (int i=0; i<pySize(plist); i++) {
        auto paramPair = PyList_GetItem(plist, i);
        auto key = PyTuple_GetItem(paramPair, 0);
        auto value = PyTuple_GetItem(paramPair, 1);
        if (!key || !value) continue;
        if (!PyString_Check(key)) { cout << "VRPyStateMachine::process Warning: key is not a string!\n"; continue; }
        if (!PyString_Check(value)) { cout << "VRPyStateMachine::process Warning: value is not a string!\n"; continue; }
        string skey = PyString_AsString(key);
        string svalue = PyString_AsString(value);
        params[skey] = svalue;
    }*/

    Py_XINCREF(p);
    auto state = self->objPtr->process(p);
    return VRPyState::fromSharedPtr( state );
}

