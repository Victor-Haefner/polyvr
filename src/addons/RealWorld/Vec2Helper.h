#ifndef VEC2HELPER_H
#define	VEC2HELPER_H

#include <OpenSG/OSGVector.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

class StreetSegment;

struct Vec2WithId {
    Vec2f vec;
    StreetSegment* seg;
};

class Vec2Helper {
public:
    static pair<bool, float> lineWithLineSegmentIntersection(Vec2f startLineA, Vec2f dirLineA, Vec2f posSeg1, Vec2f posSeg2);

    // returns how much line A has to be scaled to hit the intersection
    // || false if the lines do not intersect
    static pair<bool, float> lineIntersection(Vec2f startLineA, Vec2f dirLineA, Vec2f startLineB, Vec2f dirLineB);

    // calculates actual line intersection point
    static pair<bool, Vec2f> lineIntersectionPoint(Vec2f startLineA, Vec2f dirLineA, Vec2f startLineB, Vec2f dirLineB);

    static vector<Vec2WithId*> orderCW(vector<Vec2WithId*> points, Vec2f center);

    static bool compareAngle(Vec2WithId* a, Vec2WithId* b);
};

OSG_END_NAMESPACE;

#endif	/* VEC2HELPER_H */

