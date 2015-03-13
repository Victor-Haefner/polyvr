#ifndef VRPERLIN_H_INCLUDED
#define VRPERLIN_H_INCLUDED

#include <OpenSG/OSGVector.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRPerlin {
    private:
        float lerp(float a0, float a1, float w);
        float dotGridGradient(Vec3f* grid, Vec3i dim, Vec3i vi, Vec3f v);
        float perlin(Vec3f* grid, const Vec3i& dim, const Vec3f& v);

    public:
        VRPerlin();

        static void apply(Vec3f* data, Vec3i dim, float amount, Vec3f c1, Vec3f c2);
};

OSG_END_NAMESPACE;

#endif // VRPERLIN_H_INCLUDED
