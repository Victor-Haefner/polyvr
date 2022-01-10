#include "VRSignal.h"

#include "VRDevice.h"
#include "VRSignalT.h"

#include <OpenSG/OSGNode.h>
#include "core/utils/VRFunction.h"
#include "core/utils/toString.h"

using namespace OSG;


VRSignal_base::VRSignal_base() {}
VRSignal_base::~VRSignal_base() {}

void VRSignal_base::clear() { callbacksPtr.clear(); }

void VRSignal_base::setUpdate(bool b) { _doUpdate = b; }
bool VRSignal_base::doUpdate() { return _doUpdate; }

VRUpdateCbPtr VRSignal_base::getTriggerFkt() { return trig_fkt; }
map<int, vector<VRBaseCbWeakPtr>> VRSignal_base::getCallbacks() { return callbacksPtr; }


VRSignal::VRSignal(VRDevicePtr _dev) : event(_dev.get()) {
    VRDevicePtr data;
    trig_fkt = VRUpdateCb::create("Signal_trigger", bind(&VRSignal::triggerPtr<VRDevice>, this, data));
}

VRSignal::~VRSignal() {}

VRSignalPtr VRSignal::create(VRDevicePtr dev) { return VRSignalPtr( new VRSignal(dev) ); }

void VRSignal::add(VRBaseCbWeakPtr fkt, int priority) { callbacksPtr[priority].push_back(fkt); }

void VRSignal::sub(VRBaseCbWeakPtr fkt) {
    if (callbacksPtr.size() == 0) return;
    auto sp = fkt.lock();
    if (!sp) return;

    for (auto& prio : callbacksPtr) {
        auto& vec = prio.second;
        if (vec.size() == 0) continue;

        vector<int> toErase;
        for (unsigned int i=0; i<vec.size(); i++) {
            auto wp = vec[i];
            auto sp2 = wp.lock();
            if (!sp2 || sp == sp2) toErase.push_back(i);
        }

        for (int i = toErase.size()-1; i>=0; i--) vec.erase(vec.begin()+toErase[i]);
    }
}

