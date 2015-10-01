#include "PolyVR.h"
#include "core/utils/VROptions.h"

int main(int argc, char **argv) {
    auto pvr = OSG::PolyVR::get();

    //VROptions::get()->setOption("stand_alone", true);
	pvr.init(argc,argv);
    pvr.start();
    //pvr.exit();

	return 0;
}
