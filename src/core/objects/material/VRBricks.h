#ifndef VRBRICKS_H_INCLUDED
#define VRBRICKS_H_INCLUDED

#include "core/math/OSGMathFwd.h"
#include <OpenSG/OSGColor.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRBricks {
    private:
    public:
        VRBricks();

        static void apply(Color3f* data, Vec3i dim, float amount, Color3f c1, Color3f c2);
        static void apply(Color4f* data, Vec3i dim, float amount, Color4f c1, Color4f c2);
};

OSG_END_NAMESPACE;

#endif // VRBRICKS_H_INCLUDED
