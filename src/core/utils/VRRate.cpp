#include "VRRate.h"
#include <iostream>
#include <time.h>

using namespace std;


VRRate::VRRate() {
    count = 0;
    current = 60;
    time_0 = 0;
    time_1 = 0;
    print = true;
}

void VRRate::operator= (VRRate v) {;}

VRRate* VRRate::get() {
    static VRRate* singleton = new VRRate();
    return singleton;
}

void VRRate::setPrint(bool b) { print = b; }

int VRRate::getRate() {
    count++;
    time_1 = time(0);

    if (time_1 - time_0 > 0) {
        time_0 = time_1;
        current = count;
        count = 0;
    }
    return current;
}

VRRate::StatRate VRRate::statFPStime("statFPStime", "PolyVR Framerate");

