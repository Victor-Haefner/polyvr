#include "PolyVR.h"

int main(int argc, char **argv) {
    auto pvr = OSG::PolyVR::get();
	pvr->init(argc,argv);

    //pvr->setOption("standalone", true);

    pvr->start();
    pvr->shutdown();

	return 0;
}
