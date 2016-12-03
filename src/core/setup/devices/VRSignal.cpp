#include "VRSignal.h"

#include "VRDevice.h"
#include "VRSignalT.h"

#include <boost/bind.hpp>
#include <OpenSG/OSGNode.h>
#include "core/utils/VRFunction.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

VRSignal_base::VRSignal_base() {}
VRSignal_base::~VRSignal_base() {}

void VRSignal_base::clear() { callbacks.clear(); callbacksPtr.clear(); }

void VRSignal_base::setUpdate(bool b) { _doUpdate = b; }
bool VRSignal_base::doUpdate() { return _doUpdate; }

VRUpdateCbPtr VRSignal_base::getTriggerFkt() { return trig_fkt; }
vector<VRBaseCbWeakPtr> VRSignal_base::getCallbacks() { return callbacksPtr; }


VRSignal::VRSignal(VRDevicePtr _dev) : event(_dev.get()) {
    VRDevicePtr data;
    trig_fkt = VRFunction<int>::create("Signal_trigger", boost::bind(&VRSignal::triggerPtr<VRDevice>, this, data));
}

VRSignal::~VRSignal() {}

VRSignalPtr VRSignal::create(VRDevicePtr dev) { return VRSignalPtr( new VRSignal(dev) ); }

void VRSignal::add(VRFunction_base* fkt) { callbacks.push_back(fkt); }
void VRSignal::add(VRBaseCbWeakPtr fkt) { callbacksPtr.push_back(fkt); }

void VRSignal::sub(VRFunction_base* fkt) {
    if (callbacks.size() == 0) return;
    auto pos = find(callbacks.begin(), callbacks.end(), fkt);
    if (pos != callbacks.end()) callbacks.erase(pos);
}

void VRSignal::sub(VRBaseCbWeakPtr fkt) {
    if (callbacksPtr.size() == 0) return;
    auto sp = fkt.lock();
    if (!sp) return;
    vector<int> toErase;
    for (uint i=0; i<callbacksPtr.size(); i++) {
        auto wp = callbacksPtr[i];
        auto sp2 = wp.lock();
        if (!sp2 || sp == sp2) toErase.push_back(i);
    }
    for (int i = toErase.size()-1; i>=0; i--) callbacksPtr.erase(callbacksPtr.begin()+toErase[i]);
}

OSG_END_NAMESPACE
