#ifndef VRSIGNAL_H_INCLUDED
#define VRSIGNAL_H_INCLUDED

#include "core/utils/VRName.h"
#include "core/utils/VRFunctionFwd.h"
#include "core/utils/VRDeviceFwd.h"
#include <OpenSG/OSGConfig.h>
#include <vector>
#include <memory>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRSignal_base : public VRName {
    protected:
        vector<VRFunction_base*> callbacks;
        vector<VRBaseWeakCb> callbacksPtr;
        VRUpdatePtr trig_fkt = 0;
        bool _doUpdate = false;

    public:
        VRSignal_base();
        virtual ~VRSignal_base();

        void setUpdate(bool b);
        bool doUpdate();

        VRUpdatePtr getTriggerFkt();
        vector<VRBaseWeakCb> getCallbacks();

        void clear();
};

class VRSignal : public VRSignal_base {
    private:
        void* event = 0;

    public:
        VRSignal(VRDevice* dev = 0);
        ~VRSignal();

        static VRSignalPtr create(VRDevice* dev = 0);

        void add(VRFunction_base* fkt);
        void add(VRBaseWeakCb fkt);
        void sub(VRFunction_base* fkt);
        void sub(VRBaseWeakCb fkt);
        template<typename Event> void trigger(Event* event = 0) {
            if (event == 0) event = (Event*)this->event;
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
};

OSG_END_NAMESPACE

#endif // VRSIGNAL_H_INCLUDED
