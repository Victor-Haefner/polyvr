#ifndef VRCONSTRAINT_H_INCLUDED
#define VRCONSTRAINT_H_INCLUDED

#include <OpenSG/OSGMatrix.h>
#include "core/utils/VRStorage.h"
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRConstraint : public VRStorage {
    public:
        enum TCMode {
            NONE = 0,
            POINT = 1,
            LINE = 2,
            PLANE = 3
        };

    private:
        bool active;
        bool local;
        Matrix refMatrixA;
        Matrix refMatrixB;

        float min[6];
        float max[6];

    public:    // TODO: refactor old VRTransform stuff
        unsigned int apply_time_stamp = 0;
        Matrix constraints_reference;
        VRTransformWeakPtr constraints_referential;
        bool doTConstraint = false;
        bool doRConstraint = false;
        bool localTC = false;
        int tConMode = PLANE;
        Vec3f tConstraint = Vec3f(0,1,0);
        Vec3i rConstraint;

    public:
        VRConstraint();
        ~VRConstraint();

        static VRConstraintPtr create();

        void setActive(bool b);
        bool isActive();

        void setLocal(bool b);
        bool isLocal();

        void setMinMax(int i, float f1, float f2);
        void setMin(int i, float f);
        void setMax(int i, float f);

        float getMin(int i);
        float getMax(int i);

        void setReferenceA(Matrix m);
        void setReferenceB(Matrix m);
        Matrix getReferenceA();
        Matrix getReferenceB();

        void apply(VRTransformPtr t);
};

OSG_END_NAMESPACE;

#endif // VRCONSTRAINT_H_INCLUDED
