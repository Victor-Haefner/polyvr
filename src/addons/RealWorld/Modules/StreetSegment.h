#ifndef STREETSEGMENT_H
#define	STREETSEGMENT_H

#include <OpenSG/OSGVector.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

class StreetJoint;

class StreetSegment {
    public:
        StreetJoint* jointA = 0;
        StreetJoint* jointB = 0;
        float width;
        int lanes = 0;
        string id;
        string name;
        bool bridge = false;
        float bridgeHeight = 0;

        Vec2f leftA, leftB, rightA, rightB, leftExtA, leftExtB;

        StreetSegment(StreetJoint* jA, StreetJoint* jB, float width, string id);

        void setLeftPointFor(string jointId, Vec2f posLeft);
        void setLeftExtPointFor(string jointId, Vec2f posLeft);
        void setRightPointFor(string jointId, Vec2f posRight);

        string getOtherJointId(string jointId);
        float getDistance();
};

OSG_END_NAMESPACE;

#endif	/* STREETSEGMENT_H */

