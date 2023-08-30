#ifndef VRPOINTCLOUD_H_INCLUDED
#define VRPOINTCLOUD_H_INCLUDED

#include "core/objects/VRTransform.h"
#include "core/objects/material/VRMaterialFwd.h"
#include "core/scene/import/VRImport.h"
#include "core/utils/VRMutex.h"

#include <OpenSG/OSGColor.h>
#include <OpenSG/OSGVector.h>

OSG_BEGIN_NAMESPACE;

class VRExternalPointCloud {
    public:
        struct OcChunkRef {
            string path;
            size_t offset = 0;
            size_t size = 0;
            ofstream stream;
            bool isOpen = false;
        };

        struct OcSerialNode {
            //double size = 0;
            //Vec3d center;
            size_t chunkOffset = 0;
            size_t chunkSize = 0;
            int children[8] = {0,0,0,0,0,0,0,0};
        };

    public:
        string path;
        map<string, string> params;

        bool valid = false;
        bool hasColors = false;
        bool hasSplats = false;
        bool isSorted = false;
        bool hasOctree = false;

        int binPntSize = 0;
        size_t size = 0;
        int headerLength = 0;
        int binPntsStart = 0;

    public:
        VRExternalPointCloud(string path);
        ~VRExternalPointCloud();

        OcSerialNode getOctreeNode(Vec3d p);
        void printOctree();

        static map<string, string> readPCBHeader(string path);
        static void writePCBHeader(string path, map<string, string> params);
        static void copyPCBOctree(string path, const VRExternalPointCloud& epc);
};

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
            char w = 0;
            static const int size = 32; // 24 + 3 + 2 + 2 + 1  // for IO, because sizeof contains padding
        };

        struct PntCol {
            Vec3d p;
            Color3ub c;
            static const int size = 27; // 24 + 3  // for IO, because sizeof contains padding
        };

        struct PntData {
            Color3ub c;
            Vec2ub v1;
            Vec2ub v2;
            char w = 0;
        };

        POINTTYPE pointType = NONE;

    private:
        VRMaterialPtr mat;
        shared_ptr<Octree<PntData>> octree;
        int levels = 1;
        bool keepOctree = 0;
        bool lodsSetUp = 0;
        double geoLocationN = 0;
        double geoLocationE = 0;
        vector<int> downsamplingRate = {1};
        vector<float> lodDistances;
        VRImportCbPtr onImport;
        VRMutex mtx;

        // import options
        string filePath;
        bool lit = 0;
        int pointSize = 1;
        double leafSize = 10;
        double actualLeafSize = 0;
        size_t partitionLimit = 1e5;
        double splatScale = 1.0;

        static string splatVP;
        static string splatFP;
        static string splatGP;

        Vec2ub toSpherical(const Vec3d& v);
        void loadChunk(VRLodPtr lod);
        void onLodSwitch(VRLodEventPtr e);
        void onImportEvent(VRImportJob params);
        VRProgressPtr addProgress(string head, size_t N);

    public:
        VRPointCloud(string name = "pointcloud");
        ~VRPointCloud();

        static VRPointCloudPtr create(string name = "pointcloud");
        void applySettings(map<string, string> options);

        void addLevel(float distance, int downsampling, bool stream = false);
        void setupLODs();

        void setupMaterial(bool lit, int pointsize, bool doSplat = false, float splatModifier = 0.001);
        VRMaterialPtr getMaterial();

        void addPoint(Vec3d p, Color3ub c);
        void addPoint(Vec3d p, Splat c);

        vector<Splat> radiusSearch(Vec3d p, double r);
        vector<Splat> externalRadiusSearch(string path, Vec3d p, double r);

        void convert(string pathIn, string pathOut);
        void convertMerge(vector<string> pathIn, string pathOut);
        void genTestFile(string path, size_t N, bool doColor);
        void genTestFile2(string path, size_t N, bool doColor, int splatSize);

        void externalTransform(string path, PosePtr p);
        void externalSort(string path, size_t chunkSize, double binSize);
        void externalPartition(string path);
        void externalComputeSplats(string path, float neighborsRadius = 0.1);
        void externalColorize(string path, string images, PosePtr pcPose, float localNorth, float localEast, float pDist, int i1, int i2);

        Vec3ub projectOnPanorama(Vec3d P, VRTexturePtr tex, PosePtr vP);
        Splat computeSplat(Vec3d p0, vector<Splat> neighbors);

        shared_ptr<Octree<PntData>>& getOctree();
        VRGeometryPtr getOctreeVisual();
};

OSG_END_NAMESPACE;

#endif // VRPOINTCLOUD_H_INCLUDED
