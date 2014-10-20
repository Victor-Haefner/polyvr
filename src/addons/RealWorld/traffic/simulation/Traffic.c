#include <Python.h>
#include "Traffic.h"

static PyObject* start(PyObject* self) {
	startTraffic();
	Py_RETURN_TRUE;
}

static PyMethodDef Traffic_funcs[] = { 
	{"start", (PyCFunction)start, METH_NOARGS, "Start the simulation"},
	{NULL}
};

PyMODINIT_FUNC initTraffic(void) {
	Py_InitModule3("Traffic", Traffic_funcs, "Python bindings for the Traffic simulation");
}
