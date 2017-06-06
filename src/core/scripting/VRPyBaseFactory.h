#ifndef VRPYBASEFACTORY_H_INCLUDED
#define VRPYBASEFACTORY_H_INCLUDED

#include "VRPyBase.h"
#include "VRPyTypeCaster.h"
#include "core/utils/VRCallbackWrapper.h"

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
    for (int i=0; i<PyTuple_Size(args); i++) params.push_back(PyTuple_GetItem(args, i));
    auto wrap = OSG::VRCallbackWrapperT<PyObject*, R (T::*)(Args...)>::create();
    wrap->callback = mf;
    PyObject* res = 0;
    wrap->execute(self->objPtr.get(), params, res);
    if (!res) Py_RETURN_TRUE;
    else return res;
}

#define PyWrap(X, Y, R, P) \
(PyCFunction)proxyWrap<VRPy ## X, R (OSG::VR ## X::*)P, &OSG::VR ## X::Y>::exec \
, METH_VARARGS

#endif // VRPYBASEFACTORY_H_INCLUDED
