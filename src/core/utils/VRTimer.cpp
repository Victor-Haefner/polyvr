#include "VRTimer.h"
#include <GL/glut.h>
#include <iostream>

using namespace std;

VRTimer::VRTimer() { start(); }

void VRTimer::start() { single.start = glutGet(GLUT_ELAPSED_TIME); }
int VRTimer::stop() { return glutGet(GLUT_ELAPSED_TIME) - single.start; }

map<string, VRTimer::timer> VRTimer::beacons = map<string, VRTimer::timer>();

void VRTimer::start(string t) {
    if (timers.count(t) == 0) timers[t] = timer();
    timers[t].start = glutGet(GLUT_ELAPSED_TIME);
}

int VRTimer::stop(string t) {
    timers[t].total += glutGet(GLUT_ELAPSED_TIME) - timers[t].start;
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
        int t = glutGet(GLUT_ELAPSED_TIME);
        cout << "time beacon " << b << ": " << t - beacons[b].start << endl;
        beacons[b].start = t;
    }
}

int VRTimer::getBeacon(string b) {
    if (!beacons.count(b)) beacons[b] = timer();
    int t = glutGet(GLUT_ELAPSED_TIME);
    int res = t - beacons[b].start;
    beacons[b].start = t;
    return res;
}
