#include "PolyVR.h"
#ifndef _WIN32
#include <sys/resource.h>
#endif
#include <iostream>

int main(int argc, char **argv) {
#ifndef _WIN32
    // remove possible core file
    remove("./core");

    // Enable core dumps
    struct rlimit corelim;
    corelim.rlim_cur = -1;
    corelim.rlim_max = -1;
    if (setrlimit (RLIMIT_CORE, &corelim) != 0) std::cerr << "Couldn't set core limit\n";
#endif

	OSG::initPolyVR(argc,argv);
    OSG::startPolyVR();

	return 0;
}
