#include "E57.h"

#include <iostream>
#include "E57Foundation.h"
#include "E57Simple.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/VRPointCloud.h"
#include "core/objects/VRLod.h"
#include "core/math/partitioning/boundingbox.h"
#include "core/math/partitioning/Octree.h"
#include "core/utils/VRProgress.h"
#include "core/utils/toString.h"

#include <iostream>

using namespace e57;
using namespace std;
using namespace OSG;

void OSG::genTestPC(string path, size_t N, bool doColor) {
    size_t n = cbrt(N);
    double f = 255.0/n;
    double s = 0.01;
    N = n*n*n;

    ofstream stream(path);
    stream << "x8y8z8";
    if (doColor) stream << "r1g1b1";
    stream << "\n";
    stream << toString(N);
    stream << "\n";

    auto progress = VRProgress::create();
    progress->setup("generate points ", N);
    progress->reset();

    for (size_t i=0; i<n; i++) {
        for (size_t j=0; j<n; j++) {
            for (size_t k=0; k<n; k++) {
                Vec3d P(i*s, j*s, k*s);
                Vec3ub C(i*f, j*f, k*f);
                stream.write((const char*)&P[0], sizeof(Vec3d));
                if (doColor) stream.write((const char*)&C[0], sizeof(Vec3ub));
            }
            progress->update( n );
        }
    }

    stream.close();
}

void OSG::convertE57(string pathIn, string pathOut) {
    try {
        ImageFile imf(pathIn, "r"); // Read file from disk

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

            int gotCount = 0;
            CompressedVectorReader reader = points.reader(destBuffers);

            ofstream stream(pathOut);
            stream << "x8y8z8";
            if (hasCol) stream << "r1g1b1";
            stream << "\n";
            stream << toString(cN);
            stream << "\n";

            do {
                gotCount = (int)reader.read();

                for (int j=0; j < gotCount; j++) {
                    Vec3d P(x[j], y[j], z[j]);
                    Vec3ub C(r[j], g[j], b[j]);
                    stream.write((const char*)&P[0], sizeof(Vec3d));
                    stream.write((const char*)&C[0], sizeof(Vec3ub));
                }
                progress->update( gotCount );

            } while(gotCount);
            stream.close();
            reader.close();
        }

        imf.close();
    }
    catch (E57Exception& ex) { ex.report(__FILE__, __LINE__, __FUNCTION__); return; }
    catch (std::exception& ex) { cerr << "Got an std::exception, what=" << ex.what() << endl; return; }
    catch (...) { cerr << "Got an unknown exception" << endl; return; }
}

void OSG::loadE57(string path, VRTransformPtr res, map<string, string> importOptions) {
    cout << "load e57 pointcloud " << path << endl;
    res->setName(path);

    float downsampling = 1;
    if (importOptions.count("downsampling")) downsampling = toFloat(importOptions["downsampling"]);

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
            const int N = 1;
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

            auto pointcloud = VRPointCloud::create("pointcloud");
            pointcloud->applySettings(importOptions);

            auto pushPoint = [&](int j) {
                Vec3d pos = Vec3d(x[j], y[j], z[j]);
                Color3f col(r[j]/255.0, g[j]/255.0, b[j]/255.0);
                pointcloud->getOctree()->add(pos, new Color3f(col), -1, true, 1e5);
            };

            CompressedVectorReader reader = points.reader(destBuffers);

            //reader.seek(5500); // test

            cout << "fill octree" << endl;
            size_t Nskip = round(1.0/downsampling) - 1;
            size_t count = 0;
            int Nskipped = 0;
            int gotCount = 0;
            do {
                gotCount = (int)reader.read();

                /*if (gotCount > 0) {
                    int j = 0;
                    pushPoint(j);

                    Nskip = min(Nskip, cN-count);
                    if (Nskip > 0) reader.skip(Nskip, count);
                    progress->update( gotCount+Nskip );
                    count += gotCount+Nskip;

                    //if (count >= cN) break;
                }*/

                if (gotCount > 0) {

                    if (Nskipped+gotCount <= Nskip) Nskipped += gotCount;
                    else for (int j=0; j < gotCount; j++) {
                        Nskipped++;
                        if (Nskipped >= Nskip) {
                            pushPoint(j);
                            Nskipped = 0;
                        }
                    }

                    progress->update( gotCount );
                }

            } while(gotCount);

            pointcloud->setupLODs();
            res->addChild(pointcloud);

            reader.close();
        }

        imf.close();
    }
    catch (E57Exception& ex) { ex.report(__FILE__, __LINE__, __FUNCTION__); return; }
    catch (std::exception& ex) { cerr << "Got an std::exception, what=" << ex.what() << endl; return; }
    catch (...) { cerr << "Got an unknown exception" << endl; return; }
}

void OSG::loadPCB(string path, VRTransformPtr res, map<string, string> importOptions) {
    cout << "load PCB pointcloud " << path << endl;
    res->setName(path);

    float downsampling = 1;
    if (importOptions.count("downsampling")) downsampling = toFloat(importOptions["downsampling"]);

    ifstream stream(path);
    string h1, h2;
    getline(stream, h1);
    getline(stream, h2);
    cout << "  headers " << h1 << " " << h2 << endl;

    auto cN = toValue<size_t>(h2);
    cout << "  scan contains " << cN << " points" << endl;

    auto progress = VRProgress::create();
    progress->setup("process points ", cN);
    progress->reset();

    bool hasCol = contains(h1, "r");
    if (hasCol) cout << "   scan has colors\n";
    else cout << "   scan has no colors\n";

    auto pointcloud = VRPointCloud::create("pointcloud");
    pointcloud->applySettings(importOptions);

    cout << "fill octree" << endl;
    size_t Nskip = round(1.0/downsampling) - 1;
    size_t count = 0;
    int Nskipped = 0;

    int N = sizeof(Vec3d);
    if (hasCol) N += sizeof(Vec3ub);
    char data[256];

    while (!stream.eof()) {
        stream.read(&data[0], N);

        Vec3d pos = *(Vec3d*)&data[0];
        Vec3ub rgb = *(Vec3ub*)&data[sizeof(Vec3d)];
        Color3f col(rgb[0]/255.0, rgb[1]/255.0, rgb[2]/255.0);
        pointcloud->addPoint(pos, col);

        int Nprocessed = 0;
        if (Nskip>0) {
            Nprocessed = min(Nskip, progress->left());
            if (Nprocessed == 0) break;
            if (Nprocessed < 10) stream.ignore(N*Nprocessed, EOF); // ignore processes all chars
            else stream.seekg(stream.tellg()+N*Nprocessed); // seek jumps directly, better with jump length
        }

        progress->update( Nprocessed+1 );
    }

    pointcloud->setupLODs();
    res->addChild(pointcloud);
    stream.close();
}

void OSG::loadXYZ(string path, VRTransformPtr res, map<string, string> importOptions) {
    cout << "load xyz pointcloud " << path << endl;
    res->setName(path);

    float downsampling = 1;
    bool swapYZ = 0;
    bool xyzNoColor = 0;
    if (importOptions.count("downsampling")) downsampling = toFloat(importOptions["downsampling"]);
    if (importOptions.count("swapYZ")) swapYZ = toInt(importOptions["swapYZ"]);
    if (importOptions.count("xyzNoColor")) xyzNoColor = toInt(importOptions["xyzNoColor"]);

    /*cout << "OSG::loadXYZ, swapYZ: " << swapYZ << endl;
    for (auto o : importOptions) {
        cout << " importOption " << o.first << " " << o.second << endl;
    }*/
    try {
        auto pointcloud = VRPointCloud::create("pointcloud");
        pointcloud->applySettings(importOptions);

        VRGeoData data;
        if (!xyzNoColor){
            vector<float> vertex = vector<float>(6);
            int i=0;
            int Nskip = round(1.0/downsampling);
            int Nskipped = 0;

            ifstream file(path);
            while (file >> vertex[i]) {
                i++;
                if (i >= 6) {
                    i = 0;
                    Nskipped++;
                    if (Nskipped >= Nskip) {
                        //data.pushVert(Pnt3d(vertex[0], vertex[1], vertex[2]), Vec3d(0,1,0), Color3f(vertex[3]/255.0, vertex[4]/255.0, vertex[5]/255.0));
                        //data.pushPoint();
                        Vec3d pos;
                        if (swapYZ) pos = Vec3d(vertex[0], vertex[2], -vertex[1]);
                        else pos = Vec3d(vertex[0], vertex[1], vertex[2]);
                        Color3f col(vertex[3]/255.0, vertex[4]/255.0, vertex[5]/255.0);
                        pointcloud->getOctree()->add(pos, new Color3f(col), -1, true, 1e5);
                        Nskipped = 0;
                    }
                }
            }
        } else {
            vector<float> vertex = vector<float>(3);
            cout << " pointcloud, no Color" << endl;
            int i=0;
            int Nskip = round(1.0/downsampling);
            int Nskipped = 0;

            ifstream file(path);
            while (file >> vertex[i]) {
                i++;

                if (i >= 3) {
                    i = 0;
                    Nskipped++;
                    if (Nskipped >= Nskip) {

                        /*
                        if (swapYZ) data.pushVert(Pnt3d(vertex[0], vertex[2], -vertex[1]), Vec3d(0,1,0), Color3f(0.0, 0.0, 0.0));
                        else data.pushVert(Pnt3d(vertex[0], vertex[1], vertex[2]), Vec3d(0,1,0), Color3f(0.0, 0.0, 0.0));
                        data.pushPoint();
                        */

                        Vec3d pos;
                        if (swapYZ) pos = Vec3d(vertex[0], vertex[2], -vertex[1]);
                        else pos = Vec3d(vertex[0], vertex[1], vertex[2]);
                        Color3f col(0.0, 0.0, 0.0);
                        pointcloud->getOctree()->add(pos, new Color3f(col), -1, true, 1e5);
                        Nskipped = 0;
                    }
                }
            }
        }

        if (data.size()) {
            cout << "  assemble geometry.. " << endl;
            auto geo = data.asGeometry("points");
            res->addChild(geo);
        }

        pointcloud->setupLODs();
        res->addChild(pointcloud);
    }
    catch (std::exception& ex) { cerr << "Got an std::exception, what=" << ex.what() << endl; return; }
    catch (...) { cerr << "Got an unknown exception" << endl; return; }
}

void OSG::writeE57(VRPointCloudPtr pcloud, string path) {
    if (!pcloud) return;

    vector<double>  cartesianX;
    vector<double>  cartesianY;
    vector<double>  cartesianZ;
    vector<int>  colorRed;
    vector<int>  colorGreen;
    vector<int>  colorBlue;

    for (auto g : pcloud->getChildren(true, "Geometry")) {
        VRGeometryPtr geo = dynamic_pointer_cast<VRGeometry>(g);
        Matrix4d M = pcloud->getMatrixTo(geo);
        if (!geo) continue;
        cout << "OSG::writeE57 " << geo->getName() << endl;
        VRGeoData data(geo);
        for (int i=0; i<data.size(); i++) {
            Pnt3d P = data.getPosition(i);
            M.mult(P, P);
            Color3f C = data.getColor3(i);
            cartesianX.push_back(P[0]);
            cartesianY.push_back(P[1]);
            cartesianZ.push_back(P[2]);
            colorRed.push_back(round(C[0]*255));
            colorGreen.push_back(round(C[1]*255));
            colorBlue.push_back(round(C[2]*255));
        }
    }

    try {
        ImageFile imf(path, "w");
        StructureNode root = imf.root();

        root.set("formatName", StringNode(imf, "ASTM E57 3D Imaging Data File"));
        root.set("guid", StringNode(imf, "3F2504E0-4F89-11D3-9A0C-0305E82C3300"));

        int astmMajor;
        int astmMinor;
        ustring libraryId;
        E57Utilities().getVersions(astmMajor, astmMinor, libraryId);
        root.set("versionMajor", IntegerNode(imf, astmMajor));
        root.set("versionMinor", IntegerNode(imf, astmMinor));
        root.set("coordinateMetadata", StringNode(imf, "Not used."));

        StructureNode creationDateTime = StructureNode(imf);
        root.set("creationDateTime", creationDateTime);
        creationDateTime.set("dateTimeValue", FloatNode(imf));
        creationDateTime.set("isAtomicClockReferenced", IntegerNode(imf));

        VectorNode data3D = VectorNode(imf, true);
        root.set("data3D", data3D);

        StructureNode scan0 = StructureNode(imf);
        data3D.append(scan0);
        scan0.set("guid", StringNode(imf, "3F2504E0-4F89-11D3-9A0C-0305E82C3301"));

        StructureNode proto = StructureNode(imf);
        proto.set("cartesianX",  FloatNode(imf));
        proto.set("cartesianY",  FloatNode(imf));
        proto.set("cartesianZ",  FloatNode(imf));
        proto.set("colorRed",    IntegerNode(imf, 0, 0, 255));
        proto.set("colorGreen",  IntegerNode(imf, 0, 0, 255));
        proto.set("colorBlue",   IntegerNode(imf, 0, 0, 255));

        VectorNode codecs = VectorNode(imf, true);
        CompressedVectorNode points = CompressedVectorNode(imf, proto, codecs);
        scan0.set("points", points);
        scan0.set("name", StringNode(imf, pcloud->getName()));
        scan0.set("description", StringNode(imf, "PolyVR pointcloud export"));

        const int N = cartesianX.size();
        vector<SourceDestBuffer> sourceBuffers;
        sourceBuffers.push_back(SourceDestBuffer(imf, "cartesianX",  &cartesianX[0],  N, true, true));
        sourceBuffers.push_back(SourceDestBuffer(imf, "cartesianY",  &cartesianY[0],  N, true, true));
        sourceBuffers.push_back(SourceDestBuffer(imf, "cartesianZ",  &cartesianZ[0],  N, true, true));
        sourceBuffers.push_back(SourceDestBuffer(imf, "colorRed",    &colorRed[0],    N, true));
        sourceBuffers.push_back(SourceDestBuffer(imf, "colorGreen",  &colorGreen[0],  N, true));
        sourceBuffers.push_back(SourceDestBuffer(imf, "colorBlue",   &colorBlue[0],   N, true));

        CompressedVectorWriter writer = points.writer(sourceBuffers);
        writer.write(N);
        writer.close();
        imf.close();
    }
    catch(E57Exception& ex)     { ex.report(__FILE__, __LINE__, __FUNCTION__); }
    catch(std::exception& ex)   { cerr << "Got an std::exception, what=" << ex.what() << endl; }
    catch(...)                  { cerr << "Got an unknown exception" << endl; }
}





