#include "E57.h"

#include <iostream>
#include "E57Foundation.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/VRLod.h"
#include "core/math/boundingbox.h"
#include "core/utils/VRProgress.h"

using namespace e57;
using namespace std;
using namespace OSG;

VRTransformPtr OSG::fancyyE57import(string path) {
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
        cout << "  scan " << i << " contains " << cN << " points\n";



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

        auto wmat = VRMaterial::create("wmat");
        wmat->setLineWidth(2);
        wmat->setLit(0);
        wmat->setWireFrame(1);

        map<Vec3i, VRLodPtr> lods;
        if (true) {
            for (int i=0; i<10; i++) {
                for (int j=0; j<10; j++) {
                    for (int k=0; k<10; k++) {
                        Vec3i P(i,j,k);
                        Vec3d c(modelCenter[0] + (i - 4.5)*modelSize[0]/10, modelCenter[1] + (j - 4.5)*modelSize[1]/10, modelCenter[2] + (k - 4.5)*modelSize[2]/10);
                        auto l = VRLod::create("chunk");
                        l->setCenter(c);
                        l->addDistance(10);
                        l->addChild( VRGeometry::create("high") );
                        l->addChild( VRGeometry::create("low") );

                        dynamic_pointer_cast<VRGeometry>(l->getChild(0))->setFrom(c);
                        dynamic_pointer_cast<VRGeometry>(l->getChild(1))->setFrom(c);

                        auto b = VRGeometry::create("box");
                        b->setPrimitive("Box 3.2 4.2 2.3 1 1 1");
                        l->getChild(1)->addChild(b);
                        b->setMaterial(wmat);

                        system->addChild(l);
                        lods[P] = l;
                    }
                }
            }
        }

        map<Vec3i, VRGeoData> chunks1;
        map<Vec3i, VRGeoData> chunks2;
        if (true) { // setup system
            do {
                gotCount = reader.read();
                for (unsigned j=0; j < gotCount; j++) {
                    Vec3d p0 = Vec3d(x[j], y[j], z[j]);
                    Color3f c0(r[j]/255.0, g[j]/255.0, b[j]/255.0);
                    Vec3d p = p0 - modelCenter;
                    Vec3i P( round(4.5 + p[0]/(modelSize[0]/10)), round(4.5 + p[1]/(modelSize[1]/10)), round(4.5 + p[2]/(modelSize[2]/10)) );
                    if (P[0] < 0) P[0] = 0; if (P[0] > 9) P[0] = 9;
                    if (P[1] < 0) P[1] = 0; if (P[1] > 9) P[1] = 9;
                    if (P[2] < 0) P[2] = 0; if (P[2] > 9) P[2] = 9;

                    if (chunks1[P].size() > 1e6) continue;
                    Vec3d c1 = dynamic_pointer_cast<VRGeometry>(lods[P]->getChild(0))->getFrom();
                    chunks1[P].pushVert(p0 - c1, Vec3d(0,1,0), c0);
                    chunks1[P].pushPoint();

                    if (chunks2[P].size() > 2e3) continue;
                    Vec3d c2 = dynamic_pointer_cast<VRGeometry>(lods[P]->getChild(1))->getFrom();
                    chunks2[P].pushVert(p0 - c2, Vec3d(0,1,0), c0);
                    chunks2[P].pushPoint();
                }
                progress->update( gotCount );
            } while(gotCount);
        }

        auto mat = VRMaterial::create("pcmat");
        mat->setPointSize(5);
        mat->setLit(0);

        for (auto chunk : chunks1) {
            Vec3i P = chunk.first;
            auto geo = dynamic_pointer_cast<VRGeometry>(lods[P]->getChild(0));
            chunk.second.apply( geo );
            geo->setMaterial(mat);
        }

        for (auto chunk : chunks2) {
            Vec3i P = chunk.first;
            auto geo = dynamic_pointer_cast<VRGeometry>(lods[P]->getChild(1));
            chunk.second.apply( geo );
            geo->setMaterial(mat);
        }

        if (false) { // get boundingbox
            Boundingbox bb;
            do {
                gotCount = reader.read();
                for (unsigned j=0; j < gotCount; j++) {
                    Vec3d p = Vec3d(x[j], y[j], z[j]);
                    bb.update(p);
                }
                progress->update( gotCount );
            } while(gotCount);
            cout << "BoundingBox: " << bb.center() << "    " << bb.size() << endl;
        }

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





