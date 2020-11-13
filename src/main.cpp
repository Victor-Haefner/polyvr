/*#include "PolyVR.h"

#include <iostream>

int main(int argc, char **argv) {
    auto pvr = OSG::PolyVR::create();
	pvr->init(argc,argv);
	pvr->run();
	pvr.reset();
	std::cout << "PolyVR main returns" << std::endl;
}*/

#include "core/tests/viveTest1.cpp"


int main(int argc, char** argv) {
	startViveTest1(argc, argv);
}

