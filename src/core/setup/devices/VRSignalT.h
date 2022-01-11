#ifndef VRSIGNALT_H_INCLUDED
#define VRSIGNALT_H_INCLUDED

#include "VRSignal.h"

OSG_BEGIN_NAMESPACE;

template<typename Event>
bool VRSignal::trigger(vector<VRBaseCbWeakPtr>& callbacks, shared_ptr<Event> event) {
    if (!event && this->event) event = ((Event*)this->event)->ptr();

    for (auto& c : callbacks) {
        if (auto spc = c.lock()) {
            //( (VRFunction<Event*>*)spc.get() )(event);
            auto cb = (VRFunction< weak_ptr<Event>, bool >*)spc.get();
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

    return true;
}

OSG_END_NAMESPACE

#endif // VRSIGNALT_H_INCLUDED
