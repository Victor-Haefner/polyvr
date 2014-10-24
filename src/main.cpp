#include "PolyVR.h"
#include <sys/resource.h>
#include <iostream>

OSG_USING_NAMESPACE;
using namespace std;

int main(int argc, char **argv) {
    // remove possible core file
    remove("./core");

    // Enable core dumps
    struct rlimit corelim;
    corelim.rlim_cur = -1;
    corelim.rlim_max = -1;
    if (setrlimit (RLIMIT_CORE, &corelim) != 0) cerr << "Couldn't set core limit\n";

	initPolyVR(argc,argv);
    startPolyVR();
	return 0;
}
