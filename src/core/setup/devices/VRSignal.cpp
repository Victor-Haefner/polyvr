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




VRSignal::VRSignal(VRDevice* _dev) : dev(_dev) {
    trig_fkt = new VRFunction<int>("Signal_trigger", boost::bind(&VRSignal::trigger, this));
}

VRSignal::~VRSignal() {
    ;
}

void VRSignal::add(VRDevCb* fkt) {
    callbacks.push_back(fkt);
}

void VRSignal::sub(VRDevCb* fkt) {
    if (callbacks.size() == 0) return;
    auto pos = find(callbacks.begin(), callbacks.end(), fkt);
    if (pos != callbacks.end()) callbacks.erase(pos);
    //callbacks.erase(remove(callbacks.begin(), callbacks.end(), fkt), callbacks.end());
}

void VRSignal::trigger() {
    for (auto c : callbacks) {
        auto cb = (VRDevCb*)c;
        (*cb)(dev);
    }
}

OSG_END_NAMESPACE
