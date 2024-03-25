#ifndef VRSIGNALT_H_INCLUDED
#define VRSIGNALT_H_INCLUDED

#include "VRSignal.h"
#include "core/utils/VRFunction.h"
#include "core/scene/VRScene.h"

OSG_BEGIN_NAMESPACE;

template<typename Event>
bool VRSignal::trigger(vector<VRBaseCbWeakPtr>& callbacks, shared_ptr<Event> event) {
    if (!event && this->event) event = ((Event*)this->event)->ptr();

    using cbType = VRFunction< weak_ptr<Event>, bool >;

    for (auto& c : callbacks) {
        if (VRBaseCbPtr spc = c.lock()) {
            if (!spc) continue;
            auto cb = dynamic_pointer_cast<cbType>(spc);
            if (!cb) continue;

            bool abort = !(*cb)(event);
            if (abort) return false;
        }
    }

    return true;
}

template<typename Event>
bool VRSignal::triggerAll(shared_ptr<Event> event) {
    auto callbacks = callbacksPtr; // make copy to avoid corruption while iterating!

    for (auto& prio : callbacks) {
        bool abort = !trigger(prio.second, event);
        if (abort) return false;
    }

    if (callbacksDeferredPtr.size()) {
        auto scene = VRScene::getCurrent();
        if (scene) scene->queueJob( VRUpdateCb::create("sigDeferred", bind(&VRSignal::triggerAllDeferred<Event>, this, event) ) );
    }

    return true;
}

template<typename Event>
bool VRSignal::triggerAllDeferred(shared_ptr<Event> event) {
    auto callbacks = callbacksDeferredPtr; // make copy to avoid corruption while iterating!

    for (auto& prio : callbacks) {
        bool abort = !trigger(prio.second, event);
        if (abort) return false;
    }

    return true;
}

OSG_END_NAMESPACE

#endif // VRSIGNALT_H_INCLUDED
