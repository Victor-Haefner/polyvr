#include "VRAxleSegmentation.h"
#include "core/utils/toString.h"

using namespace OSG;

template<> string typeName(const VRAxleSegmentation& m) { return "AxleSegmentation"; }

VRAxleSegmentation::VRAxleSegmentation() {}
VRAxleSegmentation::~VRAxleSegmentation() {}

VRAxleSegmentationPtr VRAxleSegmentation::create() { return VRAxleSegmentationPtr(new VRAxleSegmentation()); }


void VRAxleSegmentation::analyse(VRObjectPtr o) {
    obj = o;
}
