#ifndef VRTREE_H_INCLUDED
#define VRTREE_H_INCLUDED

#include "core/objects/geometry/VRGeometry.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

struct segment;
struct seg_params;
struct leaf_params;

class VRTree : public VRTransform {
    private:
        int seed = 0;
        segment* trunc = 0;
        VRLodPtr lod;
        vector<shared_ptr<seg_params>> parameters;
        vector<shared_ptr<leaf_params>> foliage;
        vector<segment*> branches;
        vector<VRGeometryPtr> leafGeos;
        vector<VRGeometryPtr> woodGeos;
        map<int, VRGeometryPtr> leafLodCache;
        map<int, VRGeometryPtr> truncLodCache;
        static VRMaterialPtr treeMat;
        static VRMaterialPtr leafMat;

        Vec3f truncColor = Vec3f(0.4,0.2,0);

        float random (float min, float max);
        float variation(float val, float var);
        Vec3f randUVec();
        Vec3f randomRotate(Vec3f v, float a); //rotate a vector with angle 'a' in a random direction

        void initMaterials();

        void setup();
        void initLOD();
        void initArmatureGeo();
        void testSetup();

    protected:
        VRObjectPtr copy(vector<VRObjectPtr> children);

    public:
        VRTree(string name = "tree");
        ~VRTree();

        static VRTreePtr create(string name = "tree");
        VRTreePtr ptr();

        segment* grow(int seed = 0, segment* p = 0, int iteration = 0, float t = 1);
        void growLeafs(shared_ptr<leaf_params>);

        void addBranching(int nodes = 1, int branching = 5,
                   float n_angle = 0.2, float p_angle = 0.6, float length = 0.8, float radius = 0.1,
                   float n_angle_v = 0.2, float p_angle_v = 0.4, float length_v = 0.2, float radius_v = 0.2);

        // deprecated
        void setup(int branching = 5, int iterations = 5, int seed = 0,
                   float n_angle = 0.2, float p_angle = 0.6, float length = 0.8, float radius = 0.1,
                   float n_angle_v = 0.2, float p_angle_v = 0.4, float length_v = 0.2, float radius_v = 0.2);

        void addLeafs(int lvl, int amount, float size = 0.03);
        void setLeafMaterial(VRMaterialPtr mat);

        void createHullTrunkLod(VRGeoData& geo, int lvl, Vec3f offset, int ID);
        void createHullLeafLod(VRGeoData& geo, int lvl, Vec3f offset, int ID);
};

OSG_END_NAMESPACE;

#endif // VRTREE_H_INCLUDED
