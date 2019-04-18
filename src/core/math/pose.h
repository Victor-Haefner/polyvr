#ifndef POSE_H_INCLUDED
#define POSE_H_INCLUDED

#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGMatrix.h>
#include "core/math/VRMathFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class Pose {
    private:
        vector<Vec3d> data;

    public:
        Pose();
        Pose(const Matrix& m);
        Pose(const Matrix4d& m);
        Pose(const Pose& p);
        Pose(Vec3d p, Vec3d d = Vec3d(0,0,-1), Vec3d u = Vec3d(0,1,0), Vec3d s = Vec3d(1,1,1));
        void set(Vec3d p, Vec3d d = Vec3d(0,0,-1), Vec3d u = Vec3d(0,1,0), Vec3d s = Vec3d(1,1,1));
        static PosePtr create();
        static PosePtr create(const Matrix4d& m);
        static PosePtr create(const Pose& p);
        static PosePtr create(Vec3d p, Vec3d d = Vec3d(0,0,-1), Vec3d u = Vec3d(0,1,0), Vec3d s = Vec3d(1,1,1));

        void setPos(Vec3d p);
        void setDir(Vec3d d);
        void setUp(Vec3d u);
        void setScale(Vec3d s);
        void makeUpOrthogonal();
        void makeDirOrthogonal();

        Vec3d pos() const;
        Vec3d dir() const;
        Vec3d up() const;
        Vec3d x() const;
        Vec3d scale() const;

        Matrix4d asMatrix() const;
        void invert();
        Vec3d transform(Vec3d p);
        Vec3d transformInv(Vec3d p);

        PosePtr multLeft(PosePtr p);
        PosePtr multRight(PosePtr p);

        string toString();

        bool operator == (const Pose& other) const;
        bool operator != (const Pose& other) const;
};

OSG_END_NAMESPACE;

#endif // POSE_H_INCLUDED
