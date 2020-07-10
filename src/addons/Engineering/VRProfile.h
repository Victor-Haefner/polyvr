#ifndef VRPROFILE_H_INCLUDED
#define VRPROFILE_H_INCLUDED

#include "core/math/OSGMathFwd.h"
#include <OpenSG/OSGConfig.h>
#include <vector>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRProfile {
    private:
        vector<Vec2d> pnts;

    public:
        void add(Vec2d v);
        vector<Vec3d> get(const Vec3d& n = NDIR(), const Vec3d& u = UP());
};

OSG_END_NAMESPACE;

#endif // VRPROFILE_H_INCLUDED
