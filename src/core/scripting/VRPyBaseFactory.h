#ifndef VRPYBASEFACTORY_H_INCLUDED
#define VRPYBASEFACTORY_H_INCLUDED

#include "VRPyBase.h"
#include "VRPyTypeCaster.h"
#include "core/utils/VRCallbackWrapper.h"
#include "core/utils/VRCallbackWrapperT.h"

template<typename T> bool toValue(PyObject* o, T& b);

template<typename T> bool toValue(PyObject* o, vector<T>& v) {
    if (!PyList_Check(o)) return 0;
    for (int i=0; i<VRPyBase::pySize(o); i++) {
        T t;
        PyObject* oi = PyList_GetItem(o, i);
        if (!toValue(oi, t)) return 0;
        v.push_back( t );
    }
    return 1;
}

template<typename T> bool toValue(PyObject* o, vector<vector<T>>& v) {
    if (!PyList_Check(o)) return 0;
    for (int i=0; i<VRPyBase::pySize(o); i++) {
        PyObject* oi = PyList_GetItem(o, i);
        v.push_back( vector<T>() );
        if (!toValue(oi, v[i])) return 0;
    }
    return 1;
}

template<typename T, typename U> bool toValue(PyObject* o, map<T, U>& m) {
    if (!PyDict_Check(o)) return 0;
    PyObject* keys = PyDict_Keys(o);
    for (int i=0; i<VRPyBase::pySize(keys); i++) {
        T t;
        U u;
        PyObject* k = PyList_GetItem(keys, i);
        PyObject* f = PyDict_GetItem(o, k);
        if (!toValue(k, t)) return 0;
        if (!toValue(f, u)) return 0;
        m[t] = u;
    }
    return 1;
}

template<typename T> bool toValue(PyObject* o, std::shared_ptr<VRFunction<T>>& v) {
    //if (!VRPyEntity::check(o)) return 0; // TODO: add checks!
    if (VRPyBase::isNone(o)) v = 0;
    else {
        Py_IncRef(o);
        v = VRFunction<T>::create( "pyExecCall", bind(VRPyBase::execPyCallVoid<T>, o, PyTuple_New(1), std::placeholders::_1) );
    }
    return 1;
}

template<> bool toValue(PyObject* o, std::shared_ptr<VRFunction<void>>& v);

template<>
struct VRCallbackWrapper<PyObject*> : VRCallbackWrapperBase {
    VRCallbackWrapper() {}
    virtual ~VRCallbackWrapper() {}

    template<typename T>
    PyObject* convert(const T& t) { return VRPyTypeCaster::cast(t); }

    template<typename T>
    PyObject* convert(const vector<T>& t) { return VRPyTypeCaster::cast(t); }

    template<typename T, typename G>
    PyObject* convert(const map<T,G>& t) { return VRPyTypeCaster::cast(t); }

    virtual bool execute(void* obj, const vector<PyObject*>& params, PyObject*& result) = 0;
};

template<bool allowPacking, typename sT, typename T, T, class O> struct proxyWrap;
template<bool allowPacking, typename sT, typename T, typename R, typename ...Args, R (T::*mf)(Args...), class O>
struct proxyWrap<allowPacking, sT, R (T::*)(Args...), mf, O> {
    static PyObject* exec(sT* self, PyObject* args);
};

template<bool allowPacking, typename sT, typename T, typename R, typename ...Args, R (T::*mf)(Args...), class O>
PyObject* proxyWrap<allowPacking, sT, R (T::*)(Args...), mf, O>::exec(sT* self, PyObject* args) {
    if (!self->valid()) return NULL; // error set in call to valid
    vector<PyObject*> params;
    for (int i=0; i<PyTuple_Size(args); i++) params.push_back(PyTuple_GetItem(args, i));
    auto wrap = VRCallbackWrapperT<PyObject*, O, R (T::*)(Args...)>::create();
    if (!wrap) { self->setErr( "Internal error in proxyWrap, invalid wrapper!" ); return NULL; }

    size_t Nargs = sizeof...(Args); // try packing parameter into a list
    if (params.size() >= 2 && params.size() <= 4 && Nargs == 1 && allowPacking) {
        PyObject* res = PyList_New(params.size());
        for (uint i=0; i<params.size(); i++) PyList_SetItem(res, i, params[i]);
        params = { res };
    }

    wrap->callback = mf;
    PyObject* res = 0;
    T* tPtr = self->objPtr ? self->objPtr.get() : self->obj;
    bool success = wrap->execute(tPtr, params, res);
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
#define FOR_EACH9(X, ...) #X ", " FOR_EACH8( __VA_ARGS__ )

#define FOR_EACH_NARG(...) FOR_EACH_ARG_N(__VA_ARGS__, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define FOR_EACH_ARG_N(_1, _2, _3, _4, _5, _6, _7, _8, _9, N, ...) N

#define CONCATENATE(arg1, arg2) arg1##arg2
#define FOR_EACH_(N, ...) CONCATENATE(FOR_EACH, N)(__VA_ARGS__)
#define FOR_EACH(...) FOR_EACH_(FOR_EACH_NARG(__VA_ARGS__), __VA_ARGS__)


// actual py wrapper macros

#define PyWrapDoku(F, D, R, ...) \
#R " " #F "( " FOR_EACH( __VA_ARGS__ ) " )\n\t" D

#define PyWrap(X, Y, D, R, ...) \
(PyCFunction)proxyWrap<0, VRPy ## X, R (OSG::VR ## X::*)( __VA_ARGS__ ), &OSG::VR ## X::Y, VRCallbackWrapperParams<MACRO_GET_STR( "" )> >::exec , METH_VARARGS, PyWrapDoku(Y,D,R,__VA_ARGS__)

#define PyWrapPack(X, Y, D, R, ...) \
(PyCFunction)proxyWrap<1, VRPy ## X, R (OSG::VR ## X::*)( __VA_ARGS__ ), &OSG::VR ## X::Y, VRCallbackWrapperParams<MACRO_GET_STR( "" )> >::exec , METH_VARARGS, PyWrapDoku(Y,D,R,__VA_ARGS__)

/* // WASM does not like the cast!
#define PyCastWrap(X, Y, D, R, ...) \
(PyCFunction)proxyWrap<0, VRPy ## X, R (OSG::VR ## X::*)( __VA_ARGS__ ), (R (OSG::VR ## X::*)( __VA_ARGS__ )) &OSG::VR ## X::Y, VRCallbackWrapperParams<MACRO_GET_STR( "" )> >::exec , METH_VARARGS, PyWrapDoku(Y,D,R,__VA_ARGS__)
*/

/* TODO
#define PyConstWrap(X, Y, D, R, ...) \
(PyCFunction)proxyWrap<VRPy ## X, R (*const OSG::VR ## X::*)( __VA_ARGS__ ), &OSG::VR ## X::Y, VRCallbackWrapperParams<MACRO_GET_STR( "" )> >::exec , METH_VARARGS, PyWrapDoku(Y,D,R,__VA_ARGS__)
*/

#define PyWrap2(X, Y, D, R, ...) \
(PyCFunction)proxyWrap<0, VRPy ## X, R (OSG::X::*)( __VA_ARGS__ ), &OSG::X::Y, VRCallbackWrapperParams<MACRO_GET_STR( "" )> >::exec , METH_VARARGS, PyWrapDoku(Y,D,R,__VA_ARGS__)

/* // WASM does not like the cast!
#define PyCastWrap2(X, Y, D, R, ...) \
(PyCFunction)proxyWrap<0, VRPy ## X, R (OSG::X::*)( __VA_ARGS__ ), (R (OSG::X::*)( __VA_ARGS__ )) &OSG::X::Y, VRCallbackWrapperParams<MACRO_GET_STR( "" )> >::exec , METH_VARARGS, PyWrapDoku(Y,D,R,__VA_ARGS__)
*/

/*
#define PyWrap2(X, Y, D, R, ...) \
(PyCFunction)proxyWrap<VRPy ## X, R (OSG::X::*)( __VA_ARGS__ ), (R (OSG::X::*)( __VA_ARGS__ )) &OSG::X::Y, VRCallbackWrapperParams<MACRO_GET_STR( "" )> >::exec , METH_VARARGS, PyWrapDoku(Y,D,R,__VA_ARGS__)
*/

// pass optional parameters S as a single string, will all arguments separated by '|'

#define PyWrapOpt(X, Y, D, S, R, ...) \
(PyCFunction)proxyWrap<0, VRPy ## X, R (OSG::VR ## X::*)( __VA_ARGS__ ), &OSG::VR ## X::Y, VRCallbackWrapperParams< MACRO_GET_STR( S ) > >::exec, METH_VARARGS, PyWrapDoku(Y,D,R,__VA_ARGS__)

/* // WASM does not like the cast!
#define PyWrapOpt(X, Y, D, S, R, ...) \
(PyCFunction)proxyWrap<0, VRPy ## X, R (OSG::VR ## X::*)( __VA_ARGS__ ), (R (OSG::VR ## X::*)( __VA_ARGS__ )) &OSG::VR ## X::Y, VRCallbackWrapperParams< MACRO_GET_STR( S ) > >::exec, METH_VARARGS, PyWrapDoku(Y,D,R,__VA_ARGS__)
*/

/* // WASM does not like the cast!
#define PyWrapOpt2(X, Y, D, S, R, ...) \
(PyCFunction)proxyWrap<0, VRPy ## X, R (OSG::X::*)( __VA_ARGS__ ), (R (OSG::X::*)( __VA_ARGS__ )) &OSG::X::Y, VRCallbackWrapperParams< MACRO_GET_STR( S ) > >::exec, METH_VARARGS, PyWrapDoku(Y,D,R,__VA_ARGS__)
*/

#define PyWrapOpt2(X, Y, D, S, R, ...) \
(PyCFunction)proxyWrap<0, VRPy ## X, R (OSG::X::*)( __VA_ARGS__ ), &OSG::X::Y, VRCallbackWrapperParams< MACRO_GET_STR( S ) > >::exec, METH_VARARGS, PyWrapDoku(Y,D,R,__VA_ARGS__)

#endif // VRPYBASEFACTORY_H_INCLUDED
