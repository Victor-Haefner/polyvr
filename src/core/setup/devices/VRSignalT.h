#ifndef VRSIGNALT_H_INCLUDED
#define VRSIGNALT_H_INCLUDED

#include "VRSignal.h"

OSG_BEGIN_NAMESPACE;

template<typename Event>
bool VRSignal::triggerPtr(shared_ptr<Event> event) {
    if (!event && this->event) event = ((Event*)this->event)->ptr();
    auto callbacks = callbacksPtr; // make copy to avoid corruption while iterating!
    bool abort = false;

    for (auto& prio : callbacks) {
        for (auto& c : prio.second) {
            if (auto spc = c.lock()) {
                //( (VRFunction<Event*>*)spc.get() )(event);
                auto cb = (VRFunction< weak_ptr<Event>, bool >*)spc.get();
                abort = !(*cb)(event);
                if (abort) break;
            }
        }
        if (abort) break;
    }
    return !abort;
}

OSG_END_NAMESPACE

#endif // VRSIGNALT_H_INCLUDED
