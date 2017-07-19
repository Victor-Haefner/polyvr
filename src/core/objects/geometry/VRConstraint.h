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
        bool active = 1;
        bool local = 0;
        Matrix4d refMatrixA;
        Matrix4d refMatrixB;
        Matrix4d refMatrixBI;

        float min[6];
        float max[6];

        // precomputed ressources
        Matrix4d rotRebase;
        Matrix4d rotRebaseI;
        Matrix4d refRebased;
        Matrix4d refRebasedI;

        void prepare();

    public:    // TODO: refactor old VRTransform stuff
        unsigned int apply_time_stamp = 0;
        Matrix4d Reference;
        VRTransformWeakPtr Referential;
        bool localTC = false;
        bool localRC = false;
        int tConMode = NONE;
        int rConMode = NONE;
        Vec3d tConstraint = Vec3d(0,1,0);
        Vec3d rConstraint = Vec3d(0,1,0);

    public:
        VRConstraint();
        ~VRConstraint();

        static VRConstraintPtr create();
        VRConstraintPtr duplicate();

        void setActive(bool b, VRTransformPtr obj);
        bool isActive();

        void setLocal(bool b);
        bool isLocal();

        void setMinMax(int i, float f1, float f2);
        void setMin(int i, float f);
        void setMax(int i, float f);

        float getMin(int i);
        float getMax(int i);

        void setReferenceA(Matrix4d m);
        void setReferenceB(Matrix4d m);
        Matrix4d getReferenceA();
        Matrix4d getReferenceB();

        void lockRotation();

        void apply(VRTransformPtr t);


        // TODO: refactor old VRTransform stuff
        void setReference(Matrix4d m);
        void setReferential(VRTransformPtr ref);
        void setTConstraint(Vec3d trans, int mode, bool local = false);
        void setRConstraint(Vec3d rot, int mode, bool local = false);
        bool getRMode();
        bool getTMode();
        Vec3d getTConstraint();
        Vec3d getRConstraint();
        bool hasTConstraint();
        bool hasRConstraint();
        bool hasConstraint();
};

OSG_END_NAMESPACE;

#endif // VRCONSTRAINT_H_INCLUDED
