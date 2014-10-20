#ifndef STREETSEGMENT_H
#define	STREETSEGMENT_H

using namespace OSG;
using namespace std;

namespace realworld {

    class StreetSegment {
    public:
        string streetJointA_ID;
        string streetJointB_ID;
        float width;
        int lanes;
        string id;
        string name;
        bool bridge;
        bool smallBridge;
        bool leftBridge;
        bool rightBridge;

        Vec2f leftA, leftB, rightA, rightB, leftExtA, leftExtB;

        StreetSegment(string streetJointA_ID, string streetJointB_ID, float width, string id) {
            this->streetJointA_ID = streetJointA_ID;
            this->streetJointB_ID = streetJointB_ID;
            this->width = width;
            this->id = id;
            this->lanes = 0;
            this->name = "";
            this->bridge = false;
            this->smallBridge = false;
            this->leftBridge = false;
            this->rightBridge = false;
        }

        void setLeftPointFor(string jointId, Vec2f posLeft) {
            if (this->streetJointA_ID == jointId) {
                this->rightB = posLeft;
            } else {
                this->leftA = posLeft;
            }
        }
        void setLeftExtPointFor(string jointId, Vec2f posLeft) {
            if (this->streetJointA_ID == jointId) {
                this->leftExtB = posLeft;
            } else {
                this->leftExtA = posLeft;
            }
        }
        void setRightPointFor(string jointId, Vec2f posRight) {
            if (this->streetJointA_ID == jointId) {
                this->leftB = posRight;
            } else {
                this->rightA = posRight;
            }
        }

        string getOtherJointId(string jointId) {
            if (this->streetJointA_ID == jointId) {
                return this->streetJointB_ID;
            } else {
                return this->streetJointA_ID;
            }
        }

        float getDistance(){
            float a = abs(leftA.getValues()[0] - leftB.getValues()[0]);
            float b = abs(leftA.getValues()[1] - leftB.getValues()[1]);
            return sqrt(a*a + b*b);
        }

    };
}
//// ---- Class StreetSegment ----
//StreetSegment.removeDuplicates = function (segments) {
//	var singleSegments = [];
//	for (var i = 0; i < segments.length; i++) {
//		var s = segments[i];
//		var hasDuplicate = false;
//		for (var j = i+1; j < segments.length; j++) {
//			var s2 = segments[j];
//			if ((s.streetJointA_ID === s2.streetJointA_ID || s.streetJointA_ID === s2.streetJointB_ID) &&
//				(s.streetJointB_ID === s2.streetJointA_ID || s.streetJointB_ID === s2.streetJointB_ID)) hasDuplicate = true;
//		};
//
//		if (!hasDuplicate) singleSegments.push(s);
//	};
//	return singleSegments;
//};

#endif	/* STREETSEGMENT_H */

