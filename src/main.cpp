#include "core/objects/VRTransform.h"
#include "PolyVR.h"

int main(int argc, char **argv) {
    auto pvr = OSG::PolyVR::get();
	pvr->init(argc,argv);
    pvr->start();
    pvr->shutdown();
	return 0;
}
