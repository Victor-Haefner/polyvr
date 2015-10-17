#ifndef STREETJOINT_H
#define	STREETJOINT_H

#include <string>
#include <vector>
#include <OpenSG/OSGVector.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

struct JointPoints {
    Vec2f left;
    Vec2f right;
    Vec2f leftExt;
};

class StreetJoint {
    public:
        Vec2f position;
        string id;
        vector<string> segmentIds;
        string info;
        bool bridge = false;
        bool smallBridge = false;

        vector<JointPoints*> jointPointCache_;
        bool calcSegPoints_ = false;

        StreetJoint(Vec2f position, string id);

        void merge(StreetJoint* streetJoint);

        // call this, when joint needs recalculation
        void resetCaches();
};

OSG_END_NAMESPACE;

#endif	/* STREETJOINT_H */

