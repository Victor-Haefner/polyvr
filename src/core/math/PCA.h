#ifndef PCA_H_INCLUDED
#define PCA_H_INCLUDED

#include "core/math/VRMathFwd.h"
#include "core/math/pose.h"
#include "core/objects/VRObjectFwd.h"
#include <OpenSG/OSGMatrix.h>

using namespace std;
OSG_BEGIN_NAMESPACE;

class PCA {
    private:
        vector<Vec3d> pnts;

        Vec3d computeCentroid();
        Matrix4d computeCovMatrix();
        Matrix4d computeEigenvectors(Matrix4d m);

    public:
        PCA();
        ~PCA();
        static PCAPtr create();

        Pose compute();
        void add(Vec3d p);
        void addMesh(VRObjectPtr obj);
        int size();
        void clear();

        void test();
};

OSG_END_NAMESPACE;

#endif // PCA_H_INCLUDED
