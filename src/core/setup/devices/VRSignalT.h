#ifndef VRSIGNALT_H_INCLUDED
#define VRSIGNALT_H_INCLUDED

#include "VRSignal.h"

OSG_BEGIN_NAMESPACE;

template<typename Event>
void VRSignal::trigger(Event* event) {
    if (!event) event = (Event*)this->event;
    auto callbacks = callbacksPtr; // make copy to avoid corruption while iterating!
    for (auto c : callbacks) {
        if (auto spc = c.lock()) {
            //( (VRFunction<Event*>*)spc.get() )(event);
            auto cb = (VRFunction<Event*>*)spc.get();
            (*cb)(event);
        }
    }
}

template<typename Event>
void VRSignal::triggerPtr(shared_ptr<Event> event) {
    if (!event && this->event) event = ((Event*)this->event)->ptr();
    auto callbacks = callbacksPtr; // make copy to avoid corruption while iterating!
    for (auto c : callbacks) {
        if (auto spc = c.lock()) {
            //( (VRFunction<Event*>*)spc.get() )(event);
            auto cb = (VRFunction< weak_ptr<Event> >*)spc.get();
            (*cb)(event);
        }
    }
}

OSG_END_NAMESPACE

#endif // VRSIGNALT_H_INCLUDED
