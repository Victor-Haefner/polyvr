#ifndef VRPOINTCLOUD_H_INCLUDED
#define VRPOINTCLOUD_H_INCLUDED

#include "core/objects/VRTransform.h"
#include <OpenSG/OSGColor.h>
#include <OpenSG/OSGVector.h>

OSG_BEGIN_NAMESPACE;

class VRPointCloud : public VRTransform {
    public:
        enum POINTTYPE {
            NONE,
            COLOR,
            SPLAT
        };

        struct Splat {
            Vec3d p;
            Color3ub c;
            Vec2ub v1;
            Vec2ub v2;
            char w;
        };

        struct PntCol {
            Vec3d p;
            Color3ub c;
        };

        POINTTYPE pointType = NONE;

    private:
        VRMaterialPtr mat;
        OctreePtr octree;
        int levels = 1;
        bool keepOctree = 0;
        vector<int> downsamplingRate = {1};
        vector<float> lodDistances;

        static string splatVP;
        static string splatFP;
        static string splatGP;

    public:
        VRPointCloud(string name = "pointcloud");
        ~VRPointCloud();

        static VRPointCloudPtr create(string name = "pointcloud");
        void applySettings(map<string, string> options);

        void addLevel(float distance, int downsampling);
        void setupLODs();

        void setupMaterial(bool lit, int pointsize, bool doSplat = false, float splatModifier = 0.001);
        VRMaterialPtr getMaterial();

        void addPoint(Vec3d p, Color3ub c);
        void addPoint(Vec3d p, Splat c);

        void convert(string pathIn);
        void genTestFile(string path, size_t N, bool doColor);
        void genTestFile2(string path, size_t N, bool doColor);
        void externalSort(string path, size_t Nchunks, double binSize);

        OctreePtr getOctree();
};

OSG_END_NAMESPACE;

#endif // VRPOINTCLOUD_H_INCLUDED
