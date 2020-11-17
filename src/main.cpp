#include "PolyVR.h"

#include <iostream>

int main(int argc, char **argv) {
    auto pvr = OSG::PolyVR::create();
	pvr->init(argc,argv);
	pvr->run();
	pvr.reset();
	std::cout << "PolyVR main returns" << std::endl;
}

