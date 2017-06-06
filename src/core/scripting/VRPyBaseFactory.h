#ifndef VRPYBASEFACTORY_H_INCLUDED
#define VRPYBASEFACTORY_H_INCLUDED

#include "VRPyBase.h"
#include "VRPyTypeCaster.h"
#include "core/utils/VRCallbackWrapper.h"

template<typename T> bool parseValue(PyObject* args, T& t);
template<typename pyT, typename sT, typename T, T> struct proxy;
template<typename pyT, typename sT, typename T, typename R, typename ...Args, R (T::*mf)(Args...)>
struct proxy<pyT, sT, R (T::*)(Args...), mf> {
    static PyObject* set(sT* self, PyObject* args) {
        if (!self->valid()) return NULL;
        pyT val;
        if( !parseValue<pyT>(args, val) ) return NULL;
        (self->objPtr.get()->*mf)(val);
        Py_RETURN_TRUE;
    }

    static PyObject* get(sT* self) {
        if (!self->valid()) return NULL;
        return VRPyTypeCaster::cast( (self->objPtr.get()->*mf)() );
    }
};

#define PySetter(X, Y, Z) \
(PyCFunction)proxy<Z, VRPy ## X, void (OSG::VR ## X::*)(Z), &OSG::VR ## X::Y>::set \
, METH_VARARGS

#define PyGetter(X, Y, Z) \
(PyCFunction)proxy<void, VRPy ## X, Z (OSG::VR ## X::*)(), &OSG::VR ## X::Y>::get \
, METH_NOARGS


// ----------------- second gen --------------

template<typename T>
void toValue(PyObject* o, T& b);

#include "core/utils/VRCallbackWrapperT.h"

template<typename sT, typename T, T> struct proxyWrap;
template<typename sT, typename T, typename R, typename ...Args, R (T::*mf)(Args...)>
struct proxyWrap<sT, R (T::*)(Args...), mf> {
    static PyObject* exec(sT* self, PyObject* args);
};

template<typename sT, typename T, typename R, typename ...Args, R (T::*mf)(Args...)>
PyObject* proxyWrap<sT, R (T::*)(Args...), mf>::exec(sT* self, PyObject* args) {
    if (!self->valid()) return NULL;
    vector<PyObject*> params;
    for (int i=0; i<PyList_Size(args); i++) params.push_back(PyList_GetItem(args, i));
    auto wrap = OSG::VRCallbackWrapperT<vector<PyObject*>, R (T::*)(Args...)>::create();
    wrap->callback = mf;
    wrap->execute(self->objPtr.get(), params);
    Py_RETURN_TRUE;
}

#define PyWrap(X, P, Y) \
(PyCFunction)proxyWrap<VRPy ## X, void (OSG::VR ## X::*)P, &OSG::VR ## X::Y>::exec \
, METH_VARARGS

#endif // VRPYBASEFACTORY_H_INCLUDED
