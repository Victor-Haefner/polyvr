#include <boost/bind.hpp>

template<class T> PyTypeObject* VRPyBaseT<T>::typeRef = &VRPyBaseT<T>::type;
template<class T> VRPyBaseT<T>::VRPyBaseT() {;}

template <typename T>
bool VRPyBaseT<T>::check(PyObject* o) { return typeRef == o->ob_type; }

template <typename T>
void VRPyBase::execPyCall(PyObject* pyFkt, PyObject* pArgs, T t) {
    if (pyFkt == 0) return;
    PyGILState_STATE gstate = PyGILState_Ensure();
    if (PyErr_Occurred() != NULL) PyErr_Print();

    PyTuple_SetItem(pArgs, pySize(pArgs)-1, toPyObject(t));
    PyObject_CallObject(pyFkt, pArgs);

    //Py_XDECREF(pArgs); Py_DecRef(pyFkt); // TODO!!

    if (PyErr_Occurred() != NULL) PyErr_Print();
    PyGILState_Release(gstate);
}

template <typename T>
VRFunction<T>* VRPyBase::parseCallback(PyObject* args) {
	PyObject* pyFkt = 0;
	PyObject* pArgs = 0;
    if (pySize(args) == 1) if (! PyArg_ParseTuple(args, "O", &pyFkt)) return 0;
    if (pySize(args) == 2) if (! PyArg_ParseTuple(args, "OO", &pyFkt, &pArgs)) return 0;
	if (pyFkt == 0) return 0;
    Py_IncRef(pyFkt);

    if (pArgs == 0) pArgs = PyTuple_New(0);
    else if (string(pArgs->ob_type->tp_name) == "list") pArgs = PyList_AsTuple(pArgs);
    _PyTuple_Resize(&pArgs, pySize(pArgs)+1);

    return new VRFunction<T>( "pyExecCall", boost::bind(VRPyBase::execPyCall<T>, pyFkt, pArgs, _1) );
}

template <class T, class t>
bool VRPyBase::pyListToVector(PyObject* o, T& vec) {
    PyObject *pi, *pj;
    t tmp;
    Py_ssize_t N = PyList_Size(o);

    for (Py_ssize_t i=0; i<N; i++) {
        pi = PyList_GetItem(o, i);
        for (Py_ssize_t j=0; j<PyList_Size(pi); j++) {
            pj = PyList_GetItem(pi, j);
            tmp[j] = PyFloat_AsDouble(pj);
        }
        vec.push_back(tmp);
    }
    return true;
}

template<class T>
PyObject* VRPyBaseT<T>::fromPtr(T* obj) {
    if (obj == 0) Py_RETURN_NONE;
    VRPyBaseT<T> *self = (VRPyBaseT<T> *)typeRef->tp_alloc(typeRef, 0);
    if (self != NULL) {
        self->obj = obj;
        self->owner = false;
    }
    return (PyObject *)self;
}

template<class T>
bool VRPyBaseT<T>::parse(PyObject *args, T** obj) {
    *obj = 0;
    if (args == 0) return false;
    VRPyBaseT<T>* o = NULL;
    if (! PyArg_ParseTuple(args, "O", &o)) return false;
    if (isNone((PyObject*)o)) { PyErr_SetString(err, "Object passed is None!"); return false; }
    *obj = o->obj;
    return true;
}

template<class T>
PyObject* VRPyBaseT<T>::alloc(PyTypeObject* type, T* t) {
    VRPyBaseT<T>* self = (VRPyBaseT<T> *)type->tp_alloc(type, 0);
    if (self != NULL) {
        self->owner = true;
        self->obj = t;
    }
    return (PyObject *)self;
}

template<class T> PyObject* VRPyBaseT<T>::New(PyTypeObject *type, PyObject *args, PyObject *kwds) { return alloc( type, new T() ); }
template<class T> PyObject* VRPyBaseT<T>::New_named(PyTypeObject *type, PyObject *args, PyObject *kwds) { return alloc( type, new T( parseString(args) ) ); }
template<class T> PyObject* VRPyBaseT<T>::New_toZero(PyTypeObject *type, PyObject *args, PyObject *kwds) { return alloc( type, 0 ); }

template<class T>
PyObject* VRPyBaseT<T>::New_VRObjects(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    VRPyBaseT<T>* self = (VRPyBaseT<T> *)New_named(type, args, kwds);
    if (self != NULL) self->obj->setPersistency(0);
    return (PyObject *)self;
}

template<class T>
PyObject* VRPyBaseT<T>::New_VRObjects_optional(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    VRPyBaseT<T>* self = 0;
    if (pySize(args) == 0) self = (VRPyBaseT<T> *)New(type, args, kwds);
    else self = (VRPyBaseT<T> *)New_named(type, args, kwds);
    if (self != NULL) self->obj->setPersistency(0);
    return (PyObject *)self;
}

template<class T>
PyObject* VRPyBaseT<T>::New_VRObjects_unnamed(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    VRPyBaseT<T>* self = (VRPyBaseT<T> *)New(type, args, kwds);
    if (self != NULL) self->obj->setPersistency(0);
    return (PyObject *)self;
}

template<class T>
void VRPyBaseT<T>::dealloc(VRPyBaseT<T>* self) {
    //if (self->owner && self->obj != 0) delete self->obj; // TOCHECK
    self->ob_type->tp_free((PyObject*)self);
}

template<class T>
int VRPyBaseT<T>::init(VRPyBaseT<T> *self, PyObject *args, PyObject *kwds) {
    return 0;
}

template<class T>
void VRPyBaseT<T>::registerModule(string name, PyObject* mod, PyTypeObject* tp_base) {
    if (tp_base) typeRef->tp_base = tp_base;

    if ( PyType_Ready(typeRef) < 0 ) { cout << "\nERROR! could not register " << name << endl; return; }
    Py_INCREF(typeRef);
    PyModule_AddObject(mod, name.c_str(), (PyObject*)typeRef);
}
