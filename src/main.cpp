#include "PolyVR.h"
#include "core/utils/VROptions.h"

int main(int argc, char **argv) {
    //VROptions::get()->setOption("stand_alone", true);
	OSG::initPolyVR(argc,argv);
    OSG::startPolyVR();
    //OSG::exitPolyVR();

	return 0;
}
