#include "VRSignal.h"

#include <boost/bind.hpp>
#include <OpenSG/OSGNode.h>
#include "core/utils/VRFunction.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

VRSignal_base::VRSignal_base() {
    ;
}

VRSignal_base::~VRSignal_base() {
    delete trig_fkt;
}

void VRSignal_base::clear() { callbacks.clear(); }

void VRSignal_base::setUpdate(bool b) { _doUpdate = b; }
bool VRSignal_base::doUpdate() { return _doUpdate; }

VRFunction<int>* VRSignal_base::getTriggerFkt() { return trig_fkt; }




VRSignal::VRSignal(VRDevice* _dev) : event(_dev) {
    trig_fkt = new VRFunction<int>("Signal_trigger", boost::bind(&VRSignal::trigger<VRDevice>, this, (VRDevice*)0));
}

VRSignal::~VRSignal() {
    ;
}

void VRSignal::add(VRFunction_base* fkt) { callbacks.push_back(fkt); }
void VRSignal::add(VRFunctionPtr fkt) { callbacksPtr.push_back(fkt); }

void VRSignal::sub(VRFunction_base* fkt) {
    if (callbacks.size() == 0) return;
    auto pos = find(callbacks.begin(), callbacks.end(), fkt);
    if (pos != callbacks.end()) callbacks.erase(pos);
}

void VRSignal::sub(VRFunctionPtr fkt) {
    if (callbacksPtr.size() == 0) return;
    auto sp = fkt.lock();
    for (int i=0; i<callbacksPtr.size(); i++) {
        auto sp2 = callbacksPtr[i].lock();
        if (sp && sp2 ? sp == sp2 : false) callbacksPtr.erase(callbacksPtr.begin()+i);
    }
}

OSG_END_NAMESPACE
