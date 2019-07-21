#ifndef TSDF_H_INCLUDED
#define TSDF_H_INCLUDED

#include <OpenSG/OSGVector.h>
#include "core/math/VRMathFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class TSDF {
    private:
        Vec3i size;
        vector<float> field;

        bool inside(Vec3i& p);
        size_t index(Vec3i& p);

    public:
        TSDF(Vec3i size);
        ~TSDF();

        TSDFPtr create(Vec3i size);

        void set(float f, Vec3i p);
        float get(Vec3i p);
};

OSG_END_NAMESPACE;

#endif // TSDF_H_INCLUDED
