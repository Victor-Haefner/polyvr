#include "VRTimeline.h"
#include "core/utils/VRFunction.h"

using namespace OSG;

double clamp(double f, double a = -1, double b = 1);

VRTimeline::SubTime::SubTime(double t1, double t2) : t1(t1), t2(t2) {
    T = t2 - t1;
}

bool VRTimeline::SubTime::before(double t) { return bool(t < t1); }
bool VRTimeline::SubTime::after(double t) { return bool(t > t2); }
bool VRTimeline::SubTime::active(double t) { return bool(t >= t1 && t <= t2); }

double VRTimeline::SubTime::interp(double t) {
    return clamp( (t - t1) / T, 0.0, 1.0 );
}

double VRTimeline::SubTime::convert(double t) {
    return t1 + t*T;
}


template<> void VRTimeline::Entry<VRTimelinePtr>::update(double t) { obj->setTime( subTime.interp(t) ); }
template<> void VRTimeline::Entry<VRAnimCbPtr>::update(double t) { (*obj)( subTime.interp(t) ); }


VRTimeline::VRTimeline() {}
VRTimeline::~VRTimeline() {}

VRTimelinePtr VRTimeline::create() { return VRTimelinePtr( new VRTimeline() ); }
VRTimelinePtr VRTimeline::ptr() { return static_pointer_cast<VRTimeline>(shared_from_this()); }

void VRTimeline::addCallback(double t1, double t2, VRAnimCbPtr cb) {
    callbacks.push_back( Entry<VRAnimCbPtr>(t1, t2, cb) );
}

void VRTimeline::addTimeline(double t1, double t2, VRTimelinePtr tl) {
    timelines.push_back( Entry<VRTimelinePtr>(t1, t2, tl) );
}

void VRTimeline::setTime(double t) {
    t = clamp(t, 0.0, 1.0);

    for (auto& e : callbacks) e.update(t);
    for (auto& e : timelines) e.update(t);
}

