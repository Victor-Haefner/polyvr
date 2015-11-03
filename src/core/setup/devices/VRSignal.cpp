#include "VRSignal.h"

#include <boost/bind.hpp>
#include <OpenSG/OSGNode.h>
#include "core/utils/VRFunction.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

VRSignal_base::VRSignal_base() {}

VRSignal_base::~VRSignal_base() {}

void VRSignal_base::clear() { callbacks.clear(); }

void VRSignal_base::setUpdate(bool b) { _doUpdate = b; }
bool VRSignal_base::doUpdate() { return _doUpdate; }

VRUpdatePtr VRSignal_base::getTriggerFkt() { return trig_fkt; }


VRSignal::VRSignal(VRDevice* _dev) : event(_dev) {
    trig_fkt = VRFunction<int>::create("Signal_trigger", boost::bind(&VRSignal::trigger<VRDevice>, this, (VRDevice*)0));
}

VRSignal::~VRSignal() {}

VRSignalPtr VRSignal::create(VRDevice* dev) { return VRSignalPtr( new VRSignal(dev) ); }

void VRSignal::add(VRFunction_base* fkt) { callbacks.push_back(fkt); }
void VRSignal::add(VRBaseWeakCb fkt) { callbacksPtr.push_back(fkt); }

void VRSignal::sub(VRFunction_base* fkt) {
    if (callbacks.size() == 0) return;
    auto pos = find(callbacks.begin(), callbacks.end(), fkt);
    if (pos != callbacks.end()) callbacks.erase(pos);
}

void VRSignal::sub(VRBaseWeakCb fkt) {
    if (callbacksPtr.size() == 0) return;
    auto sp = fkt.lock();
    if (!sp) return;
    vector<int> toErase;
    for (int i=0; i<callbacksPtr.size(); i++) {
        auto wp = callbacksPtr[i];
        auto sp2 = wp.lock();
        if (!sp2 || sp == sp2) toErase.push_back(i);
    }
    for (int i = toErase.size()-1; i>=0; i--) callbacksPtr.erase(callbacksPtr.begin()+toErase[i]);
}

OSG_END_NAMESPACE
