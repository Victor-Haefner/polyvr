#ifndef VRSIGNALT_H_INCLUDED
#define VRSIGNALT_H_INCLUDED

#include "VRSignal.h"

OSG_BEGIN_NAMESPACE;

template<typename Event>
void VRSignal::trigger(Event* event) {
    if (!event) event = (Event*)this->event;
    for (auto c : callbacks) {
        auto cb = (VRFunction<Event*>*)c;
        (*cb)(event);
    }
    for (auto c : callbacksPtr) {
        if (auto spc = c.lock()) {
            //( (VRFunction<Event*>*)spc.get() )(event);
            auto cb = (VRFunction<Event*>*)spc.get();
            (*cb)(event);
        }
    }
}

template<typename Event>
void VRSignal::triggerPtr(shared_ptr<Event> event) {
    if (!event and this->event) event = ((Event*)this->event)->ptr();
    if (callbacks.size() == 0 && callbacksPtr.size() == 0) return;

    for (auto c : callbacks) {
        auto cb = (VRFunction< weak_ptr<Event> >*)c;
        (*cb)(event);
    }

    for (auto c : callbacksPtr) {
        if (auto spc = c.lock()) {
            //( (VRFunction<Event*>*)spc.get() )(event);
            auto cb = (VRFunction< weak_ptr<Event> >*)spc.get();
            (*cb)(event);
        }
    }
}

OSG_END_NAMESPACE

#endif // VRSIGNALT_H_INCLUDED
