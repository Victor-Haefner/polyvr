#include "VRFunction.h"
#include "VRProfiler.h"

void VRFunction_base::t0() {
    prof_id = VRProfiler::get()->regStart(name);
}

void VRFunction_base::t1() {
    VRProfiler::get()->regStop(prof_id);
}
