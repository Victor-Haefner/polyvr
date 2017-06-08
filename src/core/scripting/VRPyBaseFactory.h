#ifndef VRPYBASEFACTORY_H_INCLUDED
#define VRPYBASEFACTORY_H_INCLUDED

#include "VRPyBase.h"
#include "VRPyTypeCaster.h"
#include "core/utils/VRCallbackWrapper.h"

template<typename T>
bool toValue(PyObject* o, T& b);

#include "core/utils/VRCallbackWrapperT.h"

template<typename ...Args>
struct proxyParams {
    template<Args... args>
    struct Values {
        static int size() {
            const int n = sizeof...(Args);
            return n;
        }
    };
};

template<typename sT, typename T, T, class O> struct proxyWrap;
template<typename sT, typename T, typename R, typename ...Args, R (T::*mf)(Args...), class O>
struct proxyWrap<sT, R (T::*)(Args...), mf, O> {
    static PyObject* exec(sT* self, PyObject* args);
};

template<typename sT, typename T, typename R, typename ...Args, R (T::*mf)(Args...), class O>
PyObject* proxyWrap<sT, R (T::*)(Args...), mf, O>::exec(sT* self, PyObject* args) {
    if (!self->valid()) return NULL;
    vector<PyObject*> params;
    for (int i=0; i<PyTuple_Size(args); i++) params.push_back(PyTuple_GetItem(args, i));
    auto wrap = OSG::VRCallbackWrapperT<PyObject*, O, R (T::*)(Args...)>::create();
    wrap->callback = mf;
    PyObject* res = 0;
    bool success = wrap->execute(self->objPtr.get(), params, res);
    if (!success) { self->setErr(wrap->err); return NULL; }
    if (!res) Py_RETURN_TRUE;
    else return res;
}

#define PyWrap(X, Y, R, ...) \
(PyCFunction)proxyWrap<VRPy ## X, R (OSG::VR ## X::*)( __VA_ARGS__ ), &OSG::VR ## X::Y, void >::exec \
, METH_VARARGS

// the I can be set to an integer less than or egual to the number of parameters, this means that I highest parameters are optional!

#define PyWrapOpt1(X, Y, R, ...) \
(PyCFunction)proxyWrap<VRPy ## X, R (OSG::VR ## X::*)( __VA_ARGS__ ), &OSG::VR ## X::Y, proxyParams<__VA_ARGS__>

#define PyWrapOpt2(...) \
::Values<0.0> >::exec \
, METH_VARARGS

// ::Values<__VA_ARGS__> >::exec

#endif // VRPYBASEFACTORY_H_INCLUDED
