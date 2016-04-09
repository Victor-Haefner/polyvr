#ifndef VRUNDOINTERFACET_H_INCLUDED
#define VRUNDOINTERFACET_H_INCLUDED

#include "VRUndoInterface.h"
#include "VRFunction.h"
#include "core/tools/VRUndoManager.h"

template<class F, class O, class V>
void OSG::VRUndoInterface::recUndo(F f, O* o, V v1, V v2) {
    auto u = undo.lock();
    if (!u) return;
    if (v1 == v2) return;

    auto f_undo = VRFunction<int>::create( "undo", boost::bind(f, o, v1) );
    auto f_redo = VRFunction<int>::create( "redo", boost::bind(f, o, v2) );

    u->recUndo(f_undo, f_redo);
}

#endif // VRUNDOINTERFACET_H_INCLUDED
