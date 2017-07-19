#ifndef VRPROFILE_H_INCLUDED
#define VRPROFILE_H_INCLUDED

#include <OpenSG/OSGVector.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRProfile {
    private:
        vector<Vec2d> pnts;

    public:
        void add(Vec2d v);
        vector<Vec3d> get(Vec3d n = Vec3d(0,0,1), Vec3d u = Vec3d(0,1,0));
};

OSG_END_NAMESPACE;

#endif // VRPROFILE_H_INCLUDED
