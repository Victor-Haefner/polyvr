#ifndef VRPERLIN_H_INCLUDED
#define VRPERLIN_H_INCLUDED

#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGColor.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRPerlin {
    private:
        float lerp(float a0, float a1, float w);
        float hermite3(float w);
        float hermite5(float w);

        float dotGridGradient(Vec3d* grid, Vec3i dim, Vec3i vi, Vec3d v);
        float perlin(Vec3d* grid, const Vec3i& dim, const Vec3d& v);

    public:
        VRPerlin();

        static void apply(Color3f* data, Vec3i dim, float amount, Color3f c1, Color3f c2);
        static void apply(Color4f* data, Vec3i dim, float amount, Color4f c1, Color4f c2);
};

OSG_END_NAMESPACE;

#endif // VRPERLIN_H_INCLUDED
