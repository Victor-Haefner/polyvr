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

template<typename Event>
bool VRSignal::triggerAllInterleaved(VRSignalPtr other, shared_ptr<Event> event) {
    auto callbacks0 = callbacksPtr;
    auto callbacks1 = other->callbacksPtr;

    if (callbacks0.size() > 0 && callbacks1.size() > 0) {
        int k0 = min(callbacks0.begin()->first , callbacks1.begin()->first );
        int k1 = max(callbacks0.rbegin()->first, callbacks1.rbegin()->first);
        for (int i=k0; i<=k1; i++) {
            if (callbacks0.count(i)) {
                if (!trigger<Event>(callbacks0[i], event)) break;
            }
            if (callbacks1.count(i)) {
                if (!other->trigger<Event>(callbacks1[i], event)) break;
            }
        }
    }
    else if (callbacks0.size() > 0) triggerAll<Event>(event);
    else if (callbacks1.size() > 0) other->triggerAll<Event>(event);

    if (callbacks0.size() == 0 && callbacksDeferredPtr.size() > 0 ||
        callbacks1.size() == 0 && other->callbacksDeferredPtr.size() > 0) {
        auto scene = VRScene::getCurrent();
        if (scene) scene->queueJob( VRUpdateCb::create("sigInterleavedDeferred", bind(&VRSignal::triggerAllInterleavedDeferred<Event>, this, other, event) ) );
    }

    return true;
}

template<typename Event>
bool VRSignal::triggerAllInterleavedDeferred(VRSignalPtr other, shared_ptr<Event> event) {
    auto callbacks0 = callbacksDeferredPtr;
    auto callbacks1 = other->callbacksDeferredPtr;

    if (callbacks0.size() > 0 && callbacks1.size() > 0) {
        int k0 = min(callbacks0.begin()->first , callbacks1.begin()->first );
        int k1 = max(callbacks0.rbegin()->first, callbacks1.rbegin()->first);
        for (int i=k0; i<=k1; i++) {
            if (callbacks0.count(i)) {
                if (!trigger<Event>(callbacks0[i], event)) break;
            }
            if (callbacks1.count(i)) {
                if (!other->trigger<Event>(callbacks1[i], event)) break;
            }
        }
    }
    else if (callbacks0.size() > 0) {
        for (auto& prio : callbacks0) {
            bool abort = !trigger(prio.second, event);
            if (abort) return false;
        }
    }
    else if (callbacks1.size() > 0) {
        for (auto& prio : callbacks1) {
            bool abort = !other->trigger(prio.second, event);
            if (abort) return false;
        }
    }

    return true;
}


OSG_END_NAMESPACE

#endif // VRSIGNALT_H_INCLUDED
