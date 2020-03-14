#ifndef VRNORMALMAP_H_INCLUDED
#define VRNORMALMAP_H_INCLUDED

#include "core/math/OSGMathFwd.h"
#include <OpenSG/OSGColor.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRNormalmap {
    private:
        float lerp(float a0, float a1, float w);
        float hermite3(float w);
        float hermite5(float w);

        float dotGridGradient(Vec3d* grid, Vec3i dim, Vec3i vi, Vec3d v);
        float perlin(Vec3d* grid, const Vec3i& dim, const Vec3d& v);

    public:
        VRNormalmap();

        static void apply(Color3f* data, Vec3i dim, float amount);
        static void apply(Color4f* data, Vec3i dim, float amount);
};

OSG_END_NAMESPACE;

#endif // VRNORMALMAP_H_INCLUDED
