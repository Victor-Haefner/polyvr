#include "StreetJoint.h"

using namespace OSG;

StreetJoint::StreetJoint(Vec2f position, string id) {
    this->position = position;
    this->id = id;
}

void StreetJoint::merge(StreetJoint* streetJoint) {
    for (auto seg : streetJoint->segments) {
        if (find(segments.begin(), segments.end(), seg) == segments.end()) {
            segments.push_back(seg);
        }
    }

    info = info + " +merge(" + streetJoint->info + ")";
    resetCaches();
}

// call this, when joint needs recalculation
void StreetJoint::resetCaches() {
    calcSegPoints_ = false;
    jointPointCache_.clear();
}
