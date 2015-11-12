#include "StreetSegment.h"
#include "StreetJoint.h"

using namespace OSG;

StreetSegment::StreetSegment(StreetJoint* jA, StreetJoint* jB, float width, string id) {
    jointA = jA;
    jointB = jB;
    this->width = width;
    this->id = id;
}

void StreetSegment::setLeftPointFor(string jointId, Vec2f posLeft) {
    if (jointA->id == jointId) rightB = posLeft;
    else leftA = posLeft;
}

void StreetSegment::setLeftExtPointFor(string jointId, Vec2f posLeft) {
    if (jointA->id == jointId) leftExtB = posLeft;
    else leftExtA = posLeft;
}

void StreetSegment::setRightPointFor(string jointId, Vec2f posRight) {
    if (jointA->id == jointId) leftB = posRight;
    else rightA = posRight;
}

string StreetSegment::getOtherJointId(string jointId) {
    return jointA->id == jointId ? jointB->id : jointA->id;
}

float StreetSegment::getDistance() { return (leftA-leftB).length(); }
