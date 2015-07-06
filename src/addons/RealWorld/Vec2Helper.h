#ifndef VEC2HELPER_H
#define	VEC2HELPER_H

using namespace OSG;
using namespace std;

namespace realworld {

    struct Vec2WithId {
        Vec2f vec;
        string id;
    };

    class Vec2Helper {
    public:
        static pair<bool, float> lineWithLineSegmentIntersection(Vec2f startLineA, Vec2f dirLineA, Vec2f posSeg1, Vec2f posSeg2) {
            Vec2f dir12 = posSeg2-posSeg1;
            dir12.normalize();
            float lenAB = (posSeg2 - posSeg1).length();

            pair<bool, float> s = lineIntersection(posSeg1, dir12, startLineA, dirLineA);
            pair<bool, float> sInverse = lineIntersection(startLineA, dirLineA, posSeg1, dir12);

            if (s.first && sInverse.first && sInverse.second > 0 && s.second > 0 && s.second < lenAB)
                return make_pair(true, s.second);

            return make_pair(false, 0);
        }

        // returns how much line A has to be scaled to hit the intersection
        // || false if the lines do not intersect
        static pair<bool, float> lineIntersection(Vec2f startLineA, Vec2f dirLineA, Vec2f startLineB, Vec2f dirLineB) {
            float u_b = dirLineB.getValues()[1] * dirLineA.getValues()[0] - dirLineB.getValues()[0] * dirLineA.getValues()[1];

            if (u_b != 0) {
                float uaT = (dirLineB.getValues()[0] * (startLineA.getValues()[1] - startLineB.getValues()[1])) - (dirLineB.getValues()[1] * (startLineA.getValues()[0] - startLineB.getValues()[0]));
                //float ubT = (dirLineA.getValues()[0] * (startLineA.getValues()[1] - startLineB.getValues()[1])) - (dirLineA.getValues()[1] * (startLineA.getValues()[0] - startLineB.getValues()[0]));

                float ua = uaT / u_b;
                //float ub = ubT / u_b;

                //if (ua >= 0 && ua <= 1 && ub >= 0 && ub <= 1) {
                    return make_pair(true, ua);
                //}
            }

            return make_pair(false, 0);
        }

        // calculates actual line intersection point
        static pair<bool, Vec2f> lineIntersectionPoint(Vec2f startLineA, Vec2f dirLineA, Vec2f startLineB, Vec2f dirLineB) {
            pair<bool, float> s = lineIntersection(startLineA, dirLineA, startLineB, dirLineB);
            if (!s.first) return make_pair(false, Vec2f(0,0));
            return make_pair(true, startLineA + dirLineA*s.second);
        }

        static vector<Vec2WithId*> orderCW(vector<Vec2WithId*> points, Vec2f center) {
            for (unsigned int i=0; i<points.size(); i++) {
                Vec2f p = points[i]->vec-center;
                p.normalize();

                points[i]->vec = p;
            }

            std::sort(points.begin(), points.end(), compareAngle);
            return points;
        }

        static bool compareAngle(Vec2WithId* a, Vec2WithId* b) {
            float ang = atan2(b->vec.getValues()[1], b->vec.getValues()[0]) - atan2(a->vec.getValues()[1], a->vec.getValues()[0]);
            if (ang < 0) return false;
            return true;
        }
    };
}

#endif	/* VEC2HELPER_H */

