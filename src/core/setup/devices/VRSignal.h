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
        vector<VRBaseCbWeakPtr> callbacksPtr;
        VRUpdateCbPtr trig_fkt = 0;
        bool _doUpdate = false;

    public:
        VRSignal_base();
        virtual ~VRSignal_base();

        void setUpdate(bool b);
        bool doUpdate();

        VRUpdateCbPtr getTriggerFkt();
        vector<VRBaseCbWeakPtr> getCallbacks();

        void clear();
};

class VRSignal : public VRSignal_base {
    private:
        void* event = 0;

    public:
        VRSignal(VRDevicePtr dev = 0);
        ~VRSignal();

        static VRSignalPtr create(VRDevicePtr dev = 0);

        void add(VRBaseCbWeakPtr fkt);
        void sub(VRBaseCbWeakPtr fkt);
        template<typename Event> void trigger(Event* event = 0);
        template<typename Event> void triggerPtr(std::shared_ptr<Event> event = 0);
};

OSG_END_NAMESPACE

#endif // VRSIGNAL_H_INCLUDED
