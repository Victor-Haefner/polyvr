#ifndef VRCALLBACKWRAPPERT_H_INCLUDED
#define VRCALLBACKWRAPPERT_H_INCLUDED

#include "VRCallbackWrapper.h"

OSG_BEGIN_NAMESPACE;

template <typename P, typename T> struct VRCallbackWrapperT;
template <typename P, typename T, typename R, typename ...Args>
struct VRCallbackWrapperT<P, R (T::*)(Args...)> : public VRCallbackWrapper<P> {
    typedef R (T::*Callback)(Args...);
    Callback callback;

    static shared_ptr<VRCallbackWrapperT> create() {
        return shared_ptr<VRCallbackWrapperT>( new VRCallbackWrapperT() );
    }

     // TODO: generic solution instead one for each number of parameter??
     // template class O is not really needed, only a trick to allow to call without parameters

    template<class O>
    R call(T* obj, const vector<P>& params) {
        if (params.size() != 0) return R();
        return (obj->*callback)();
    }

    template<class O, class A>
    R call(T* obj, const vector<P>& params) {
        if (params.size() != 1) return R();
        A a; toValue(params[0], a);
        return (obj->*callback)( a );
    }

    template<class O, class A, class B>
    R call(T* obj, const vector<P>& params) {
        if (params.size() != 2) return R();
        A a; toValue(params[0], a);
        B b; toValue(params[1], b);
        return (obj->*callback)( a, b );
    }

    template<class O, class A, class B, class C>
    R call(T* obj, const vector<P>& params) {
        if (params.size() != 3) return R();
        A a; toValue(params[0], a);
        B b; toValue(params[1], b);
        C c; toValue(params[2], c);
        return (obj->*callback)( a, b, c );
    }

    bool execute(void* o, const vector<P>& params, P& result) {
        if (!callback) return false;
        result = this->convert( call<R, Args...>((T*)o, params) );
        return true;
    }
};

template <typename P, typename T, typename ...Args>
struct VRCallbackWrapperT<P, void (T::*)(Args...)> : public VRCallbackWrapper<P> {
    typedef void (T::*Callback)(Args...);
    Callback callback;

    static shared_ptr<VRCallbackWrapperT> create() {
        return shared_ptr<VRCallbackWrapperT>( new VRCallbackWrapperT() );
    }

    template<class O>
    void call(T* obj, const vector<P>& params) {
        if (params.size() != 1) return;
        (obj->*callback)();
    }

    template<class O, class A>
    void call(T* obj, const vector<P>& params) {
        if (params.size() != 1) return;
        A a; toValue(params[0], a);
        (obj->*callback)( a );
    }

    template<class O, class A, class B>
    void call(T* obj, const vector<P>& params) {
        if (params.size() != 2) return;
        A a; toValue(params[0], a);
        B b; toValue(params[1], b);
        (obj->*callback)( a, b );
    }

    template<class O, class A, class B, class C>
    void call(T* obj, const vector<P>& params) {
        if (params.size() != 3) return;
        A a; toValue(params[0], a);
        B b; toValue(params[1], b);
        C c; toValue(params[2], c);
        (obj->*callback)( a, b, c );
    }

    bool execute(void* o, const vector<P>& params, P& result) {
        if (!callback) return false;
        call<void, Args...>((T*)o, params);
        return true;
    }
};

OSG_END_NAMESPACE;

#endif // VRCALLBACKWRAPPERT_H_INCLUDED
