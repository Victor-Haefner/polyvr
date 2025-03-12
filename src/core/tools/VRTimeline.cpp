#include "VRTimeline.h"

using namespace OSG;

VRTimeline::VRTimeline() {}
VRTimeline::~VRTimeline() {}

VRTimelinePtr VRTimeline::create() { return VRTimelinePtr( new VRTimeline() ); }
VRTimelinePtr VRTimeline::ptr() { return static_pointer_cast<VRTimeline>(shared_from_this()); }

void VRTimeline::addCallback(double t1, double t2, VRAnimCbPtr cb) {
    ;
}

void VRTimeline::addTimeline(double t1, double t2, VRTimelinePtr tl) {
    ;
}

void VRTimeline::setTime(double t) {
    ;
}
