#include "StreetJoint.h"

using namespace OSG;

StreetJoint::StreetJoint(Vec2f position, string id) {
    this->position = position;
    this->id = id;
}

void StreetJoint::merge(StreetJoint* streetJoint) {
    for (string segId : streetJoint->segmentIds) {
        if (find(segmentIds.begin(), segmentIds.end(), segId) == segmentIds.end()) {
            segmentIds.push_back(segId);
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
