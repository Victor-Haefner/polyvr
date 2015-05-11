#ifndef VRTREE_H_INCLUDED
#define VRTREE_H_INCLUDED

#include "core/objects/geometry/VRGeometry.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

struct segment;
struct seg_params;

class VRTree : public VRGeometry {
    private:
        segment* trunc;
        vector<segment*>* branches;
        VRGeometry* armatureGeo;

        float random (float min, float max);
        float variation(float val, float var);
        Vec3f randUVec();
        Vec3f randomRotate(Vec3f v, float a); //rotate a vector with angle 'a' in a random direction

        void grow(const seg_params& sp, segment* p, int iteration = 0);
        void initMaterial();

        void initArmatureGeo();
        void testSetup();

    public:
        VRTree();

        void setup(int branching = 5, int iterations = 5, int seed = 0,
                   float n_angle = 0.2, float p_angle = 0.6, float l_factor = 0.8, float r_factor = 0.5,
                   float n_angle_v = 0.2, float p_angle_v = 0.4, float l_factor_v = 0.2, float r_factor_v = 0.2);
};

OSG_END_NAMESPACE;

#endif // VRTREE_H_INCLUDED
