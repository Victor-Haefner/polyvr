#ifndef VRPYBASEFACTORY_H_INCLUDED
#define VRPYBASEFACTORY_H_INCLUDED

#include "VRPyBase.h"
#include "VRPyTypeCaster.h"

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

#endif // VRPYBASEFACTORY_H_INCLUDED
