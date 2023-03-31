#include "VRGuiSignals.h"
#include "core/setup/devices/VRSignal.h"

#include <iostream>

using namespace std;
using namespace OSG;

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

void VRGuiSignals::addCallback(string name, Callback c) { callbacks[name].push_back(c); }
void VRGuiSignals::addResizeCallback(string name, ResizeCallback c) { resizeCallbacks[name].push_back(c); }

bool VRGuiSignals::trigger(string name, Options options) {
    if (!callbacks.count(name)) {
        //cout << " ..no callbacks, skip " << name << endl;
        return false;
    }

    for (auto& callback : callbacks[name]) {
        bool b = callback(options);
        if (!b) break;
    }

    return true;
}

bool VRGuiSignals::triggerResize(string name, int x, int y, int w, int h) {
    if (!resizeCallbacks.count(name)) {
        //cout << " ..no resize callbacks, skip " << name << endl;
        return false;
    }

    for (auto& callback : resizeCallbacks[name]) {
        bool b = callback(x,y,w,h);
        if (!b) break;
    }

    return true;
}

