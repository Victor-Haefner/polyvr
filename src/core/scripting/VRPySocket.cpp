#include "VRPySocket.h"
#include "VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(Socket, New_named_ptr);

PyMethodDef VRPySocket::methods[] = {
    {"getName", (PyCFunction)VRPySocket::getName, METH_NOARGS, "Return socket name." },
    {"send", (PyCFunction)VRPySocket::send, METH_VARARGS, "Send message - send(string msg)" },
    {"destroy", (PyCFunction)VRPySocket::destroy, METH_NOARGS, "Destroy socket." },
    {"ping", (PyCFunction)VRPySocket::ping, METH_VARARGS, "Test if an address is reachable - bool ping(string IP, string port)" },
    {NULL}  /* Sentinel */
};

PyObject* VRPySocket::ping(VRPySocket* self, PyObject* args) {
	if (!self->valid()) return NULL;
    PyObject *ip, *p;
    if (! PyArg_ParseTuple(args, "OO", &ip, &p)) return NULL;
    return PyBool_FromLong( self->objPtr->ping( PyString_AsString(ip), PyString_AsString(p)) );
}

PyObject* VRPySocket::getName(VRPySocket* self) {
	if (!self->valid()) return NULL;
    return PyString_FromString(self->objPtr->getName().c_str());
}

PyObject* VRPySocket::destroy(VRPySocket* self) {
	if (!self->valid()) return NULL;
    self->objPtr = 0;
    Py_RETURN_TRUE;
}

PyObject* VRPySocket::send(VRPySocket* self, PyObject* args) {
	if (!self->valid()) return NULL;
    PyObject* msg = NULL;
    if (! PyArg_ParseTuple(args, "O", &msg)) return NULL;
    if (msg == NULL) {
        PyErr_SetString(err, "Missing message parameter");
        return NULL;
    }

    string _msg = PyString_AsString(msg);
    //self->objPtr->sendMessage(_msg);
    Py_RETURN_TRUE;
}
