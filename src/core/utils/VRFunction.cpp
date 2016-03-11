#include "VRFunction.h"
#include "VRProfiler.h"

#include <boost/exception/diagnostic_information.hpp>

void VRFunction_base::t0() {
    prof_id = VRProfiler::get()->regStart(name);
}

void VRFunction_base::t1() {
    VRProfiler::get()->regStop(prof_id);
}

void VRFunction_base::printExcept(boost::exception& e) {
    cout << "VRFunction::() exception occured: " << boost::diagnostic_information(e) << endl;
}
