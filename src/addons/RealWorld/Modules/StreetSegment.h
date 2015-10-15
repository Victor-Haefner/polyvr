#ifndef STREETSEGMENT_H
#define	STREETSEGMENT_H

#include <OpenSG/OSGVector.h>

using namespace OSG;
using namespace std;

namespace realworld {

    class StreetSegment {
    public:
        string streetJointA_ID;
        string streetJointB_ID;
        float width;
        int lanes = 0;
        string id;
        string name;
        bool bridge = false;
        bool smallBridge = false;
        bool leftBridge = false;
        bool rightBridge = false;

        Vec2f leftA, leftB, rightA, rightB, leftExtA, leftExtB;

        StreetSegment(string streetJointA_ID, string streetJointB_ID, float width, string id) {
            this->streetJointA_ID = streetJointA_ID;
            this->streetJointB_ID = streetJointB_ID;
            this->width = width;
            this->id = id;
        }

        void setLeftPointFor(string jointId, Vec2f posLeft) {
            if (streetJointA_ID == jointId) rightB = posLeft;
            else leftA = posLeft;
        }

        void setLeftExtPointFor(string jointId, Vec2f posLeft) {
            if (streetJointA_ID == jointId) leftExtB = posLeft;
            else leftExtA = posLeft;
        }

        void setRightPointFor(string jointId, Vec2f posRight) {
            if (streetJointA_ID == jointId) leftB = posRight;
            else rightA = posRight;
        }

        string getOtherJointId(string jointId) {
            return streetJointA_ID == jointId ? streetJointB_ID : streetJointA_ID;
        }

        float getDistance() { return (leftA-leftB).length(); }
    };
}

#endif	/* STREETSEGMENT_H */

