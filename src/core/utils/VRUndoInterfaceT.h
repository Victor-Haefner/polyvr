#ifndef VRUNDOINTERFACET_H_INCLUDED
#define VRUNDOINTERFACET_H_INCLUDED

#include "VRUndoInterface.h"
#include "VRFunction.h"
#include "core/tools/VRUndoManager.h"

template<class O>
void OSG_VRUndoInterface_valid(std::weak_ptr<O> o, bool& b) {
    b = o.lock() ? 1 : 0;
}

template<class F, class O, class V>
void OSG::VRUndoInterface::recUndo(F f, std::shared_ptr<O> o, V v1, V v2) {
    auto u = undo.lock();
    if (!u) return;
    if (v1 == v2) return;

    auto f_undo = VRUpdateCb::create( "undo", bind(f, o.get(), v1) );
    auto f_redo = VRUpdateCb::create( "redo", bind(f, o.get(), v2) );

    std::weak_ptr<O> ow = o;
    auto f_valid = VRFunction<bool&>::create( "undo_valid", bind(OSG_VRUndoInterface_valid<O>, ow, placeholders::_1) );

    u->recUndo(f_undo, f_redo, f_valid);
}

#endif // VRUNDOINTERFACET_H_INCLUDED
