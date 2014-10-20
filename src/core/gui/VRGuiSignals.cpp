#include "VRGuiSignals.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

VRGuiSignals* VRGuiSignals::get() {
    static VRGuiSignals* singleton = new VRGuiSignals();
    return singleton;
}

VRGuiSignals::VRGuiSignals() {}

VRSignal* VRGuiSignals::getSignal(string name) {
    if (signals.count(name) == 0) signals[name] = new VRSignal();
    return signals[name];
}

OSG_END_NAMESPACE
