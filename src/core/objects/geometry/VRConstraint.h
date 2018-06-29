#ifndef VRCONSTRAINT_H_INCLUDED
#define VRCONSTRAINT_H_INCLUDED

#include <OpenSG/OSGMatrix.h>
#include "core/utils/VRStorage.h"
#include "core/objects/VRObjectFwd.h"
#include "core/math/VRMathFwd.h"

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
        bool active = 0;
        bool local = 0;
        Matrix4d refMatrixA;
        Matrix4d refMatrixB;
        Matrix4d refMatrixAI;
        Matrix4d refMatrixBI;

        float min[6];
        float max[6];

    public:    // TODO: refactor old VRTransform stuff
        unsigned int apply_time_stamp = 0;
        VRTransformWeakPtr Referential;

    public:
        VRConstraint();
        ~VRConstraint();

        static VRConstraintPtr create();
        VRConstraintPtr duplicate();

        void setActive(bool b);
        void setLocal(bool b);
        bool isActive();
        bool isLocal();

        void setMinMax(int i, float f1, float f2);
        void setMin(int i, float f);
        void setMax(int i, float f);
        float getMin(int i);
        float getMax(int i);

        void lock(vector<int> dofs, float v = 0);
        void free(vector<int> dofs);
        void lockRotation();
        bool isLocked(int i);

        void setReferenceA(PosePtr p);
        void setReferenceB(PosePtr p);
        PosePtr getReferenceA();
        PosePtr getReferenceB();

        void apply(VRTransformPtr obj, VRObjectPtr parent = 0);


        // TODO: refactor old VRTransform stuff
        void setReference(PosePtr m);
        void setReferential(VRTransformPtr ref);
        void setTConstraint(Vec3d trans, TCMode mode, bool local = false);
        void setRConstraint(Vec3d rot, TCMode mode, bool local = false);
};

OSG_END_NAMESPACE;

#endif // VRCONSTRAINT_H_INCLUDED
