#include "StreetSegment.h"

using namespace OSG;

StreetSegment::StreetSegment(string streetJointA_ID, string streetJointB_ID, float width, string id) {
    this->streetJointA_ID = streetJointA_ID;
    this->streetJointB_ID = streetJointB_ID;
    this->width = width;
    this->id = id;
}

void StreetSegment::setLeftPointFor(string jointId, Vec2f posLeft) {
    if (streetJointA_ID == jointId) rightB = posLeft;
    else leftA = posLeft;
}

void StreetSegment::setLeftExtPointFor(string jointId, Vec2f posLeft) {
    if (streetJointA_ID == jointId) leftExtB = posLeft;
    else leftExtA = posLeft;
}

void StreetSegment::setRightPointFor(string jointId, Vec2f posRight) {
    if (streetJointA_ID == jointId) leftB = posRight;
    else rightA = posRight;
}

string StreetSegment::getOtherJointId(string jointId) {
    return streetJointA_ID == jointId ? streetJointB_ID : streetJointA_ID;
}

float StreetSegment::getDistance() { return (leftA-leftB).length(); }
