#ifndef POSE_H_INCLUDED
#define POSE_H_INCLUDED

#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGMatrix.h>
#include "core/math/VRMathFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class pose {
    private:
        vector<Vec3d> data;

    public:
        pose();
        pose(const Matrix4d& m);
        pose(const pose& p);
        pose(Vec3d p, Vec3d d = Vec3d(0,0,-1), Vec3d u = Vec3d(0,1,0));
        void set(Vec3d p, Vec3d d = Vec3d(0,0,-1), Vec3d u = Vec3d(0,1,0));
        static posePtr create();
        static posePtr create(const Matrix4d& m);
        static posePtr create(const pose& p);
        static posePtr create(Vec3d p, Vec3d d = Vec3d(0,0,-1), Vec3d u = Vec3d(0,1,0));

        void setPos(Vec3d p);
        void setDir(Vec3d d);
        void setUp(Vec3d u);

        Vec3d pos() const;
        Vec3d dir() const;
        Vec3d up() const;
        Vec3d x() const;

        Matrix4d asMatrix() const;
        void invert();
        Vec3d transform(Vec3d p);

        string toString();
};

OSG_END_NAMESPACE;

#endif // POSE_H_INCLUDED
