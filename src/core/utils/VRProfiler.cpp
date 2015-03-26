#include "VRProfiler.h"

VRProfiler* VRProfiler::get() {
    static VRProfiler* instance = new VRProfiler();
    return instance;
}

VRProfiler::VRProfiler() {}
int VRProfiler::regStart(string name) {}
void VRProfiler::regStop(int ID) {}
