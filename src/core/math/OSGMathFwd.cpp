#include "OSGMathFwd.h"
#include <OpenSG/OSGVector.h>

using namespace std;

namespace OSG {
    Vec3d& VEC3D() { static Vec3d v; return v; }
    Vec3d& DIR() { static Vec3d d(0,0,-1); return d; }
    Vec3d& NDIR() { static Vec3d d(0,0,1); return d; }
    Vec3d& UP() { static Vec3d u(0,1,0); return u; }
    Vec3d& NUP() { static Vec3d u(0,-1,0); return u; }
    Vec3d& SCALE() { static Vec3d s(1,1,1); return s; }
}
