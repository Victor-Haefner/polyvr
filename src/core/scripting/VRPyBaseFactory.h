#ifndef VRPYBASEFACTORY_H_INCLUDED
#define VRPYBASEFACTORY_H_INCLUDED

#include "VRPyBase.h"
#include "VRPyTypeCaster.h"
#include "core/utils/VRCallbackWrapper.h"
#include "core/utils/VRCallbackWrapperT.h"

template<typename T> bool toValue(PyObject* o, T& b);
template<typename T> bool toValue(PyObject* o, vector<T>& v) {
    for (int i=0; i<VRPyBase::pySize(o); i++) {
        T t;
        PyObject* oi = PyList_GetItem(o, i);
        if (!toValue(oi, t)) return 0;
        v.push_back( t );
    }
    return 1;
}

template<>
struct VRCallbackWrapper<PyObject*> : VRCallbackWrapperBase {
    VRCallbackWrapper() {}
    virtual ~VRCallbackWrapper() {}

    template<typename T>
    PyObject* convert(const T& t) { return VRPyTypeCaster::cast(t); }

    template<typename T>
    PyObject* convert(const vector<T>& t) { return VRPyTypeCaster::cast(t); }

    virtual bool execute(void* obj, const vector<PyObject*>& params, PyObject*& result) = 0;
};

template<typename sT, typename T, T, class O> struct proxyWrap;
template<typename sT, typename T, typename R, typename ...Args, R (T::*mf)(Args...), class O>
struct proxyWrap<sT, R (T::*)(Args...), mf, O> {
    static PyObject* exec(sT* self, PyObject* args);
};

template<typename sT, typename T, typename R, typename ...Args, R (T::*mf)(Args...), class O>
PyObject* proxyWrap<sT, R (T::*)(Args...), mf, O>::exec(sT* self, PyObject* args) {
    if (!self->valid()) return NULL; // error set in call to valid
    vector<PyObject*> params;
    for (int i=0; i<PyTuple_Size(args); i++) params.push_back(PyTuple_GetItem(args, i));
    auto wrap = VRCallbackWrapperT<PyObject*, O, R (T::*)(Args...)>::create();
    wrap->callback = mf;
    PyObject* res = 0;
    bool success = wrap->execute(self->objPtr.get(), params, res);
    if (!success) { self->setErr(wrap->err); return NULL; }
    if (!res) Py_RETURN_TRUE;
    else return res;
}

// macros to allow iteration over macro variadic for documentation

#define FOR_EACH1(X, ...) #X
#define FOR_EACH2(X, ...) #X ", " FOR_EACH1( __VA_ARGS__ )
#define FOR_EACH3(X, ...) #X ", " FOR_EACH2( __VA_ARGS__ )
#define FOR_EACH4(X, ...) #X ", " FOR_EACH3( __VA_ARGS__ )
#define FOR_EACH5(X, ...) #X ", " FOR_EACH4( __VA_ARGS__ )
#define FOR_EACH6(X, ...) #X ", " FOR_EACH5( __VA_ARGS__ )
#define FOR_EACH7(X, ...) #X ", " FOR_EACH6( __VA_ARGS__ )
#define FOR_EACH8(X, ...) #X ", " FOR_EACH7( __VA_ARGS__ )

#define FOR_EACH_NARG(...) FOR_EACH_ARG_N(__VA_ARGS__, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define FOR_EACH_ARG_N(_1, _2, _3, _4, _5, _6, _7, _8, N, ...) N

#define CONCATENATE(arg1, arg2) arg1##arg2
#define FOR_EACH_(N, ...) CONCATENATE(FOR_EACH, N)(__VA_ARGS__)
#define FOR_EACH(...) FOR_EACH_(FOR_EACH_NARG(__VA_ARGS__), __VA_ARGS__)


// actual py wrapper macros

#define PyWrapDoku(F, D, R, ...) \
D " - " #R " " #F "( " FOR_EACH( __VA_ARGS__ ) " )"

#define PyWrap(X, Y, D, R, ...) \
(PyCFunction)proxyWrap<VRPy ## X, R (OSG::VR ## X::*)( __VA_ARGS__ ), &OSG::VR ## X::Y, VRCallbackWrapperParams<MACRO_GET_STR( "" )> >::exec , METH_VARARGS, PyWrapDoku(Y,D,R,__VA_ARGS__)

#define PyCastWrap(X, Y, D, R, ...) \
(PyCFunction)proxyWrap<VRPy ## X, R (OSG::VR ## X::*)( __VA_ARGS__ ), (R (OSG::VR ## X::*)( __VA_ARGS__ )) &OSG::VR ## X::Y, VRCallbackWrapperParams<MACRO_GET_STR( "" )> >::exec , METH_VARARGS, PyWrapDoku(Y,D,R,__VA_ARGS__)

// TODO
//#define PyConstWrap(X, Y, D, R, ...) \
//(PyCFunction)proxyWrap<VRPy ## X, R (*const OSG::VR ## X::*)( __VA_ARGS__ ), &OSG::VR ## X::Y, VRCallbackWrapperParams<MACRO_GET_STR( "" )> >::exec , METH_VARARGS, PyWrapDoku(Y,D,R,__VA_ARGS__)

#define PyWrap2(X, Y, D, R, ...) \
(PyCFunction)proxyWrap<VRPy ## X, R (OSG::X::*)( __VA_ARGS__ ), (R (OSG::X::*)( __VA_ARGS__ )) &OSG::X::Y, VRCallbackWrapperParams<MACRO_GET_STR( "" )> >::exec , METH_VARARGS, PyWrapDoku(Y,D,R,__VA_ARGS__)


// pass optional parameters S as a single string, will all arguments separated by '|'

#define PyWrapOpt(X, Y, D, S, R, ...) \
(PyCFunction)proxyWrap<VRPy ## X, R (OSG::VR ## X::*)( __VA_ARGS__ ), &OSG::VR ## X::Y, VRCallbackWrapperParams< MACRO_GET_STR( S ) > >::exec, METH_VARARGS, PyWrapDoku(Y,D,R,__VA_ARGS__)

#define PyCastWrapOpt(X, Y, D, S, R, ...) \
(PyCFunction)proxyWrap<VRPy ## X, R (OSG::VR ## X::*)( __VA_ARGS__ ), (R (OSG::VR ## X::*)( __VA_ARGS__ )) &OSG::VR ## X::Y, VRCallbackWrapperParams< MACRO_GET_STR( S ) > >::exec, METH_VARARGS, PyWrapDoku(Y,D,R,__VA_ARGS__)

#define PyWrapOpt2(X, Y, D, S, R, ...) \
(PyCFunction)proxyWrap<VRPy ## X, R (OSG::X::*)( __VA_ARGS__ ), (R (OSG::X::*)( __VA_ARGS__ )) &OSG::X::Y, VRCallbackWrapperParams< MACRO_GET_STR( S ) > >::exec, METH_VARARGS, PyWrapDoku(Y,D,R,__VA_ARGS__)

#endif // VRPYBASEFACTORY_H_INCLUDED
