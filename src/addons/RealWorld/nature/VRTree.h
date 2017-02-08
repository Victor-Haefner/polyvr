#ifndef VRTREE_H_INCLUDED
#define VRTREE_H_INCLUDED

#include "core/objects/geometry/VRGeometry.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

struct segment;
struct seg_params;

class VRTree : public VRTransform {
    private:
        segment* trunc = 0;
        VRLodPtr lod;
        vector<segment*> branches;
        vector<VRGeometryPtr> leafGeos;
        vector<VRGeometryPtr> woodGeos;
        static VRMaterialPtr treeMat;
        static VRMaterialPtr leafMat;

        float random (float min, float max);
        float variation(float val, float var);
        Vec3f randUVec();
        Vec3f randomRotate(Vec3f v, float a); //rotate a vector with angle 'a' in a random direction

        void grow(const seg_params& sp, segment* p, int iteration = 0);
        void initMaterials();

        void initLOD();
        void initArmatureGeo();
        void testSetup();

    protected:
        VRObjectPtr copy(vector<VRObjectPtr> children);

    public:
        VRTree();
        ~VRTree();

        static VRTreePtr create();
        VRTreePtr ptr();

        void setup(int branching = 5, int iterations = 5, int seed = 0,
                   float n_angle = 0.2, float p_angle = 0.6, float l_factor = 0.8, float r_factor = 0.5,
                   float n_angle_v = 0.2, float p_angle_v = 0.4, float l_factor_v = 0.2, float r_factor_v = 0.2);

        void addLeafs(int lvl, int amount);
        void setLeafMaterial(VRMaterialPtr mat);

        void createHullTrunkLod(VRGeoData& geo, int lvl, Vec3f offset);
        void createHullLeafLod(VRGeoData& geo, int lvl, Vec3f offset);
};

OSG_END_NAMESPACE;

#endif // VRTREE_H_INCLUDED
