#include "StreetAlgos.h"
#include "Modules/StreetSegment.h"
#include "Modules/StreetJoint.h"
#include "Vec2Helper.h"

using namespace OSG;
using namespace std;
using namespace realworld;

namespace realworld {
    struct StreetBorder {
        string streetSegmentId;
        Vec2f start;
        Vec2f end;

        StreetBorder(string streetSegmentId, Vec2f start, Vec2f end);
        Vec2f dir();
    };
}

StreetBorder::StreetBorder(string streetSegmentId, Vec2f start, Vec2f end) {
    this->streetSegmentId = streetSegmentId;
    this->start = start;
    this->end = end;
}

Vec2f StreetBorder::dir() {
    Vec2f result = this->end - this->start;
    result.normalize();
    return result;
}


/** returns start && end point of the left border of a street segment */
StreetBorder* StreetAlgos::segmentGetLeftBorderTo(StreetSegment* seg, string jointId, map<string, StreetJoint*> streetJoints) {
    StreetJoint* joint = streetJoints[jointId];
    StreetJoint* oJoint = streetJoints[seg->getOtherJointId(jointId)];
    Vec2f posStart = oJoint->position;
    Vec2f posEnd = joint->position;
    Vec2f d = posEnd - posStart;
    d.normalize();
    d = d * (seg->width * 0.5f);
    Vec2f dOrtho = Vec2f(d[1], -d[0]);
    return new StreetBorder(seg->id, posStart+dOrtho, posEnd+dOrtho);
}

/** returns start && end point of the right border of a street segment */
StreetBorder* StreetAlgos::segmentGetRightBorderTo(StreetSegment* seg, string jointId, map<string, StreetJoint*> streetJoints) {
    StreetJoint* joint = streetJoints[jointId];
    StreetJoint* oJoint = streetJoints[seg->getOtherJointId(jointId)];
    Vec2f posStart = oJoint->position;
    Vec2f posEnd = joint->position;
    Vec2f d = posEnd - posStart;
    d.normalize();
    d = d * (seg->width * 0.5f);
    Vec2f dOrtho = Vec2f(d[1], -d[0]);
    return new StreetBorder(seg->id, posStart-dOrtho, posEnd-dOrtho);
}

/** returns imporant points to a joint for one street segment */
JointPoints* StreetAlgos::segmentGetPointsFor(StreetSegment* seg, string jointId) {
    JointPoints* result = new JointPoints();
    if (seg->streetJointA_ID == jointId) {
        result->left = seg->leftB;
        result->right = seg->rightB;
        result->leftExt = seg->leftExtB;
    } else {
        result->left = seg->leftA;
        result->right = seg->rightA;
        result->leftExt = seg->leftExtA;
    }
    return result;
}

/** rémoves duplicate names from a given vector */
void StreetAlgos::vectorStrRemoveDuplicates(vector<string> vec) {
    sort(vec.begin(), vec.end());
    vec.erase(unique(vec.begin(), vec.end()), vec.end());
}

/** calculates the important points of a given joint */
void StreetAlgos::calcSegments(StreetJoint* joint, map<string, StreetSegment*> streetSegments, map<string, StreetJoint*> streetJoints) {
    if (joint->calcSegPoints_) return;
    joint->calcSegPoints_ = true;

    vectorStrRemoveDuplicates(joint->segmentIds);
    jointOrderSegments(joint, streetSegments, streetJoints);

    Vec2f jCenter = joint->position;

    if (joint->segmentIds.size() == 1) { // special handling
        StreetSegment* seg = streetSegments[joint->segmentIds[0]];
        seg->setLeftPointFor(joint->id, segmentGetLeftBorderTo(seg, joint->id, streetJoints)->end);
        seg->setRightPointFor(joint->id, segmentGetRightBorderTo(seg, joint->id, streetJoints)->end);
        seg->setLeftExtPointFor(joint->id, joint->position + (segmentGetRightBorderTo(seg, joint->id, streetJoints)->dir() * (seg->width * 0.5f)));
        return;
    }

    float maxWidth = 0;
    for (string segId : joint->segmentIds) { //2-4 IDs
        StreetSegment* seg = streetSegments[segId];
        if (seg->width > maxWidth) maxWidth = seg->width;
    }
    maxWidth = maxWidth * 0.5f;

    for (unsigned int i=0; i<joint->segmentIds.size(); i++) {
        StreetSegment* s1 = streetSegments[joint->segmentIds[i]];
        StreetSegment* s2 = streetSegments[joint->segmentIds[(i+1) % joint->segmentIds.size()]];

        StreetBorder* s1BorderLeft = segmentGetLeftBorderTo(s1, joint->id, streetJoints);
        StreetBorder* s2BorderRight = segmentGetRightBorderTo(s2, joint->id, streetJoints);
        pair<bool, Vec2f> point = Vec2Helper::lineIntersectionPoint(s1BorderLeft->start, s1BorderLeft->dir(), s2BorderRight->start, s2BorderRight->dir());
        // calc length from border start to intersection
        float pDistS1 = (point.second - s1BorderLeft->start).length();
        float pDistS2 = (point.second - s2BorderRight->start).length();
        // calc length of border (minus the width of the street)
        float cDistS1 = (s1BorderLeft->end - s1BorderLeft->start).length() - maxWidth;
        float cDistS2 = (s2BorderRight->end - s2BorderRight->start).length() - maxWidth;

        if (pDistS1 < cDistS1 && pDistS2 < cDistS2 && point.first) {
            s1->setLeftPointFor(joint->id, point.second);
            s2->setRightPointFor(joint->id, point.second);
            //s1->setLeftExtPointFor(joint->id, p.second);
        } else {
            StreetJoint* s1_oJoint = streetJoints[s1->getOtherJointId(joint->id)];
            StreetJoint* s2_oJoint = streetJoints[s2->getOtherJointId(joint->id)];
            Vec2f leftMidPoint = s1_oJoint->position + (s1BorderLeft->dir() * cDistS1);
            Vec2f rightMidPoint = s2_oJoint->position + (s2BorderRight->dir() * cDistS2);
            if (cDistS1 > pDistS1) cDistS1 = pDistS1;
            if (cDistS2 > pDistS2) cDistS2 = pDistS2;
            Vec2f leftPoint = s1BorderLeft->start + (s1BorderLeft->dir() * cDistS1);
            Vec2f rightPoint = s2BorderRight->start + (s2BorderRight->dir() * cDistS2);

            s1->setLeftPointFor(joint->id, leftPoint);
            s2->setRightPointFor(joint->id, rightPoint);

            Vec2f dirLR = rightMidPoint-leftMidPoint;
            dirLR.normalize();
            Vec2f dirLR_ortho = Vec2f(dirLR[1], -dirLR[0]);
            pair<bool, float> intersectionLeft = Vec2Helper::lineIntersection(s1BorderLeft->start, s1BorderLeft->dir(), joint->position, dirLR_ortho);
            pair<bool, float> intersectionRight = Vec2Helper::lineIntersection(s2BorderRight->start, s2BorderRight->dir(), joint->position, dirLR_ortho);

            if (intersectionLeft.second > intersectionRight.second) {
                pair<bool, Vec2f> p2 = Vec2Helper::lineIntersectionPoint(s1BorderLeft->start, s1BorderLeft->dir(), joint->position, dirLR_ortho);
                s1->setLeftExtPointFor(joint->id, p2.second);
            } else {
                pair<bool, Vec2f> p2 = Vec2Helper::lineIntersectionPoint(s2BorderRight->start, s2BorderRight->dir(), joint->position, dirLR_ortho);
                s1->setLeftExtPointFor(joint->id, p2.second);
            }
        }
    }
}

/** calculates the important points to a given joint */
vector<JointPoints*> StreetAlgos::calcJoints(StreetJoint* joint, map<string, StreetSegment*> streetSegments, map<string, StreetJoint*> streetJoints) {
    if (joint->jointPointCache_.size() > 0) return joint->jointPointCache_;

    vector<JointPoints*> jointPoints;
    for(string segId : joint->segmentIds) {
        StreetSegment* seg = streetSegments[segId];

        // make borders the same length (use the smaller border length as reference)
        JointPoints* p = segmentGetPointsFor(seg, joint->id);

        StreetBorder* borderLeft = segmentGetLeftBorderTo(seg, joint->id, streetJoints);
        StreetBorder* borderRight = segmentGetRightBorderTo(seg, joint->id, streetJoints);

        float distLeft = (p->left - borderLeft->start).length();
        float distRight = (p->right - borderRight->start).length();

        float EXTRA_DIST = 0.1f;
        Vec2f pLeft, pRight;
        if (distLeft < distRight) {
            pLeft = borderLeft->start + (borderLeft->dir() * (distLeft - EXTRA_DIST));
            pRight = borderRight->start + (borderLeft->dir() * (distLeft - EXTRA_DIST));
        } else {
            pLeft = borderLeft->start + (borderLeft->dir() * (distRight - EXTRA_DIST));
            pRight = borderRight->start + (borderLeft->dir() * (distRight - EXTRA_DIST));
        }

        seg->setRightPointFor(joint->id, pRight);
        seg->setLeftPointFor(joint->id, pLeft);

        JointPoints* jp = new JointPoints();
        jp->left = pLeft;
        jp->right = pRight;
        jp->leftExt = p->leftExt;
        jointPoints.push_back(jp);
    }
    joint->jointPointCache_ = jointPoints;
    return jointPoints;
}

/** orders points of joint, so they are at the right position to work with*/
void StreetAlgos::jointOrderSegments(StreetJoint* joint, map<string, StreetSegment*> streetSegments, map<string, StreetJoint*> streetJoints) {
    if (joint == 0) return;
    Vec2f center = joint->position;
    vector<Vec2WithId*> points;
    for (string segId : joint->segmentIds) {
        if (!streetSegments.count(segId)) { continue; }
        StreetSegment* seg = streetSegments[segId];
        if (seg == 0) continue;
        StreetJoint* oJoint = streetJoints[seg->getOtherJointId(joint->id)];
        if (oJoint == 0) continue;
        Vec2WithId* p = new Vec2WithId();
        p->vec = oJoint->position;
        p->id = segId;
        points.push_back(p);
    }

    points = Vec2Helper::orderCW(points, center);

    joint->segmentIds.clear();
    for (Vec2WithId* point : points) joint->segmentIds.push_back(point->id);
}
