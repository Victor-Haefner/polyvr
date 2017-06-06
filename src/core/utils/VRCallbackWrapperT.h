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

    template<class A> // TODO: generic solution instead one for each number of parameter??
    R call(T* obj, const P& params) {
        if (params.size() != 1) return R();
        A a; toValue(params[0], a);
        return (obj->*callback)( a );
    }

    template<class A, class B>
    R call(T* obj, const P& params) {
        if (params.size() != 2) return R();
        A a; toValue(params[0], a);
        B b; toValue(params[1], b);
        return (obj->*callback)( a, b );
    }

    template<class A, class B, class C>
    R call(T* obj, const P& params) {
        if (params.size() != 3) return R();
        A a; toValue(params[0], a);
        B b; toValue(params[1], b);
        C c; toValue(params[2], c);
        return (obj->*callback)( a, b, c );
    }

    bool execute(void* o, const P& params) {
        if (!callback) return false;
        call<Args...>((T*)o, params);
        return true;
    }
};

OSG_END_NAMESPACE;

#endif // VRCALLBACKWRAPPERT_H_INCLUDED
