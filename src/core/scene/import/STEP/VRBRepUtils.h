#ifndef VRBREPUTILS_H_INCLUDED
#define VRBREPUTILS_H_INCLUDED

#include <OpenSG/OSGVector.h>

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRBRepUtils {
    protected:

        static const int Ncurv = 5;//17; // N in 2pi - should be multiple of 4 +1
        static const float Dangle; // angle segment
        static vector<float> Adict; // all angles

        bool sameVec(const Vec3f& v1, const Vec3f& v2, float d = 1e-5);
        vector<float> angleFrame(float a1, float a2);
        int getSideN(float a);
        Vec2f getSide(float a);
        Vec2f getSide(int i);

    public:
        VRBRepUtils();
};

OSG_END_NAMESPACE;

#endif // VRBREPUTILS_H_INCLUDED
