#include "VRGuiSignals.h"
#include "core/setup/devices/VRSignal.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

VRGuiSignals* VRGuiSignals::get() {
    static VRGuiSignals* singleton = new VRGuiSignals();
    return singleton;
}

VRGuiSignals::VRGuiSignals() {}

VRSignalPtr VRGuiSignals::getSignal(string name) {
    if (signals.count(name) == 0) signals[name] = VRSignal::create();
    return signals[name];
}

vector<string> VRGuiSignals::getSignals() {
    vector<string> res;
    for (auto s : signals) res.push_back(s.first);
    return res;
}

void VRGuiSignals::clear() {
    signals.clear();
}

OSG_END_NAMESPACE
