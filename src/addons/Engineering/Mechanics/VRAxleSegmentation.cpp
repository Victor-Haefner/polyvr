#include "VRAxleSegmentation.h"
#include "core/utils/toString.h"

using namespace OSG;

VRAxleSegmentation::VRAxleSegmentation() {}
VRAxleSegmentation::~VRAxleSegmentation() {}

VRAxleSegmentationPtr VRAxleSegmentation::create() { return VRAxleSegmentationPtr(new VRAxleSegmentation()); }


void VRAxleSegmentation::analyse(VRObjectPtr o) {
    obj = o;
}
