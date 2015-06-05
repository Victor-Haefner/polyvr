#ifndef VRSIGNAL_H_INCLUDED
#define VRSIGNAL_H_INCLUDED

#include "core/utils/VRName.h"
#include <OpenSG/OSGConfig.h>
#include <vector>

class VRFunction_base;
template<class T> class VRFunction;

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRDevice;
typedef VRFunction<VRDevice*> VRDevCb;

class VRSignal_base : public VRName {
    protected:
        vector<VRFunction_base*> callbacks;
        VRFunction<int>* trig_fkt = 0;
        bool _doUpdate = false;

    public:
        VRSignal_base();
        virtual ~VRSignal_base();

        void setUpdate(bool b);
        bool doUpdate();

        VRFunction<int>* getTriggerFkt();

        void clear();
};

class VRSignal : public VRSignal_base {
    private:
        void* event = 0;

    public:
        VRSignal(VRDevice* dev = 0);
        ~VRSignal();

        void add(VRFunction_base* fkt);
        void sub(VRFunction_base* fkt);
        template<typename Event> void trigger(Event* event = 0) {
            if (event == 0) event = (Event*)this->event;
            for (auto c : callbacks) {
                auto cb = (VRFunction<Event*>*)c;
                (*cb)(event);
            }
        }
};

OSG_END_NAMESPACE

#endif // VRSIGNAL_H_INCLUDED
