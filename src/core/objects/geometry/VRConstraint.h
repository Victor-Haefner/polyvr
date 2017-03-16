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
        Matrix refMatrixA;
        Matrix refMatrixB;
        Matrix refMatrixBI;

        float min[6];
        float max[6];

        // precomputed ressources
        Matrix rotRebase;
        Matrix rotRebaseI;
        Matrix refRebased;
        Matrix refRebasedI;

        void prepare();

    public:    // TODO: refactor old VRTransform stuff
        unsigned int apply_time_stamp = 0;
        Matrix Reference;
        VRTransformWeakPtr Referential;
        bool localTC = false;
        bool localRC = false;
        int tConMode = NONE;
        int rConMode = NONE;
        Vec3f tConstraint = Vec3f(0,1,0);
        Vec3f rConstraint = Vec3f(0,1,0);

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

        void setReferenceA(Matrix m);
        void setReferenceB(Matrix m);
        Matrix getReferenceA();
        Matrix getReferenceB();

        void apply(VRTransformPtr t);


        // TODO: refactor old VRTransform stuff
        void setReference(Matrix m);
        void setReferential(VRTransformPtr ref);
        void setTConstraint(Vec3f trans, int mode, bool local = false);
        void setRConstraint(Vec3f rot, int mode, bool local = false);
        bool getRMode();
        bool getTMode();
        Vec3f getTConstraint();
        Vec3f getRConstraint();
        bool hasTConstraint();
        bool hasRConstraint();
        bool hasConstraint();
};

OSG_END_NAMESPACE;

#endif // VRCONSTRAINT_H_INCLUDED
