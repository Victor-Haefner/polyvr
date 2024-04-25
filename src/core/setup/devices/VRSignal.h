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
        map<int, vector<VRBaseCbWeakPtr>> callbacksPtr;
        map<int, vector<VRBaseCbWeakPtr>> callbacksDeferredPtr;
        VRUpdateCbPtr trig_fkt = 0;
        bool _doUpdate = false;

    public:
        VRSignal_base();
        virtual ~VRSignal_base();

        void setUpdate(bool b);
        bool doUpdate();

        VRUpdateCbPtr getTriggerFkt();
        map<int, vector<VRBaseCbWeakPtr>> getCallbacks();
        map<int, vector<VRBaseCbWeakPtr>> getDeferredCallbacks();

        void clear();
};

class VRSignal : public VRSignal_base {
    private:
        void* event = 0;

    public:
        VRSignal(VRDevicePtr dev = 0);
        ~VRSignal();

        static VRSignalPtr create(VRDevicePtr dev = 0);

        void add(VRBaseCbWeakPtr fkt, int priority = 0, bool deferred = false); // lower priority comes first
        void sub(VRBaseCbWeakPtr fkt);

        template<typename Event> bool trigger(vector<VRBaseCbWeakPtr>& callbacks, std::shared_ptr<Event> event = 0);
        template<typename Event> bool triggerAll(std::shared_ptr<Event> event = 0);
        template<typename Event> bool triggerAllDeferred(std::shared_ptr<Event> event = 0);
        template<typename Event> bool triggerAllInterleaved(VRSignalPtr other, std::shared_ptr<Event> event = 0);
        template<typename Event> bool triggerAllInterleavedDeferred(VRSignalPtr other, std::shared_ptr<Event> event = 0);
};

OSG_END_NAMESPACE

#endif // VRSIGNAL_H_INCLUDED
