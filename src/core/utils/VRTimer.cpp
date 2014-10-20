#include "VRTimer.h"
#include <GL/glut.h>
#include <iostream>

using namespace std;

void VRTimer::start(string t) {
    if (timers.count(t) == 0) timers[t] = timer();
    timers[t].start = glutGet(GLUT_ELAPSED_TIME);
}

void VRTimer::stop(string t) {
    timers[t].total += glutGet(GLUT_ELAPSED_TIME) - timers[t].start;
}

void VRTimer::print() {
    cout << "\nProfiling results:";
    map<string, timer>::iterator itr;
    for(itr = timers.begin(); itr != timers.end(); itr++) cout << "\n  " << itr->first << " : " << itr->second.total << flush;
    cout << endl << endl;
    timers.clear();
}
