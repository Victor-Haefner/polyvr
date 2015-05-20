#ifndef VRCONSTRAINT_H_INCLUDED
#define VRCONSTRAINT_H_INCLUDED


#include <OpenSG/OSGMatrix.h>


OSG_BEGIN_NAMESPACE;
using namespace std;

class VRConstraint {
    private:
        bool active;
        bool local;
        Matrix refMatrixA;
        Matrix refMatrixB;

        float min[6];
        float max[6];

    public:
        VRConstraint();
        ~VRConstraint();

        void setActive(bool b);
        bool isActive();

        void setLocal(bool b);
        bool isLocal();

        void setMinMax(int i, float f1, float f2);
        void setMin(int i, float f);
        void setMax(int i, float f);

        float getMin(int i);
        float getMax(int i);

        void updateMatrix(Matrix& m);
        void updatePose(Vec3f& from, Vec3f& at, Vec3f& up);

        void setReferenceA(Matrix m);
        void setReferenceB(Matrix m);
        Matrix getReferenceA();
        Matrix getReferenceB();
};

OSG_END_NAMESPACE;

#endif // VRCONSTRAINT_H_INCLUDED
