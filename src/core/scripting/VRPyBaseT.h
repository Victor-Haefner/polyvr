#ifndef VRPYBASET_H_INCLUDED
#define VRPYBASET_H_INCLUDED

#include "VRPyTypeCaster.h"
#include "VRPyBaseFactory.h"
#include "core/scene/VRScene.h"
#include "core/objects/object/VRObject.h"

#define newPyType( X, Y, NEWfkt ) \
template<> PyTypeObject VRPyBaseT< X >::type = { \
    PyObject_HEAD_INIT(NULL) \
    0, \
    "VR." #Y, \
    sizeof( VRPy ## Y ),0, \
    (destructor)dealloc, \
    0,0,0,0,0,0,0,0,0,0,0,0,0,0, \
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, \
    "VR" #Y " binding", \
    0,0,0,0,0,0, \
    VRPy ## Y ::methods, \
    0,0,0,0,0,0,0, \
    (initproc)init, 0, \
    NEWfkt, \
}; \
\
template <> \
bool toValue(PyObject* o, std::shared_ptr<X>& v) { \
    if (VRPyBase::isNone(o)) { v = 0; return 1; } \
    if (!VRPy ## Y::check(o)) return 0; \
    v = ((VRPy ## Y*)o)->objPtr; return 1; \
}; \
\
template <> \
bool toValue(PyObject* o, X*& v) { \
    if (VRPyBase::isNone(o)) { v = 0; return 1; } \
    if (!VRPy ## Y::check(o)) return 0; \
    v = ((VRPy ## Y*)o)->obj; return 1; \
}; \
\
template<> \
PyObject* VRPyTypeCaster::cast(const std::shared_ptr<X>& e) { \
    return VRPy ## Y::fromSharedPtr(e); \
};\
\
template<> \
PyObject* VRPyTypeCaster::cast(const std::weak_ptr<X>& e) { \
    return VRPy ## Y::fromSharedPtr(e.lock()); \
};\
\
template<> \
string typeName(const X& e) { \
    return #Y; \
};

#define simpleVRPyType( X, NEWfkt ) newPyType( VR ## X , X , NEWfkt )
#define simplePyType( X, NEWfkt ) newPyType( X , X , NEWfkt )

template<class T> PyTypeObject* VRPyBaseT<T>::typeRef = &VRPyBaseT<T>::type;
template<class T> VRPyBaseT<T>::VRPyBaseT() {;}
/*template<class T> VRPyBaseT<T>::~VRPyBaseT() {
    cout << "VRPyBaseT<T>::destruct " << this << " " << this->obj << " " << this->objPtr << " " << typeRef->tp_name << endl;
}*/

template <typename T>
bool VRPyBaseT<T>::valid() {
    if ((objPtr && objPtr.get()) || obj) return true;
    setErr("Py object is invalid!");
    return false;
}

template <typename T>
bool VRPyBaseT<T>::check(PyObject* o) {
    return (PyObject_IsInstance(o, (PyObject*)typeRef) == 1);
}

template <typename T, typename R>
R VRPyBase::execPyCall(PyObject* pyFkt, PyObject* pArgs, T t) {
    R r;
    if (pyFkt == 0) return r;
    PyGILState_STATE gstate = PyGILState_Ensure();
    if (PyErr_Occurred() != NULL) PyErr_Print();

    PyTuple_SetItem(pArgs, pySize(pArgs)-1, VRPyTypeCaster::cast(t));
    auto res = PyObject_CallObject(pyFkt, pArgs);

    //Py_XDECREF(pArgs); Py_DecRef(pyFkt); // TODO!!

    if (PyErr_Occurred() != NULL) PyErr_Print();
    PyGILState_Release(gstate);

    toValue(res, r);
    return r;
}

template <typename T>
void VRPyBase::execPyCallVoid(PyObject* pyFkt, PyObject* pArgs, T t) {
    if (pyFkt == 0) return;
    PyGILState_STATE gstate = PyGILState_Ensure();
    if (PyErr_Occurred() != NULL) PyErr_Print();

    PyTuple_SetItem(pArgs, pySize(pArgs)-1, VRPyTypeCaster::cast(t));
    PyObject_CallObject(pyFkt, pArgs);

    //Py_XDECREF(pArgs); Py_DecRef(pyFkt); // TODO!!

    if (PyErr_Occurred() != NULL) PyErr_Print();
    PyGILState_Release(gstate);
}

template <typename T, typename R>
VRFunction<T, R>* VRPyBase::parseCallback(PyObject* args) {
	PyObject* pyFkt = 0;
	PyObject* pArgs = 0;
    if (pySize(args) == 1) if (! PyArg_ParseTuple(args, "O", &pyFkt)) return 0;
    if (pySize(args) == 2) if (! PyArg_ParseTuple(args, "OO", &pyFkt, &pArgs)) return 0;
	if (pyFkt == 0) return 0;
    Py_IncRef(pyFkt);

    if (pArgs == 0) pArgs = PyTuple_New(0);
    else if (string(pArgs->ob_type->tp_name) == "list") pArgs = PyList_AsTuple(pArgs);
    _PyTuple_Resize(&pArgs, pySize(pArgs)+1);

    return new VRFunction<T, R>( "pyExecCall", bind(VRPyBase::execPyCall<T, R>, pyFkt, pArgs, std::placeholders::_1) );
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
PyObject* VRPyBaseT<T>::fromObject(T obj) {
    VRPyBaseT<T> *self = (VRPyBaseT<T> *)typeRef->tp_alloc(typeRef, 0);
    if (self == NULL) Py_RETURN_NONE;
    T* optr = new T(obj);
    self->objPtr = std::shared_ptr<T>( optr );
    self->owner = false;
    return (PyObject *)self;
}

template<class T>
PyObject* VRPyBaseT<T>::fromPtr(T* obj) {
    VRPyBaseT<T> *self = (VRPyBaseT<T> *)typeRef->tp_alloc(typeRef, 0);
    if (self == NULL) Py_RETURN_NONE;
    self->obj = obj;
    self->owner = false;
    return (PyObject *)self;
}

template<class T>
PyObject* VRPyBaseT<T>::fromSharedPtr(std::shared_ptr<T> obj) {
    if (obj == 0) Py_RETURN_NONE;
    if (typeRef->tp_alloc == 0) {
        cout << "VRPyBase::fromSharedPtr for type " << typeName<T>(*obj) << " failed because of missing type alloc" << endl;
        Py_RETURN_NONE;
    }
    VRPyBaseT<T> *self = (VRPyBaseT<T> *)typeRef->tp_alloc(typeRef, 0);
    if (self == NULL) {
        cout << "VRPyBase::fromSharedPtr for type " << typeName<T>(*obj) << " failed because of failed type alloc" << endl;
        Py_RETURN_NONE;
    }
    self->objPtr = obj;
    self->owner = false;
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
bool VRPyBaseT<T>::parse(PyObject *args, std::shared_ptr<T>* obj) {
    if (args == 0) return false;
    VRPyBaseT<T>* o = NULL;
    if (! PyArg_ParseTuple(args, "O", &o)) return false;
    if (isNone((PyObject*)o)) { PyErr_SetString(err, "Object passed is None!"); return false; }
    *obj = o->objPtr;
    return true;
}

template<class T>
PyObject* VRPyBaseT<T>::allocPtr(PyTypeObject* type, std::shared_ptr<T> t) {
    VRPyBaseT<T>* self = (VRPyBaseT<T> *)type->tp_alloc(type, 0);
    if (self != NULL) {
        self->owner = true;
        self->objPtr = t;
    }
    return (PyObject *)self;
}

template<class T> PyObject* VRPyBaseT<T>::New_ptr(PyTypeObject *type, PyObject *args, PyObject *kwds) { return allocPtr( type, T::create() ); }
template<class T> PyObject* VRPyBaseT<T>::New_toZero(PyTypeObject *type, PyObject *args, PyObject *kwds) { return allocPtr( type, 0 ); }

template<class T> PyObject* VRPyBaseT<T>::New_named_ptr(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    const char* n = 0;
    if (! PyArg_ParseTuple(args, "s", (char*)&n)) return NULL;
    return allocPtr( type, T::create( string(n) ) );
}

template<class T>
PyObject* VRPyBaseT<T>::New_VRObjects_ptr(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    const char* name = 0;
    const char* parent = 0;
    if (! PyArg_ParseTuple(args, "|ss", &name, &parent)) return NULL;
    VRPyBaseT<T>* self = 0;
    if (name) self = (VRPyBaseT<T>*)allocPtr( type, T::create( name ) );
    else self = (VRPyBaseT<T>*)allocPtr( type, T::create() );
    if (self) self->objPtr->setPersistency(0);
    if (parent) {
        auto p = OSG::VRScene::getCurrent()->get(parent);
        if (p) p->addChild(self->objPtr);
    }
    return (PyObject *)self;
}

template<class T>
void VRPyBaseT<T>::dealloc(VRPyBaseT<T>* self) {
    //cout << "VRPyBaseT<T>::dealloc " << self << " " << self->obj << " " << self->objPtr << " " << typeRef->tp_name << endl;
    //if (self->owner && self->obj != 0) delete self->obj; // TOCHECK
    if (self->objPtr) self->objPtr = 0;
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

#endif //VRPYBASET_H_INCLUDED
