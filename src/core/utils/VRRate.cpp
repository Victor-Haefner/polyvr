#include "VRRate.h"
#include <iostream>
#include <time.h>

using namespace std;


VRRate::VRRate() {}
VRRate::~VRRate() {}

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

VRRate::StatRate VRRate::statFPS("statFPS", "PolyVR framerate");
VRRate::StatRate VRRate::statScriptFPS("statScriptFPS", "PolyVR scripts framerate");
VRRate::StatRate VRRate::statPhysFPS("statPhysFPS", "PolyVR physics framerate");

