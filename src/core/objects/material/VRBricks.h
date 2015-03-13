#ifndef VRBRICKS_H_INCLUDED
#define VRBRICKS_H_INCLUDED

#include <OpenSG/OSGVector.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRBricks {
    private:
    public:
        VRBricks();

        static void apply(Vec3f* data, Vec3i dim, float amount, Vec3f c1, Vec3f c2);
};

OSG_END_NAMESPACE;

#endif // VRBRICKS_H_INCLUDED
