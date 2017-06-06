#ifndef VRONTOLOGYT_H_INCLUDED
#define VRONTOLOGYT_H_INCLUDED

#include "core/utils/VRCallbackWrapperT.h"
#include "VROntology.h"

OSG_BEGIN_NAMESPACE;

template <typename T, typename R, typename ...Args>
VRCallbackStrWrapperPtr VROntology::addBuiltin( string builtin, R (T::*callback)(Args...) ) {
    auto b = VRCallbackWrapperT<string, R (T::*)(Args...)>::create();
    b->callback = callback;
    builtins[builtin] = b;
    return b;
}

OSG_END_NAMESPACE;

#endif // VRONTOLOGYT_H_INCLUDED




