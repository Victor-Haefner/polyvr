#ifndef POSE_H_INCLUDED
#define POSE_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <vector>
#include "core/math/VRMathFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

template<class ValueTypeT, unsigned int SizeI> class Vector;
typedef Vector< double, 3 > Vec3d;

template<class ValueTypeT> class TransformationMatrix;
typedef TransformationMatrix<float> Matrix;
typedef TransformationMatrix<double> Matrix4d;

class Pose {
    private:
        vector<Vec3d> data;

        static Vec3d& DIR();
        static Vec3d& UP();
        static Vec3d& SCALE();

    public:
        Pose();
        Pose(const Matrix& m);
        Pose(const Matrix4d& m);
        Pose(const Pose& p);
        Pose(const Vec3d& p, const Vec3d& d = DIR(), const Vec3d& u = UP(), const Vec3d& s = SCALE());
        void set(const Vec3d& p, const Vec3d& d = DIR(), const Vec3d& u = UP(), const Vec3d& s = SCALE());
        static PosePtr create();
        static PosePtr create(const Matrix4d& m);
        static PosePtr create(const Pose& p);
        static PosePtr create(const Vec3d& p, const Vec3d& d = DIR(), const Vec3d& u = UP(), const Vec3d& s = SCALE());

        void setPos(const Vec3d& p);
        void setDir(const Vec3d& d);
        void setUp(const Vec3d& u);
        void setScale(const Vec3d& s);
        void makeUpOrthogonal();
        void makeDirOrthogonal();

        Vec3d pos();
        Vec3d dir();
        Vec3d up();
        Vec3d x();
        Vec3d scale();

        Matrix4d asMatrix() const;
        void invert();
        Vec3d transform(const Vec3d& p, bool doTranslate = true);
        Vec3d transformInv(const Vec3d& p);

        PosePtr multLeft(PosePtr p);
        PosePtr multRight(PosePtr p);

        string toString();

        bool operator == (const Pose& other) const;
        bool operator != (const Pose& other) const;
};

OSG_END_NAMESPACE;

#endif // POSE_H_INCLUDED
