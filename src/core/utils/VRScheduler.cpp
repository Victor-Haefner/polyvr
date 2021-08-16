#include "VRScheduler.h"

using namespace OSG;

VRScheduler::VRScheduler() {}
VRScheduler::~VRScheduler() {}

VRSchedulerPtr VRScheduler::create() { return VRSchedulerPtr( new VRScheduler() ); }
VRSchedulerPtr VRScheduler::ptr() { return static_pointer_cast<VRScheduler>(shared_from_this()); }

void VRScheduler::postpone(function<void()> f) { postponed.push_back(f); }
size_t VRScheduler::getNPostponed() { return postponed.size(); }

void VRScheduler::callPostponed(bool recall, int limit) {
    if (recall) {
        int N = 0;
        while (postponed.size()) {
            auto tocall = postponed; // make a copy, because a function might add another postponed..
            postponed.clear();
            for (auto& f : tocall) f();
            N++;
            if (N >= limit) break;
        }
    } else {
        auto tocall = postponed; // make a copy, because a function might add another postponed..
        postponed.clear();
        for (auto& f : tocall) f();
    }
}
