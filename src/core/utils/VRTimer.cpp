#include "VRTimer.h"
#include "core/utils/system/VRSystem.h"
#include <iostream>

using namespace std;

VRTimer::VRTimer() { start(); }

void VRTimer::start() { single.start = getTime()*1e-3; }
double VRTimer::stop() { return getTime()*1e-3- single.start; }
void VRTimer::reset() { start(); }

map<string, VRTimer::timer> VRTimer::beacons = map<string, VRTimer::timer>();

void VRTimer::start(string t) {
    if (timers.count(t) == 0) timers[t] = timer();
    timers[t].start = getTime()*1e-3;
}

double VRTimer::stop(string t) {
    timers[t].total += getTime()*1e-3 - timers[t].start;
    return timers[t].total;
}

void VRTimer::print() {
    cout << "\nProfiling results:";
    map<string, timer>::iterator itr;
    for(itr = timers.begin(); itr != timers.end(); itr++) cout << "\n  " << itr->first << " : " << itr->second.total << flush;
    cout << endl << endl;
    timers.clear();
}

void VRTimer::emitBeacon(string b) {
    if (!beacons.count(b)) beacons[b] = timer();
    else {
        int t = getTime()*1e-3;
        cout << "time beacon " << b << ": " << t - beacons[b].start << endl;
        beacons[b].start = t;
    }
}

double VRTimer::getBeacon(string b) {
    if (!beacons.count(b)) beacons[b] = timer();
    double t = getTime()*1e-3;
    double res = t - beacons[b].start;
    beacons[b].start = t;
    return res;
}
