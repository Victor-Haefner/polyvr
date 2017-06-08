#ifndef VRCALLBACKWRAPPERT_H_INCLUDED
#define VRCALLBACKWRAPPERT_H_INCLUDED

#include "VRCallbackWrapper.h"

#define CW_CHECK_SIZE(N) \
if (params.size() > N) { \
    VRCallbackWrapperBase::err = "Function takes at most "+toString(N)+" arguments ("+toString( params.size() )+" given)"; \
    return false; \
}

#define CW_GET_VALUE(i,T,t) \
T t; \
if (! toValue(params[i], t)) { \
    VRCallbackWrapperBase::err = "Function argument "+toString(i)+" expects a "+typeName(t)+" ("+typeName( params[i] )+" given)"; \
    return false; \
}

//    this->err = "Function argument "+toString(i)+" expects a "+typeName(t)+" ("+typeName( params[i] )+" given)";

OSG_BEGIN_NAMESPACE;

template <typename P, typename U, typename T> struct VRCallbackWrapperT;
template <typename P, typename U, typename T, typename R, typename ...Args>
struct VRCallbackWrapperT<P, U, R (T::*)(Args...)> : public VRCallbackWrapper<P> {
    typedef R (T::*Callback)(Args...);
    Callback callback;

    static shared_ptr<VRCallbackWrapperT> create() {
        return shared_ptr<VRCallbackWrapperT>( new VRCallbackWrapperT() );
    }

     // TODO: generic solution instead one for each number of parameter??
     // template class O is not really needed, only a trick to allow to call without parameters

    template<class O>
    bool call(T* obj, const vector<P>& params, R& r) {
        CW_CHECK_SIZE(0);
        r = (obj->*callback)(); return true;
    }

    template<class O, class A>
    bool call(T* obj, const vector<P>& params, R& r) {
        CW_CHECK_SIZE(1);
        CW_GET_VALUE(0,A,a);
        r = (obj->*callback)(); return true;
    }

    template<class O, class A, class B>
    bool call(T* obj, const vector<P>& params, R& r) {
        CW_CHECK_SIZE(2);
        CW_GET_VALUE(0,A,a);
        CW_GET_VALUE(1,B,b);
        r = (obj->*callback)( a, b ); return true;
    }

    template<class O, class A, class B, class C>
    bool call(T* obj, const vector<P>& params, R& r) {
        CW_CHECK_SIZE(3);
        CW_GET_VALUE(0,A,a);
        CW_GET_VALUE(1,B,b);
        CW_GET_VALUE(2,C,c);
        r = (obj->*callback)( a, b, c ); return true;
    }

    template<class O, class A, class B, class C, class D>
    bool call(T* obj, const vector<P>& params, R& r) {
        CW_CHECK_SIZE(4);
        CW_GET_VALUE(0,A,a);
        CW_GET_VALUE(1,B,b);
        CW_GET_VALUE(2,C,c);
        CW_GET_VALUE(3,D,d);
        r = (obj->*callback)( a, b, c, d ); return true;
    }

    template<class O, class A, class B, class C, class D, class E>
    bool call(T* obj, const vector<P>& params, R& r) {
        CW_CHECK_SIZE(5);
        CW_GET_VALUE(0,A,a);
        CW_GET_VALUE(1,B,b);
        CW_GET_VALUE(2,C,c);
        CW_GET_VALUE(3,D,d);
        CW_GET_VALUE(4,E,e);
        r = (obj->*callback)( a, b, c, d, e ); return true;
    }

    template<class O, class A, class B, class C, class D, class E, class F>
    bool call(T* obj, const vector<P>& params, R& r) {
        CW_CHECK_SIZE(6);
        CW_GET_VALUE(0,A,a);
        CW_GET_VALUE(1,B,b);
        CW_GET_VALUE(2,C,c);
        CW_GET_VALUE(3,D,d);
        CW_GET_VALUE(4,E,e);
        CW_GET_VALUE(5,F,f);
        r = (obj->*callback)( a, b, c, d, e, f ); return true;
    }

    bool execute(void* o, const vector<P>& params, P& result) {
        if (!callback) return false;
        R res;
        if (!call<R, Args...>((T*)o, params, res)) return false;
        result = this->convert( res );
        return true;
    }
};

// --- void specialisation ---

template <typename P, typename U, typename T, typename ...Args>
struct VRCallbackWrapperT<P, U, void (T::*)(Args...)> : public VRCallbackWrapper<P> {
    typedef void (T::*Callback)(Args...);
    Callback callback;

    static shared_ptr<VRCallbackWrapperT> create() {
        return shared_ptr<VRCallbackWrapperT>( new VRCallbackWrapperT() );
    }


    template<class O>
    bool call(T* obj, const vector<P>& params) {
        CW_CHECK_SIZE(0);
        (obj->*callback)(); return true;
    }

    template<class O, class A>
    bool call(T* obj, const vector<P>& params) {
        CW_CHECK_SIZE(1);
        CW_GET_VALUE(0,A,a);
        (obj->*callback)(a); return true;
    }

    template<class O, class A, class B>
    bool call(T* obj, const vector<P>& params) {
        CW_CHECK_SIZE(2);
        CW_GET_VALUE(0,A,a);
        CW_GET_VALUE(1,B,b);
        (obj->*callback)( a, b ); return true;
    }

    template<class O, class A, class B, class C>
    bool call(T* obj, const vector<P>& params) {
        CW_CHECK_SIZE(3);
        CW_GET_VALUE(0,A,a);
        CW_GET_VALUE(1,B,b);
        CW_GET_VALUE(2,C,c);
        (obj->*callback)( a, b, c ); return true;
    }

    template<class O, class A, class B, class C, class D>
    bool call(T* obj, const vector<P>& params) {
        CW_CHECK_SIZE(4);
        CW_GET_VALUE(0,A,a);
        CW_GET_VALUE(1,B,b);
        CW_GET_VALUE(2,C,c);
        CW_GET_VALUE(3,D,d);
        (obj->*callback)( a, b, c, d ); return true;
    }

    template<class O, class A, class B, class C, class D, class E>
    bool call(T* obj, const vector<P>& params) {
        CW_CHECK_SIZE(5);
        CW_GET_VALUE(0,A,a);
        CW_GET_VALUE(1,B,b);
        CW_GET_VALUE(2,C,c);
        CW_GET_VALUE(3,D,d);
        CW_GET_VALUE(4,E,e);
        (obj->*callback)( a, b, c, d, e ); return true;
    }

    template<class O, class A, class B, class C, class D, class E, class F>
    bool call(T* obj, const vector<P>& params) {
        CW_CHECK_SIZE(6);
        CW_GET_VALUE(0,A,a);
        CW_GET_VALUE(1,B,b);
        CW_GET_VALUE(2,C,c);
        CW_GET_VALUE(3,D,d);
        CW_GET_VALUE(4,E,e);
        CW_GET_VALUE(5,F,f);
        (obj->*callback)( a, b, c, d, e, f ); return true;
    }

    bool execute(void* o, const vector<P>& params, P& result) {
        if (!callback) return false;
        if (!call<void, Args...>((T*)o, params)) return false;
        return true;
    }
};

OSG_END_NAMESPACE;

#endif // VRCALLBACKWRAPPERT_H_INCLUDED
