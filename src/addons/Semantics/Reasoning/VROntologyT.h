#ifndef VRONTOLOGYT_H_INCLUDED
#define VRONTOLOGYT_H_INCLUDED

#include "VROntology.h"
#include "VRSemanticUtils.h"
#include "core/utils/toString.h"

OSG_BEGIN_NAMESPACE;

template <typename T> struct VRSemanticBuiltinT;
template <typename T, typename R, typename ...Args>
struct VRSemanticBuiltinT<R (T::*)(Args...)> : public VRSemanticBuiltin {
    typedef R (T::*Callback)(Args...);
    Callback callback;

    static VRSemanticBuiltinPtr create() {
        return VRSemanticBuiltinPtr( new VRSemanticBuiltinT() );
    }

    template<class A> // TODO: generic solution instead one for each number of parameter??
    R call(T* obj, const Params& params) {
        A a; toValue(params[0], a);
        (obj->*callback)( a );
    }

    template<class A, class B>
    R call(T* obj, const Params& params) {
        A a; toValue(params[0], a);
        B b; toValue(params[1], b);
        (obj->*callback)( a, b );
    }

    template<class A, class B, class C>
    R call(T* obj, const Params& params) {
        A a; toValue(params[0], a);
        B b; toValue(params[1], b);
        C c; toValue(params[2], c);
        (obj->*callback)( a, b, c );
    }

    bool execute(VRObjectPtr o, const Params& params) {
        auto obj = dynamic_pointer_cast<T>(o);
        if (!obj) return false;
        call<Args...>(obj.get(), params);
        return true;
    }
};

template <typename T, typename R, typename ...Args>
VRSemanticBuiltinPtr VROntology::addBuiltin( string builtin, R (T::*callback)(Args...) ) {
    auto b = VRSemanticBuiltinT<R (T::*)(Args...)>::create();
    builtins[builtin] = b;
    return b;
}

OSG_END_NAMESPACE;

#endif // VRONTOLOGYT_H_INCLUDED




