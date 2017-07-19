#ifndef VEC2HELPER_H
#define	VEC2HELPER_H

#include <OpenSG/OSGVector.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

class StreetSegment;

struct Vec2WithId {
    Vec2d vec;
    StreetSegment* seg;
};

class Vec2Helper {
public:
    static pair<bool, float> lineWithLineSegmentIntersection(Vec2d startLineA, Vec2d dirLineA, Vec2d posSeg1, Vec2d posSeg2);

    // returns how much line A has to be scaled to hit the intersection
    // || false if the lines do not intersect
    static pair<bool, float> lineIntersection(Vec2d startLineA, Vec2d dirLineA, Vec2d startLineB, Vec2d dirLineB);

    // calculates actual line intersection point
    static pair<bool, Vec2d> lineIntersectionPoint(Vec2d startLineA, Vec2d dirLineA, Vec2d startLineB, Vec2d dirLineB);

    static vector<Vec2WithId*> orderCW(vector<Vec2WithId*> points, Vec2d center);

    static bool compareAngle(Vec2WithId* a, Vec2WithId* b);
};

OSG_END_NAMESPACE;

#endif	/* VEC2HELPER_H */

