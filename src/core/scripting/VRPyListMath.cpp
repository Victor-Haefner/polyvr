#include "VRPyListMath.h"

// DEPRECATED

/*void printPyListType() {
    cout << "PyList_Type methods:\n";
    for (int i=0; PyList_Type.tp_methods[i].ml_name != NULL; i++) {
        PyMethodDef& m = PyList_Type.tp_methods[i];
        cout << " " << i << ") " << m.ml_name << endl;
        cout << "      " << m.ml_doc << endl;
    }
}

void appendMethod(PyMethodDef def) {
    int N = 0;
    for (;PyList_Type.tp_methods[N].ml_name != NULL; N++);
    PyMethodDef new_meths[N+2];
    for (int i=0; i<N; i++) new_meths[i] = PyList_Type.tp_methods[i];
    new_meths[N] = def;
    new_meths[N+1] = {NULL};
    PyList_Type.tp_methods = new_meths;
}

void VRPyListMath::init(PyObject* mod) {
    PyMethodDef testDef = {"add", (PyCFunction)VRPyListMath::add, METH_VARARGS, "test add fkt"};
    appendMethod(testDef);


    PyTypeObject* typeRef = &PyList_Type;
    string name = "list";
    if ( PyType_Ready(typeRef) < 0 ) { cout << "\nERROR! could not register " << name << endl; return; }
    PyModule_AddObject(mod, name.c_str(), (PyObject*)typeRef);
}

PyObject* VRPyListMath::add(PyObject* self, PyObject* args) {
    cout << "YAAAAY: add working\n";
    Py_RETURN_TRUE;
}*/
