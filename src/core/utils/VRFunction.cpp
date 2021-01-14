#include "VRFunction.h"
#include "VRProfiler.h"

#include <boost/exception/diagnostic_information.hpp>

void VRFunction_base::t0() {
#ifndef WASM
    prof_id = VRProfiler::get()->regStart(name);
#endif
}

void VRFunction_base::t1() {
#ifndef WASM
    VRProfiler::get()->regStop(prof_id);
#endif
}

void VRFunction_base::printExcept(boost::exception& e) {
    cout << "VRFunction::() exception occured: " << boost::diagnostic_information(e) << endl;
}
