#include "VRPointCloud.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/VRLod.h"
#include "core/math/Octree.h"

using namespace OSG;
using namespace std;

VRPointCloud::VRPointCloud(string name) : VRTransform(name) {
    octree = Octree::create(10);
    mat = VRMaterial::create("pcmat");
}

VRPointCloud::~VRPointCloud() {}

VRPointCloudPtr VRPointCloud::create(string name) { return VRPointCloudPtr( new VRPointCloud(name) ); }

void VRPointCloud::setupMaterial(bool lit, int pointsize) {
    mat->setLit(lit);
    mat->setPointSize(pointsize);
}

VRMaterialPtr VRPointCloud::getMaterial() { return mat; }
OctreePtr VRPointCloud::getOctree() { return octree; }

void VRPointCloud::setupLODs() {
    //cout << "\nsetup lods for " << double(Np)/1e6 << " M points" << endl;
    for (auto leaf : octree->getAllLeafs()) {
        Vec3d c = leaf->getCenter();

        auto geo1 = VRGeometry::create("lvl1");
        auto geo2 = VRGeometry::create("lvl2");
        auto geo3 = VRGeometry::create("lvl3");
        geo1->setFrom(c);
        geo2->setFrom(c);
        geo3->setFrom(c);

        auto l = VRLod::create("chunk");
        l->setCenter(c);
        l->addDistance(4.5);
        l->addDistance(7);
        l->addChild( geo1 );
        l->addChild( geo2 );
        l->addChild( geo3 );

        addChild(l);

        VRGeoData chunk1;
        VRGeoData chunk2;
        VRGeoData chunk3;

        if (leaf->dataSize() > 1e5) cout << "leafsize: " << leaf->dataSize() << endl;

        for (int i = 0; i < leaf->dataSize(); i++) {
            void* d = leaf->getData(i);
            Vec3d p = leaf->getPoint(i);
            Color3f col = *((Color3f*)d);
            chunk1.pushVert(p - c, Vec3d(0,1,0), col);
            chunk1.pushPoint();
        }

        for (int i = 0; i < leaf->dataSize(); i+=20) {
            void* d = leaf->getData(i);
            Vec3d p = leaf->getPoint(i);
            Color3f col = *((Color3f*)d);
            chunk2.pushVert(p - c, Vec3d(0,1,0), col);
            chunk2.pushPoint();
        }

        for (int i = 0; i < leaf->dataSize(); i+=200) {
            void* d = leaf->getData(i);
            Vec3d p = leaf->getPoint(i);
            Color3f col = *((Color3f*)d);
            chunk3.pushVert(p - c, Vec3d(0,1,0), col);
            chunk3.pushPoint();
        }

        if (chunk1.size() > 0) chunk1.apply( geo1 );
        if (chunk2.size() > 0) chunk2.apply( geo2 );
        if (chunk3.size() > 0) chunk3.apply( geo3 );
        geo1->setMaterial(mat);
        geo2->setMaterial(mat);
        geo3->setMaterial(mat);

        leaf->delContent<Color3f>();
    }

    //addChild(octree->getVisualization());
}
