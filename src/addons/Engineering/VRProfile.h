#ifndef VRPROFILE_H_INCLUDED
#define VRPROFILE_H_INCLUDED

#include <OpenSG/OSGVector.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRProfile {
    private:
        vector<Vec2f> pnts;

    public:
        void add(Vec2f v);
        vector<Vec3f> get(Vec3f n = Vec3f(0,0,1), Vec3f u = Vec3f(0,1,0));
};

OSG_END_NAMESPACE;

#endif // VRPROFILE_H_INCLUDED
