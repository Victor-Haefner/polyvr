template<class T> PyTypeObject* VRPyBaseT<T>::typeRef = &VRPyBaseT<T>::type;
template<class T> VRPyBaseT<T>::VRPyBaseT() {;}

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
    if (self != NULL) self->obj->addAttachment("dynamicaly_generated", 0);
    return (PyObject *)self;
}

template<class T>
PyObject* VRPyBaseT<T>::New_VRObjects_optional(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    VRPyBaseT<T>* self = 0;
    if (pySize(args) == 0) self = (VRPyBaseT<T> *)New(type, args, kwds);
    else self = (VRPyBaseT<T> *)New_named(type, args, kwds);
    if (self != NULL) self->obj->addAttachment("dynamicaly_generated", 0);
    return (PyObject *)self;
}

template<class T>
PyObject* VRPyBaseT<T>::New_VRObjects_unnamed(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    VRPyBaseT<T>* self = (VRPyBaseT<T> *)New(type, args, kwds);
    if (self != NULL) self->obj->addAttachment("dynamicaly_generated", 0);
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
