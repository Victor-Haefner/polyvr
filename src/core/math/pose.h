#ifndef POSE_H_INCLUDED
#define POSE_H_INCLUDED

#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGMatrix.h>
#include "core/math/VRMathFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class pose {
    private:
        vector<Vec3f> data;

    public:
        pose();
        pose(const Matrix& m);
        pose(const pose& p);
        pose(Vec3f p, Vec3f d = Vec3f(0,0,-1), Vec3f u = Vec3f(0,1,0));
        void set(Vec3f p, Vec3f d = Vec3f(0,0,-1), Vec3f u = Vec3f(0,1,0));
        static posePtr create();
        static posePtr create(const Matrix& m);
        static posePtr create(const pose& p);
        static posePtr create(Vec3f p, Vec3f d = Vec3f(0,0,-1), Vec3f u = Vec3f(0,1,0));

        void setPos(Vec3f p);
        void setDir(Vec3f d);
        void setUp(Vec3f u);

        Vec3f pos() const;
        Vec3f dir() const;
        Vec3f up() const;

        Matrix asMatrix() const;
        void invert();

        string toString();
};

OSG_END_NAMESPACE;

#endif // POSE_H_INCLUDED
