#ifndef VRTREE_H_INCLUDED
#define VRTREE_H_INCLUDED

#include "core/objects/geometry/VRGeometry.h"
#include <OpenSG/OSGColor.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

struct segment;
struct seg_params;
struct leaf_params;

class VRTree : public VRTransform {
    public:
        static string treeSprLODvp;
        static string treeSprLODdfp;

    private:
        int seed = 0;
        segment* trunc = 0;
        VRGeometryPtr lod;
        vector<shared_ptr<seg_params>> parameters;
        vector<shared_ptr<leaf_params>> foliage;
        vector<segment*> branches;
        vector<VRGeometryPtr> leafGeos;
        vector<VRGeometryPtr> woodGeos;
        vector<VRMaterialPtr> lodMaterials;
        map<int, VRGeometryPtr> leafLodCache;
        map<int, VRGeometryPtr> truncLodCache;
        static VRMaterialPtr treeMat;
        static VRMaterialPtr leafMat;

        Color4f truncColor = Color4f(0.4,0.2,0,1);

        float random (float min, float max);
        float variation(float val, float var);
        Vec3d randUVec();
        Vec3d randomRotate(Vec3d v, float a); //rotate a vector with angle 'a' in a random direction

        void initMaterials();

        void storeSetup(VRStorageContextPtr context);
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

        segment* growSegment(int seed = 0, segment* p = 0, int iteration = 0, float t = 1);
        void growLeafs(shared_ptr<leaf_params>);
        void grow(int seed = 0);
        string getHash(vector<float> v = vector<float>());

        // params: n_angle, p_angle, length, radius
        void setup(int branching = 5, int iterations = 5, int seed = 0, Vec4d params = Vec4d(0.2, 0.6, 0.8, 0.1), Vec4d params_v = Vec4d(0.2, 0.4, 0.2, 0.2) );
        void addBranching(int nodes = 1, int branching = 5, Vec4d params = Vec4d(0.2, 0.6, 0.8, 0.1), Vec4d params_v = Vec4d(0.2, 0.4, 0.2, 0.2) );
        void addLeafs(int lvl, int amount, float size = 0.03);
        void setLeafMaterial(VRMaterialPtr mat);

        VRMaterialPtr getTruncMaterial();
        VRMaterialPtr getLeafMaterial();

        vector<VRMaterialPtr> createLODtextures(int& Hmax, VRGeoData& data);
        VRGeometryPtr createLOD(int lvl);
        VRGeometryPtr getLOD(int lvl);
        void appendLOD(VRGeoData& data, int lvl, Vec3d offset);
        void createTwigLod(VRGeoData& geo, int lvl);
        void createHullTrunkLod(VRGeoData& geo, int lvl, Vec3d offset, int ID); // soon deprecated
        void createHullLeafLod(VRGeoData& geo, int lvl, Vec3d offset, int ID); // soon deprecated

        vector<VRMaterialPtr> getLodMaterials();
};

OSG_END_NAMESPACE;

#endif // VRTREE_H_INCLUDED
