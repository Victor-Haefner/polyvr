#include "E57.h"

#include <iostream>
#include "E57Foundation.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/VRPointCloud.h"
#include "core/objects/VRLod.h"
#include "core/math/boundingbox.h"
#include "core/math/Octree.h"
#include "core/utils/VRProgress.h"
#include "core/objects/VRPointCloud.h"

using namespace e57;
using namespace std;
using namespace OSG;

VRTransformPtr OSG::fancyyE57import(string path) {
    cout << "fancyE57import " << path << endl;
    ImageFile imf(path, "r"); // Read file from disk
    StructureNode root = imf.root();
    if (!root.isDefined("/data3D")) { cout << "File doesn't contain 3D images" << endl; return 0; }

    e57::Node n = root.get("/data3D");
    if (n.type() != E57_VECTOR) { cout << "bad file" << endl; return 0; }

    VectorNode data3D(n);
    int64_t scanCount = data3D.childCount(); // number of scans in file
    cout << " file read succefully, it contains " << scanCount << " scans" << endl;

    auto system = VRTransform::create("system");

    for (int i = 0; i < scanCount; i++) {
        StructureNode scan(data3D.get(i));
        string sname = scan.pathName();

        CompressedVectorNode points( scan.get("points") );
        string pname = points.pathName();
        auto cN = points.childCount();
        cout << "  scan " << i << " contains " << double(cN)/1e6 << " M points\n";



        auto progress = VRProgress::create();
        progress->setup("process points ", cN);
        progress->reset();


        StructureNode proto(points.prototype());
        bool hasPos = (proto.isDefined("cartesianX") && proto.isDefined("cartesianY") && proto.isDefined("cartesianZ"));
        bool hasCol = (proto.isDefined("colorRed") && proto.isDefined("colorGreen") && proto.isDefined("colorBlue"));
        if (!hasPos) continue;

        if (hasCol) cout << "   scan has colors\n";
        else cout << "   scan has no colors\n";

        for (int i=0; i<proto.childCount(); i++) {
            auto child = proto.get(i);
            cout << "    scan data: " << child.pathName() << endl;
        }

        vector<SourceDestBuffer> destBuffers;
        const int N = 4;
        double x[N]; destBuffers.push_back(SourceDestBuffer(imf, "cartesianX", x, N, true));
        double y[N]; destBuffers.push_back(SourceDestBuffer(imf, "cartesianY", y, N, true));
        double z[N]; destBuffers.push_back(SourceDestBuffer(imf, "cartesianZ", z, N, true));
        double r[N];
        double g[N];
        double b[N];
        if (hasCol) {
            destBuffers.push_back(SourceDestBuffer(imf, "colorRed", r, N, true));
            destBuffers.push_back(SourceDestBuffer(imf, "colorGreen", g, N, true));
            destBuffers.push_back(SourceDestBuffer(imf, "colorBlue", b, N, true));
        }

        Vec3d modelCenter(-2.19275, 0.756323, 121.323);
        Vec3d modelSize(32.3683, 42.6181, 23.0225);

        unsigned int gotCount = 0;
        CompressedVectorReader reader = points.reader(destBuffers);

        auto pointcloud = VRPointCloud::create("pointcloud");
        pointcloud->setupMaterial(0, 10);

        cout << "fill octree" << endl;
        int Nd = 30;
        int Nc = 0;
        size_t Np = 0;
        do {
            gotCount = reader.read();
            if (Nc+gotCount<Nd) Nc += gotCount;
            else for (unsigned j=0; j < gotCount; j++) {
                Nc++;
                if (Nc >= Nd) {
                    Vec3d p = Vec3d(x[j], y[j], z[j]);
                    Color3f col(r[j]/255.0, g[j]/255.0, b[j]/255.0);
                    pointcloud->getOctree()->add(p, new Color3f(col), -1, true, 1e5);
                    Nc = 0;
                    Np++;
                }
            }
            progress->update( gotCount );
            //if (progress->get() > 0.2) break;
        } while(gotCount);

        pointcloud->setupLODs();
        system->addChild(pointcloud);

        reader.close();
    }

    imf.close();
    return system;
}

void OSG::loadE57(string path, VRTransformPtr res) {
    cout << "load e57 pointcloud " << path << endl;
    res->setName(path);

    try {
        ImageFile imf(path, "r"); // Read file from disk

        StructureNode root = imf.root();
        if (!root.isDefined("/data3D")) { cout << "File doesn't contain 3D images" << endl; return; }

        e57::Node n = root.get("/data3D");
        if (n.type() != E57_VECTOR) { cout << "bad file" << endl; return; }

        VectorNode data3D(n);
        int64_t scanCount = data3D.childCount(); // number of scans in file
        cout << " file read succefully, it contains " << scanCount << " scans" << endl;

        for (int i = 0; i < scanCount; i++) {
            StructureNode scan(data3D.get(i));
            string sname = scan.pathName();

            CompressedVectorNode points( scan.get("points") );
            string pname = points.pathName();
            auto cN = points.childCount();
            cout << "  scan " << i << " contains " << cN << " points\n";

            StructureNode proto(points.prototype());
            bool hasPos = (proto.isDefined("cartesianX") && proto.isDefined("cartesianY") && proto.isDefined("cartesianZ"));
            bool hasCol = (proto.isDefined("colorRed") && proto.isDefined("colorGreen") && proto.isDefined("colorBlue"));
            if (!hasPos) continue;

            if (hasCol) cout << "   scan has colors\n";
            else cout << "   scan has no colors\n";

            for (int i=0; i<proto.childCount(); i++) {
                auto child = proto.get(i);
                cout << "    scan data: " << child.pathName() << endl;
            }

            vector<SourceDestBuffer> destBuffers;
            const int N = 4;
            double x[N]; destBuffers.push_back(SourceDestBuffer(imf, "cartesianX", x, N, true));
            double y[N]; destBuffers.push_back(SourceDestBuffer(imf, "cartesianY", y, N, true));
            double z[N]; destBuffers.push_back(SourceDestBuffer(imf, "cartesianZ", z, N, true));
            double r[N];
            double g[N];
            double b[N];
            if (hasCol) {
                destBuffers.push_back(SourceDestBuffer(imf, "colorRed", r, N, true));
                destBuffers.push_back(SourceDestBuffer(imf, "colorGreen", g, N, true));
                destBuffers.push_back(SourceDestBuffer(imf, "colorBlue", b, N, true));
            }

            int Nchunk = 1e6; // separate in chunks because of tcmalloc large alloc issues
            VRGeoData data;
            unsigned int gotCount = 0;
            CompressedVectorReader reader = points.reader(destBuffers);
            do {
                if (data.size() > Nchunk) {
                    cout << "  assemble geometry.. " << endl;
                    auto geo = data.asGeometry(pname);
                    res->addChild(geo);
                    data = VRGeoData();
                }

                gotCount = reader.read();
                for (unsigned j=0; j < gotCount; j++) {
                    int v;
                    if (hasCol) v = data.pushVert(Pnt3d(x[j], y[j], z[j]), Vec3d(0,1,0), Color3f(r[j]/255.0, g[j]/255.0, b[j]/255.0));
                    else v = data.pushVert(Pnt3d(x[j], y[j], z[j]), Vec3d(0,1,0));
                    data.pushPoint(v);
                }
            } while(gotCount);
            reader.close();

            if (data.size()) {
                cout << "  assemble geometry.. " << endl;
                auto geo = data.asGeometry(pname);
                res->addChild(geo);
            }
        }

        imf.close();
    }
    catch (E57Exception& ex) { ex.report(__FILE__, __LINE__, __FUNCTION__); return; }
    catch (std::exception& ex) { cerr << "Got an std::exception, what=" << ex.what() << endl; return; }
    catch (...) { cerr << "Got an unknown exception" << endl; return; }
}

void OSG::loadXYZ(string path, VRTransformPtr res) {
    cout << "load xyz pointcloud " << path << endl;
    res->setName(path);

    try {

        VRGeoData data;
        vector<float> vertex = vector<float>(6);
        int i=0;

        ifstream file(path);
        while (file >> vertex[i]) {
            i++;
            if (i >= 6) {
                i = 0;
                data.pushVert(Pnt3d(vertex[0], vertex[1], vertex[2]), Vec3d(0,1,0), Color3f(vertex[3]/255.0, vertex[4]/255.0, vertex[5]/255.0));
                data.pushPoint();
            }
        }

        if (data.size()) {
            cout << "  assemble geometry.. " << endl;
            auto geo = data.asGeometry("points");
            res->addChild(geo);
        }
    }
    catch (std::exception& ex) { cerr << "Got an std::exception, what=" << ex.what() << endl; return; }
    catch (...) { cerr << "Got an unknown exception" << endl; return; }
}

//void writeE57(VRGeometryPtr geo, string path);





