#ifndef VRPOINTCLOUD_H_INCLUDED
#define VRPOINTCLOUD_H_INCLUDED

#include "core/objects/VRTransform.h"

OSG_BEGIN_NAMESPACE;

class VRPointCloud : public VRTransform {
    private:
        VRMaterialPtr mat;
        OctreePtr octree;
        int levels = 1;
        vector<int> downsamplingRate = {1};
        vector<float> lodDistances;

    public:
        VRPointCloud(string name);
        ~VRPointCloud();

        static VRPointCloudPtr create(string name);

        void addLevel(float distance, int downsampling);
        void setupLODs();

        void setupMaterial(bool lit, int pointsize);
        VRMaterialPtr getMaterial();

        OctreePtr getOctree();
};

OSG_END_NAMESPACE;

#endif // VRPOINTCLOUD_H_INCLUDED
