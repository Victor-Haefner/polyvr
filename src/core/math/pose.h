#ifndef POSE_H_INCLUDED
#define POSE_H_INCLUDED

#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGMatrix.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

class pose {
    private:
        vector<Vec3f> data;

    public:
        pose();
        pose(Vec3f p, Vec3f d, Vec3f u);
        void set(Vec3f p, Vec3f d, Vec3f u);
        static shared_ptr<pose> create(Vec3f p, Vec3f d, Vec3f u);

        Vec3f pos();
        Vec3f dir();
        Vec3f up();

        Matrix asMatrix();
};

OSG_END_NAMESPACE;

#endif // POSE_H_INCLUDED
