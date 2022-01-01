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

            map<string, string> params;
            params["N_points"] = toString(cN);
            params["format"] = "x8y8z8";
            if (hasCol) params["format"] += "r1g1b1";
            VRPointCloud::writePCBHeader(pathOut, params);

            ofstream stream(pathOut, ios::app);
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
    importOptions["filePath"] = path;
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
                Color3ub col(r[j], g[j], b[j]);
                pointcloud->addPoint(pos, col);
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

vector<size_t> extractRegionBounds(string path, vector<double> region) {
    if (region.size() != 6) return {};

    auto params = VRPointCloud::readPCBHeader(path);
    ifstream stream(path);
    int hL = toInt(params["headerLength"]);
    stream.seekg(hL);

    auto cN = toValue<size_t>(params["N_points"]);
    bool hasCol = contains( params["format"], "r");
    double binSize = toValue<double>(params["binSize"]);
    if (binSize == 0) {
        cout << " extractRegionBounds failed, binSize is 0!" << endl;
        return {};
    }

    size_t s0 = stream.tellg();
    int N = sizeof(Vec3d);
    if (hasCol) N += sizeof(Vec3ub);

    cout << " extractRegionBounds - headers: " << toString(params) << ", cN: " << cN << endl;

    auto getPoint = [&](size_t pID) -> Vec3d {
        VRPointCloud::PntCol pnt;
        stream.seekg(s0+N*pID, ios::beg);
        stream.read((char*)&pnt, N);
        return pnt.p;
    };

    static const double eps = 1e-6;

    struct Segment {
        int coord;
        double min;
        double max;

        Segment(int c, double binSize) {
            coord = c;
            min = coord*binSize - binSize - eps;
            max = min + binSize;
        }

        Segment(double& v, double binSize) {
            coord = round(0.5 + eps + v/binSize);
            min = coord*binSize - binSize - eps;
            max = min + binSize;
        }
    };

    auto homeIn = [&](size_t& A, size_t& B, int axis, double value) {
        //cout << " homeIn on " << v << " in range " << Vec2i(A, B) << endl;
        if (A >= B) {
            cout << " homeIn error, inverted range " << Vec2i(A, B) << endl;
            return;
        }

        while(B-A > 1) {
            size_t M = (A + B)*0.5;
            Vec3d PM = getPoint(M);
            if (PM[axis] < value) A = M;
            if (PM[axis] > value) B = M;
            //cout << "  M " << M << ", AB " << Vec2i(A, B) << endl;
        }
    };

    Segment segX1(region[0], binSize);
    Segment segX2(region[1], binSize);
    Segment segY1(region[2], binSize);
    Segment segY2(region[3], binSize);
    Segment segZ1(region[4], binSize);
    Segment segZ2(region[5], binSize);
    vector<size_t> bounds;

    // first find bounds on Y axis and all layers
    vector<size_t> Ybounds = { 0, cN, cN };
    homeIn(Ybounds[0], Ybounds[1], 1, segY1.min); // Y lower bound
    homeIn(Ybounds[0], Ybounds[2], 1, segY2.max); // Y upper bound

    struct Range {
        Segment segment;
        size_t b0 = 0;
        size_t b1 = 0;

        Range(int i, double binSize) : segment(i, binSize) {}
    };

    //bounds.push_back(Ybounds[1]);
    //bounds.push_back(Ybounds[2]);

    for (int i = segY1.coord; i <= segY2.coord; i++) {
        Range layer(i, binSize);

        size_t lb = Ybounds[1];
        layer.b0 = Ybounds[2];
        layer.b1 = Ybounds[2];
        homeIn(lb, layer.b0, 1, layer.segment.min); // layer Y lower bound
        homeIn(lb, layer.b1, 1, layer.segment.max); // layer Y upper bound

        //cout << "  found layer " << i << ", " << Vec2d(layer.b0, layer.b1) << endl;

        for (int j = segX1.coord; j <= segX2.coord; j++) { // then find all row bounds in X
            Range row(j, binSize);
            size_t lb = layer.b0;
            row.b0 = layer.b1;
            row.b1 = layer.b1;
            homeIn(lb, row.b0, 0, row.segment.min); // row X lower bound
            homeIn(lb, row.b1, 0, row.segment.max); // row X upper bound

            //bounds.push_back(row.b0);
            //bounds.push_back(row.b1);


            lb = row.b0;
            size_t cb0 = row.b1;
            size_t cb1 = row.b1;
            homeIn(lb, cb0, 2, segZ1.min); // cell Z lower bound
            homeIn(lb, cb1, 2, segZ2.max); // cell Z upper bound

            if (cb1 > cb0) {
                bounds.push_back(cb0);
                bounds.push_back(cb1);
            }

            //cout << "   found row " << j << ", " << Vec2d(row.b0, row.b1) << ", Z " << Vec2d(cb0, cb1) << endl;
        }
    }

    cout << "extractRegionBounds " << toString(bounds) << endl;
    return bounds;
}

void OSG::loadPCB(string path, VRTransformPtr res, map<string, string> importOptions) {
    cout << "load PCB pointcloud " << path << endl;
    importOptions["filePath"] = path;
    res->setName(path);

    float downsampling = 1;
    if (importOptions.count("downsampling")) downsampling = toFloat(importOptions["downsampling"]);

    auto region = toValue<vector<double>>(importOptions["region"]);
    auto bounds = extractRegionBounds(path, region);

    auto params = VRPointCloud::readPCBHeader(path);
    ifstream stream(path);
    int hL = toInt(params["headerLength"]);
    stream.seekg(hL);
    cout << "  headers: " << toString(params) << endl;

    auto cN = toValue<size_t>(params["N_points"]);
    bool hasCol = contains( params["format"], "r");
    bool hasSpl = contains( params["format"], "u");
    //double binSize = toValue<double>(params["binarySize"]);
    cout << "  scan contains " << cN << " points" << endl;

    auto progress = VRProgress::create();
    progress->setup("process points ", cN);
    progress->reset();

    if (hasCol) cout << "   scan has colors\n";
    else cout << "   scan has no colors\n";
    if (hasSpl) cout << "   scan has splats\n";
    else cout << "   scan has no splats\n";

    auto pointcloud = VRPointCloud::create("pointcloud");
    pointcloud->applySettings(importOptions);

    size_t Nskip = round(1.0/downsampling) - 1;
    size_t count = 0;
    int Nskipped = 0;

    int N = sizeof(Vec3d);
    if (hasCol) N = VRPointCloud::PntCol::size;
    if (hasSpl) N = VRPointCloud::Splat::size;
    VRPointCloud::Splat pnt;

    auto skip = [&](size_t Nskip) {
        size_t Nprocessed = min(Nskip, progress->left());
        if (Nprocessed == 0) return;
        if (Nprocessed < 10) stream.ignore(N*Nprocessed, EOF); // ignore processes all chars
        else stream.seekg(N*Nprocessed, ios::cur); // seek jumps directly, better with increasing jump length
        progress->update( Nprocessed );
    };

    int boundsID = 0;
    int Nbounds = bounds.size()*0.5;
    if (Nbounds > 0 && bounds[0] > 0) skip(bounds[0]);

    while (stream.read((char*)&pnt, N)) {
        Vec3d  pos = pnt.p;
        Vec3ub rgb = pnt.c;
        //cout << "  read pnt: " << pos << " " << rgb << " " << endl;
        //Color3f col(rgb[0]/255.0, rgb[1]/255.0, rgb[2]/255.0);
        if (hasSpl) pointcloud->addPoint(pos, pnt);
        else pointcloud->addPoint(pos, rgb);
        progress->update( 1 );
        if (Nskip>0) skip(Nskip);

        if (boundsID < Nbounds) {
            size_t upperBound = bounds[boundsID*2+1];
            if ( upperBound > 0 && progress->current() >= upperBound) {
                boundsID++;
                if (boundsID < Nbounds) {
                    size_t nextLowerBound = bounds[boundsID*2];
                    skip(nextLowerBound-upperBound);
                } else break;
            }
        }
    }

    pointcloud->setupLODs();
    res->addChild(pointcloud);
    stream.close();
}

void OSG::loadXYZ(string path, VRTransformPtr res, map<string, string> importOptions) {
    cout << "load xyz pointcloud " << path << endl;
    importOptions["filePath"] = path;
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
                        Color3ub col(vertex[3], vertex[4], vertex[5]);
                        pointcloud->addPoint(pos, col);
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
                        Color3ub col(0, 0, 0);
                        pointcloud->addPoint(pos, col);
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
            Color3ub C = data.getColor3ub(i);
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





