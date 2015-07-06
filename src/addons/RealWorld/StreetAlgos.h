#ifndef STREETALGOS_H
#define	STREETALGOS_H

#include "OpenSG/OSGConfig.h"
#include <string>
#include <vector>
#include <map>

using namespace OSG;
using namespace std;

namespace realworld {

struct StreetBorder;
class StreetSegment;
class StreetJoint;
class JointPoints;

class StreetAlgos {
    public:
        /** returns start && end point of the left border of a street segment */
        static StreetBorder* segmentGetLeftBorderTo(StreetSegment* seg, string jointId, map<string, StreetJoint*> streetJoints);

        /** returns start && end point of the right border of a street segment */
        static StreetBorder* segmentGetRightBorderTo(StreetSegment* seg, string jointId, map<string, StreetJoint*> streetJoints);

        /** returns imporant points to a joint for one street segment */
        static JointPoints* segmentGetPointsFor(StreetSegment* seg, string jointId);

        /** rémoves duplicate names from a given vector */
        static void vectorStrRemoveDuplicates(vector<string> vec);

        /** calculates the important points of a given joint */
        static void calcSegments(StreetJoint* joint, map<string, StreetSegment*> streetSegments, map<string, StreetJoint*> streetJoints);

        /** calculates the important points to a given joint */
        static vector<JointPoints*> calcJoints(StreetJoint* joint, map<string, StreetSegment*> streetSegments, map<string, StreetJoint*> streetJoints);

        /** orders points of joint, so they are at the right position to work with*/
        static void jointOrderSegments(StreetJoint* joint, map<string, StreetSegment*> streetSegments, map<string, StreetJoint*> streetJoints);
};

}

#endif	/* STREETALGOS_H */
